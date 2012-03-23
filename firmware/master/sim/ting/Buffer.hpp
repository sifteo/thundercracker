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
 * @author Ivan Gagis <igagis@gmail.com>
 * @brief buffer abstract class and static buffer wrapper.
 */
 

#pragma once

#ifdef DEBUG
#include <iostream>
#endif

#include "types.hpp"
#include "debug.hpp"



namespace ting{



/**
 * @brief abstract buffer template class.
 * This class is supposed to be ancestor of all buffer-like objects.
 */
template <class T> class Buffer{
	
	//forbid copying
	inline Buffer(const Buffer&);
	
protected:
	T* buf;
	size_t size;


	/**
	 * @brief Default constructor.
	 * It is protected, so only accessible by subclasses.
     */
	inline Buffer(){}

	/**
	 * @brief Assignment operator.
	 * This operator implementation does nothing.
	 * This operator is defined because it should not be available for outside use,
	 * so make it protected. But it should be defined, because some subclass may have
	 * meaningful automatically generated operator=() (e.g. StaticBuffer)
	 * which subsequently will call this operator=().
     * @param - Buffer to assign from.
     * @return reference to this Buffer object.
     */
	inline Buffer& operator=(const Buffer&){
		//do nothing
		return *this;
	}
public:
	/**
	 * @brief Create a Buffer object.
	 * Creates a Buffer object which wraps given memory buffer of specified size.
	 * Note, the memory will not be freed upon this Buffer object destruction.
	 * @param bufPtr - pointer to the memory buffer.
	 * @param bufSize - size of the memory buffer.
	 */
	inline Buffer(T* bufPtr, size_t bufSize)throw() :
			buf(bufPtr),
			size(bufSize)
	{}



	/**
	 * @brief get buffer size.
	 * @return number of elements in buffer.
	 */
	inline size_t Size()const throw(){
		return this->size;
	}



	/**
	 * @brief get size of element.
	 * @return size of element in bytes.
	 */
	inline size_t SizeOfElem()const throw(){
		return sizeof(this->buf[0]);
	}



	/**
	 * @brief get size of buffer in bytes.
	 * @return size of array in bytes.
	 */
	inline size_t SizeInBytes()const throw(){
		return this->Size() * this->SizeOfElem();
	}



	/**
	 * @brief access specified element of the buffer.
	 * Const version of Buffer::operator[].
	 * @param i - element index.
	 * @return reference to i'th element of the buffer.
	 */
	inline const T& operator[](size_t i)const throw(){
		ASSERT(i < this->Size())
		return this->buf[i];
	}



	/**
	 * @brief access specified element of the buffer.
	 * @param i - element index.
	 * @return reference to i'th element of the buffer.
	 */
	inline T& operator[](size_t i)throw(){
		ASSERT_INFO(i < this->Size(), "operator[]: index out of bounds")
		return this->buf[i];
	}



	/**
	 * @brief get pointer to first element of the buffer.
	 * @return pointer to first element of the buffer.
	 */
	inline T* Begin()throw(){
		return this->buf;
	}



	/**
	 * @brief get pointer to first element of the buffer.
	 * @return pointer to first element of the buffer.
	 */
	inline const T* Begin()const throw(){
		return this->buf;
	}



	/**
	 * @brief get pointer to "after last" element of the buffer.
	 * @return pointer to "after last" element of the buffer.
	 */
	inline T* End()throw(){
		return this->buf + this->size;
	}



	/**
	 * @brief get const pointer to "after last" element of the buffer.
	 * @return const pointer to "after last" element of the buffer.
	 */
	inline const T* End()const throw(){
		return this->buf + this->size;
	}

	
	
	/**
	 * @brief Checks if pointer points somewhere within the buffer.
     * @param p - pointer to check.
     * @return true - if pointer passed as argument points somewhere within the buffer.
	 * @return false otherwise.
     */
	inline bool Overlaps(const T* p)const throw(){
		return this->Begin() <= p && p <= (this->End() - 1);
	}


#ifdef DEBUG
	friend std::ostream& operator<<(std::ostream& s, const Buffer<T>& buf){
		for(size_t i = 0; i < buf.Size(); ++i){
			s << "\t" << buf[i] << std::endl;
		}
		return s;
	}
#endif
};//~template class Buffer




/**
 * @brief static buffer class template.
 * The static buffer template.
 */
template <class T, size_t bufSize> class StaticBuffer : public ting::Buffer<T>{
	T staticBuffer[bufSize];
public:
	inline StaticBuffer() :
			ting::Buffer<T>(staticBuffer, bufSize)
	{}



	/**
	 * @brief Copy constructor.
	 * Performs a copy of a buffer, calling copy constructor on each element of the buffer.
	 * @param buf - static buffer to copy.
	 */
	inline StaticBuffer(const StaticBuffer<T, bufSize> &buf) :
			ting::Buffer<T>(staticBuffer, bufSize),
			staticBuffer(buf.staticBuffer)
	{}
};



}//~namespace
