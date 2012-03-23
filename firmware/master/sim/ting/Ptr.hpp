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
 * @file Ptr.hpp
 * @author Ivan Gagis <igagis@gmail.com>
 * @brief Pointer wrapper.
 */

#pragma once

#include "debug.hpp"

//#define M_ENABLE_PTR_PRINT
#ifdef M_ENABLE_PTR_PRINT
#define M_PTR_PRINT(x) TRACE(x)
#else
#define M_PTR_PRINT(x)
#endif

namespace ting{


/**
 * @brief Auto-pointer template class.
 * Auto-pointer class is a wrapper above ordinary pointer.
 * It holds a pointer to an object and it will 'delete'
 * that object when pointer goes out of scope.
 */
template <class T> class Ptr{
	template <class TS> friend class Ptr;

	T* p;
public:
	explicit inline Ptr(T* ptr = 0)throw() :
			p(ptr)
	{}

	/**
	 * @brief Copy constructor.
	 * Creates a copy of 'ptr' and invalidates it.
	 * This means that if creating Ptr object like this:
	 *     Ptr<SomeClass> a(new SomeClass());//create pointer 'a'
	 *     Ptr<SomeClass> b(a);//create pointer 'b' using copy constructor
	 * then 'a' will become invalid while 'b' will hold pointer to the object
	 * of class 'SomeClass' which 'a' was holding before.
	 * I.e. when using copy constructor, no memory allocation occurs,
	 * object kept by 'a' is moved to 'b' and 'a' is invalidated.
	 * @param ptr - pointer to copy.
	 */
	//const copy constructor
	inline Ptr(const Ptr& ptr)throw(){
		M_PTR_PRINT(<< "Ptr::Ptr(copy): invoked, ptr.p = " << (ptr.p) << std::endl)
		this->p = ptr.p;
		const_cast<Ptr&>(ptr).p = 0;
	}

	template <class TS> inline Ptr(const Ptr<TS>& ptr)throw(){
		M_PTR_PRINT(<< "Ptr::Ptr(conversion): invoked, ptr.p = " << (ptr.p) << std::endl)
		this->p = ptr.p;
		const_cast<Ptr<TS>&>(ptr).p = 0;
	}

	inline ~Ptr()throw(){
		this->Destroy();
	}

	inline T* operator->()throw(){
		ASSERT_INFO(this->p, "Ptr::operator->(): this->p is zero")
		return this->p;
	}

	inline T* operator->()const throw(){
		ASSERT_INFO(this->p, "const Ptr::operator->(): this->p is zero")
		return this->p;
	}

	inline T& operator*()throw(){
		ASSERT_INFO(this->p, "Ptr::operator*(): this->p is zero")
		return *(this->operator->());
	}

	inline T& operator*()const throw(){
		ASSERT_INFO(this->p, "const Ptr::operator*(): this->p is zero")
		return *(this->operator->());
	}

	/**
	 * @brief Assignment operator.
	 * This operator works the same way as copy constructor does.
	 * That is, if assignng like this:
	 *     Ptr<SomeClass> b(new SomeClass()), a(new SomeClass());
	 *     b = a;
	 * then 'a' will become invalid and 'b' will hold the object owned by 'a' before.
	 * Note, that object owned by 'b' prior to assignment is deleted.
	 * Thus, no memory leak occurs.
	 * @param ptr - pointer to assign from.
	 */
	inline Ptr& operator=(const Ptr& ptr)throw(){
		M_PTR_PRINT(<< "Ptr::operator=(Ptr&): enter, this->p = " << (this->p) << std::endl)
		this->Destroy();
		this->p = ptr.p;
		const_cast<Ptr&>(ptr).p = 0;
		M_PTR_PRINT(<< "Ptr::operator=(Ptr&): exit, this->p = " << (this->p) << std::endl)
		return (*this);
	}

	template <class TS> inline Ptr& operator=(const Ptr<TS>& ptr)throw(){
		M_PTR_PRINT(<< "Ptr::operator=(conversion): enter, this->p = " << (this->p) << std::endl)
		this->Destroy();
		this->p = ptr.p;
		const_cast<Ptr<TS>&>(ptr).p = 0;
		M_PTR_PRINT(<< "Ptr::operator=(conversion): exit, this->p = " << (this->p) << std::endl)
		return (*this);
	}

	inline bool operator==(const T* ptr)const throw(){
		return this->p == ptr;
	}

	inline bool operator!=(const T* ptr)const throw(){
		return !this->operator==(ptr);
	}

	inline bool operator!()const throw(){
		return this->IsNotValid();
	}



	//Safe conversion to bool type.
	//Because if using simple "operator bool()" it may result in chained automatic
	//conversion to undesired types such as int.
	typedef void (Ptr::*unspecified_bool_type)();
	inline operator unspecified_bool_type()const throw(){
		return this->IsValid() ? &Ptr::Reset : 0; //Ptr::Reset is taken just because it has matching signature
	}

//	inline operator bool(){
//		return this->IsValid();
//	}



	/**
	 * @brief Extract pointer to object invalidating the Ptr.
	 * Extract the pointer to object from this Ptr instance and invalidate
	 * the Ptr instance. After that, when this Ptr instance goes out of scope
	 * the object will not be deleted because Ptr instance is already invalid
	 * at this point.
	 * @return pointer to object previously owned by that Ptr instance.
	 */
	inline T* Extract()throw(){
		T* pp = this->p;
		this->p = 0;
		return pp;
	}

	/**
	 * @brief reset pointer, destroying object it point to.
	 * This will destroy the object this pointer points to if any.
	 * After that the pointer becomes invalid.
	 */
	inline void Reset()throw(){
		this->Destroy();
		this->p = 0;
	}

	/**
	 * @brief tells if the pointer is valid or not.
	 * @return true if pointer is valid and holding some object.
	 * @return false otherwise.
	 */
	inline bool IsValid()const throw(){
		return this->p != 0;
	}

	/**
	 * @brief tells if the pointer is valid or not.
	 * @return false if object is valid.
	 * @return true otherwise.
	 */
	inline bool IsNotValid()const throw(){
		return !this->IsValid();
	}

	/**
	 * @brief Static cast.
	 * NOTE: use this method very carefully!!! It returns ordinary pointer
	 * to the object while the object itself is still owned by Ptr.
	 * Do not create other Ptr instances using that returned value!!! As it
	 * will cause double 'delete' when both Ptr instances go out of scope.
	 * @return pointer to casted class.
	 */
	template <class TS> inline TS* StaticCast()throw(){
		return static_cast<TS*>(this->operator->());
	}

private:
	inline void Destroy()throw(){
		M_PTR_PRINT(<< "Ptr::~Ptr(): delete invoked, this->p = " << this->p << std::endl)
		delete this->p;
	}

	void* operator new(size_t);

	void operator delete(void*);
};

}//~namespace ting
