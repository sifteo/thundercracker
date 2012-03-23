/* The MIT License:

Copyright (c) 2009-2012 Ivan Gagis <igagis@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */

// Home page: http://ting.googlecode.com



/**
 * @file PoolStored.hpp
 * @author Ivan Gagis <igagis@gmail.com>
 * @brief Memory Pool.
 * Alternative memory allocation functions for simple objects.
 * The main purpose of this facilities is to prevent memory fragmentation.
 */

#pragma once

#include <new>
#include <list>

#include "debug.hpp"
#include "types.hpp"
#include "Thread.hpp"
#include "Exc.hpp"
#include "Array.hpp"
#include "atomic.hpp"


//#define M_ENABLE_POOL_TRACE
#ifdef M_ENABLE_POOL_TRACE
#define M_POOL_TRACE(x) TRACE(<<"[POOL] ") TRACE(x)
#else
#define M_POOL_TRACE(x)
#endif

namespace ting{

//make sure theat we align PoolElem by int size when using MSVC compiler.
STATIC_ASSERT(sizeof(int) == 4)



template <size_t element_size, size_t num_elements_in_chunk = 32> class MemoryPool{		
	struct BufHolder{
		u8 buf[element_size];
	};
	
	struct Chunk;

	M_DECLARE_ALIGNED_MSVC(4) struct PoolElem : public BufHolder{
		PoolElem *next; //initialized only when the PoolElem is freed for the first time.
		Chunk* parent; //initialized only upon PoolElem allocation
	}
	//Align by sizeof(int) boundary, just to be more safe.
	//I once had a problem with pthread mutex when it was not aligned by 4 byte boundary,
	//so I resolved this by declaring PoolElem structure as aligned by sizeof(int).
	M_DECLARE_ALIGNED(sizeof(int));

	struct Chunk : public ting::StaticBuffer<PoolElem, num_elements_in_chunk>{
		Chunk *next, *prev; //for linked list
		
		ting::Inited<size_t, 0> numAllocated;
		
		ting::Inited<size_t, 0> freeIndex;//Used for first pass of elements allocation.
		
		ting::Inited<PoolElem*, 0> firstFree;//After element is freed it is placed into the single-linked list of free elements.
		
		inline Chunk(){
			//there is no reason in memory pool with only 1 element per chunk.
			//This assumption will also help later to  determine if chunk was
			//not full before it become empty, so it should reside in the free
			//chunks list upon turning empty.
			ASSERT(this->Size() > 1)
		}

		inline bool IsFull()const throw(){
			return this->numAllocated == this->Size();
		}
		
		inline bool IsEmpty()const throw(){
			return this->numAllocated == 0;
		}
		
		inline PoolElem* Alloc(){
			if(this->freeIndex != this->Size()){
				ASSERT(this->freeIndex < this->Size())
				PoolElem* ret = &this->operator[](this->freeIndex);
				++this->numAllocated;
				++this->freeIndex;
				ret->parent = this;
				return ret;
			}
			
			ASSERT(this->firstFree)
			
			PoolElem* ret = this->firstFree;
			ASSERT(ret->parent == this)
			this->firstFree = ret->next;
			++this->numAllocated;
			ASSERT(this->numAllocated <= this->Size())
			return ret;
		}

	private:
		Chunk(const Chunk&);
		Chunk& operator=(const Chunk&);//assignment is not allowed (no operator=() implementation provided)
	};

	ting::Inited<unsigned, 0> numChunks; //this is only used for making sure that there are no chunks upon memory pool destruction
	ting::Inited<Chunk*, 0> freeHead; //head of the free chunks list (looped list)
	
	ting::atomic::Flag lockFlag;
	
	//lock
	class Lock{
		ting::atomic::Flag& flag;
	public:
		Lock(ting::atomic::Flag& flag)throw() :
				flag(flag)
		{
			while(this->flag.Set(true)){
				ting::Thread::Sleep(0);
			}
			atomic::MemoryBarrier();
		}
		
		~Lock()throw(){
			atomic::MemoryBarrier();
			this->flag.Clear();
		}
	};
	
public:
	~MemoryPool()throw(){
		ASSERT_INFO(this->numChunks == 0, "MemoryPool: cannot destroy memory pool because it is not empty. Check for static PoolStored objects, they are not allowed, e.g. static Ref/WeakRef are not allowed!")
	}
	
public:
	void* Alloc_ts(){
		Lock guard(this->lockFlag);
		
		if(this->freeHead == 0){
			Chunk* c = new Chunk();
			c->next = c;
			c->prev = c;
			this->freeHead = c;
			++this->numChunks;
		}
		
		ASSERT(this->freeHead)
		ASSERT(!this->freeHead->IsFull())
		
		void* ret = static_cast<BufHolder*>(this->freeHead->Alloc());
		
		if(this->freeHead->IsFull()){
			//remove chunk from free chunks list
			if(this->freeHead->next == this->freeHead){//if it is the only one chunk in the list
				this->freeHead = 0;
			}else{
				this->freeHead->prev->next = this->freeHead->next;
				this->freeHead->next->prev = this->freeHead->prev;
				this->freeHead = this->freeHead->next;
			}
		}
		
		return ret;
	}

	void Free_ts(void* p)throw(){
		if(p == 0){
			return;
		}
		
		Lock guard(this->lockFlag);
		
		PoolElem* e = static_cast<PoolElem*>(static_cast<BufHolder*>(p));
		
		Chunk *c = e->parent;
		
		ASSERT(c->numAllocated > 0)
		
		if(c->numAllocated == 1){//freeing last element in the chunk
			ASSERT(c->Size() >= 2)
			//remove chunk, it should be in free chunks list
			if(this->freeHead->next == this->freeHead){//if it is the only one chunk in the list
				ASSERT(this->freeHead->prev == this->freeHead)
				ASSERT(this->freeHead == c)
				this->freeHead = 0;
			}else{
				ASSERT(this->freeHead->prev != this->freeHead)
				this->freeHead->prev->next = this->freeHead->next;
				this->freeHead->next->prev = this->freeHead->prev;
				if(this->freeHead == c){
					this->freeHead = this->freeHead->next;
				}
			}
			delete c;
			--this->numChunks;
			return;
		}
		
		if(c->IsFull()){//if chunk is full before freeing the element, need to add to the list of free chunks
			//move chunk to the beginning of the list
			ASSERT(c != this->freeHead)
			if(this->freeHead == 0){
				c->next = c;
				c->prev = c;
				this->freeHead = c;
			}else{
				c->prev = this->freeHead;
				c->next = this->freeHead->next;
				c->prev->next = c;
				c->next->prev = c;
			}
		}
		
		e->next = c->firstFree;
		c->firstFree = e;
		--c->numAllocated;
	}
};//~template class MemoryPool



template <size_t element_size, size_t num_elements_in_chunk> class StaticMemoryPool{
	static MemoryPool<element_size, num_elements_in_chunk> instance;
public:
	
	static inline void* Alloc_ts(){
		return instance.Alloc_ts();
	}
	
	static inline void Free_ts(void* p)throw(){
		instance.Free_ts(p);
	}
};



template <size_t element_size, size_t num_elements_in_chunk> typename ting::MemoryPool<element_size, num_elements_in_chunk> ting::StaticMemoryPool<element_size, num_elements_in_chunk>::instance;



/**
 * @brief Base class for pool-stored objects.
 * If the class is derived from PoolStored it will override 'new' and 'delete'
 * operators for that class so that the objects would be stored in the
 * memory pool instead of using standard memory manager to allocate memory.
 * Storing objects in memory pool allows to avoid memory fragmentation.
 * PoolStored is only useful for systems with large amount of small and
 * simple objects which have to be allocated dynamically (i.e. using new/delete
 * operators).
 * For example, PoolStored is used in ting::Ref (reference counted objects)
 * class to allocate reference counting objects which holds number of references  and
 * pointer to reference counted object.
 * NOTE: class derived from PoolStored SHALL NOT be used as a base class further.
 */
template <class T, unsigned num_elements_in_chunk> class PoolStored{

protected:
	//this should only be used as a base class
	PoolStored(){}

public:

	inline static void* operator new(size_t size){
		M_POOL_TRACE(<< "new(): size = " << size << std::endl)
		if(size != sizeof(T)){
			throw ting::Exc("PoolStored::operator new(): attempt to allocate memory block of incorrect size");
		}

		return StaticMemoryPool<sizeof(T), num_elements_in_chunk>::Alloc_ts();
	}

	inline static void operator delete(void *p)throw(){
		StaticMemoryPool<sizeof(T), num_elements_in_chunk>::Free_ts(p);
	}

private:
};



}//~namespace ting
