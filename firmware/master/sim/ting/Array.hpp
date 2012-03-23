/* The MIT License:

Copyright (c) 2008-2012 Ivan Gagis <igagis@gmail.com>

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
 * @file Array.hpp
 * @author Ivan Gagis <igagis@gmail.com>
 * @brief automatic array class.
 * Array class. It is an auto pointer for new[] / delete[].
 */

#pragma once

//#define M_ENABLE_ARRAY_PRINT
#ifdef M_ENABLE_ARRAY_PRINT 
#define M_ARRAY_PRINT(x) LOG(x)
#else
#define M_ARRAY_PRINT(x)
#endif

#include "debug.hpp"
#include "types.hpp"
#include "Buffer.hpp"
#include "math.hpp"

namespace ting{


/**
 * @brief wrapper above new[]/delete[].
 * This template class is a wrapper above new[]/delete[] operators.
 * Note that it behaves like auto-pointer. It defines operator=() and copy constructor and
 * when one class instance is assigned (by operator=() or copy constructor) to another,
 * the first one becomes invalid while the second becomes valid and acquires
 * the memory buffer from first.
 */
template <class T> class Array : public ting::Buffer<T>{

	inline void AllocateMemory(size_t arraySize){
		if(arraySize == 0){
			this->size = 0;
			this->buf = 0;
			return;
		}

		ting::u8 *buffer = new ting::u8[arraySize * sizeof(T)];
		ASSERT(buffer)

		this->buf = reinterpret_cast<T*>(buffer);
		this->size = arraySize;

		//check for strict aliasing
		ASSERT(this->buf)
		ASSERT(reinterpret_cast<void*>(this->buf) == buffer)
	}

	inline void InitObjectsByDefaultConstructor(){
		for(T* p = this->Begin(); p != this->End(); ++p){
			try{
				new (p) T();
			}catch(...){
				//exception thrown from one of the objects constructor,
				//destroy all previously created objects.
				for(T* k = this->Begin(); k != p; ++k){
					k->~T();
				}
				throw;//rethrow exception
			}
		}
	}

	inline void InitObjectsByCopyConstructor(const ting::Buffer<T>& buffer){
		ASSERT(this->Size() == buffer.Size())
		const T* s = buffer.Begin();
		for(T* p = this->Begin(); p != this->End(); ++p, ++s){
			try{
				new (p) T(*s);
			}catch(...){
				//exception thrown from one of the objects copy constructor,
				//destroy all previously created objects.
				for(T* k = this->Begin(); k != p; ++k){
					k->~T();
				}
				throw;//rethrow exception
			}
		}
	}

	inline void DestroyObjects()throw(){
		for(T* p = &this->buf[0]; p != &this->buf[this->size]; ++p){
			p->~T();
		}
	}

	inline void FreeMemory()throw(){
		ting::u8 *buffer = reinterpret_cast<ting::u8*>(this->buf);

		//check for strict aliasing
		ASSERT(reinterpret_cast<void*>(this->buf) == buffer)

		delete[] buffer;//It is ok to delete 0 pointer
	}



	inline void Destroy()throw(){
		this->DestroyObjects();
		this->FreeMemory();
	}



public:
	/**
	 * @brief Creates new array of requested size.
	 * Creates new array of requested size. Note, that it will call new[], so
	 * it will initialize all the elements by calling default constructor of
	 * the element class for each element.
	 * @param arraySize - number of elements this array should hold.
	 *                    If 0 is supplied then memory is not allocated and the created
	 *                    Array object is not valid (Array::IsValid() will return false).
	 */
	//NOTE: the constructor is explicit to avoid undesired automatic
	//conversions from size_t to Array.
	explicit inline Array(size_t arraySize = 0){
		this->AllocateMemory(arraySize);
		try{
			this->InitObjectsByDefaultConstructor();
		}catch(...){
			this->FreeMemory();
			throw;
		}
	}



	/**
	 * @brief Creates new Array containing copy of given buffer contents.
	 * Creates a new Array instance of the same size as the given buffer.
	 * The contents of that newly created Array are initialized to the
	 * contents of the given buffer, i.e. all the array elements are
	 * created using copy constructors.
	 * @param b - the buffer from which the data will be copied.
	 */
	//NOTE: the constructor is explicit to avoid possible ambiguities.
	explicit inline Array(const ting::Buffer<T>& b){
		this->AllocateMemory(b.Size());
		try{
			this->InitObjectsByCopyConstructor(b);
		}catch(...){
			this->FreeMemory();
			throw;
		}
	}



private:
	inline void CopyFrom(const Array& a){
		this->size = a.size;
		this->buf = a.buf;
		const_cast<Array&>(a).size = 0;
		const_cast<Array&>(a).buf = 0;
	}



public:
	/**
	 * @brief Copy constructor, works as auto-pointer.
	 * Creates a copy of 'a'. This copy constructor works as auto-pointer.
	 * This means that if creating Array object like this:
	 *     Array<int> a(10);//create array 'a'
	 *     Array<int> b(a);//create array 'b' using copy constructor
	 * then 'a' will become invalid while 'b' will hold pointer to the memory
	 * buffer which 'a' was holding before. I.e. when using copy constructor,
	 * no memory allocation occurs, the memory buffer kept by 'a' is moved to 'b'
	 * and 'a' is invalidated.
	 * @param a - Array object to copy.
	 */
	//copy constructor
	inline Array(const Array& a) :
			ting::Buffer<T>()
	{
		this->CopyFrom(a);
	}



	/**
	 * @brief Assignment operator, works as auto-pointer.
	 * This operator works the same way as copy constructor does.
	 * That is, if assignng like this:
	 *     Array<int> b(20), a(10);
	 *     b = a;
	 * then 'a' will become invalid and 'b' will hold the memory buffer owned by 'a' before.
	 * Note, that memory buffer owned by 'b' prior to assignment is freed and destructors are
	 * called on every element of the buffer (i.e. buffer is freed using delete[] operator).
	 * Thus, no memory leak occurs.
	 * @param a - Array object to assign from.
	 */
	inline Array& operator=(const Array& a){
		//behavior similar to ting::Ptr class
		this->Destroy();
		this->CopyFrom(a);
		return (*this);
	}



	~Array()throw(){
		M_ARRAY_PRINT(<< "Array::~Array(): invoked" << std::endl)
		this->Destroy();
		M_ARRAY_PRINT(<< "Array::~Array(): exit" << std::endl)
	}



	/**
	 * @brief initialize array with new memory buffer of given size.
	 * If array was already initialized then the memory buffer is freed (using delete[])
	 * and new memory buffer of requested size is allocated.
	 * @param arraySize - number of elements this array should hold.
	 *                    If 0 is supplied then array will become invalid.
	 */
	void Init(size_t arraySize){
		M_ARRAY_PRINT(<< "Array::Init(): buf = " << static_cast<void*>(this->buf) << std::endl)
		this->Destroy();
		this->AllocateMemory(arraySize);
		try{
			this->InitObjectsByDefaultConstructor();
		}catch(...){
			this->FreeMemory();
			this->buf = 0;
			this->size = 0;
			throw;
		}
	}



	/**
	 * @brief initialize array with the copy of given buffer contents.
	 * The contents of this Array are initialized to the
	 * contents of the given buffer, i.e. all the array elements are
	 * created using copy constructors.
	 * @param b - the buffer from which the data will be copied.
	 */
	void Init(const ting::Buffer<T>& b){
		this->Destroy();
		this->AllocateMemory(b.Size());
		try{
			this->InitObjectsByCopyConstructor(b);
		}catch(...){
			this->FreeMemory();
			this->buf = 0;
			this->size = 0;
			throw;
		}
	}



	/**
	 * @brief returns true if Array is allocated.
	 * @return true - if this Array object holds memory buffer of not zero size.
	 * @return false - if this Array object does not hold any memory buffer.
	 */
	inline bool IsValid()const throw(){
		return this->buf != 0;
	}



	/**
	 * @brief inverse of Array::IsValid().
	 * Inverse of Array::IsValid().
	 * @return true - if Array is not valid.
	 * @return false - if Array is valid.
	 */
	inline bool IsNotValid()const throw(){
		return !this->IsValid();
	}



	/**
	 * @brief Converts to bool.
	 * @return bool - value of Array::IsValid().
	 */
	//Safe conversion to bool type.
	//Because if using simple "operator bool()" it may result in chained automatic
	//conversion to undesired types such as int.
	typedef void (Array::*unspecified_bool_type)();
	inline operator unspecified_bool_type()const{
		return this->IsValid() ? &Array::Reset : 0; //Array::Reset is taken just because it has matching signature
	}



	/**
	 * @brief free array memory buffer.
	 * Frees memory buffer hold by Array object (if any).
	 * After that the Array object becomes invalid.
	 */
	inline void Reset()throw(){
		this->Destroy();
		this->buf = 0;
		this->size = 0;
	}
};//~template class Array

}//~namespace ting
