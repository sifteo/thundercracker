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
 * @file Singleton.hpp
 * @author Ivan Gagis <igagis@gmail.com>
 * @brief Singleton base class (template).
 */

#pragma once

#include "Exc.hpp"
#include "debug.hpp"
#include "types.hpp"



namespace ting{



/**
 * @brief Intrusive singleton base class.
 * This is a basic intrusive singleton template.
 * Template params: T - your singletone class type, T_InstanceOwner - class which owns the static 'instance' variable.
 * In most cases T_InstanceOwner is the same as T.
 * Usage as follows:
 * @code
 *	class MySingleton : public ting::IntrusiveSingleton<MySingleton, MySingleton>{
 *		friend class ting::IntrusiveSingleton<MySingleton, MySingleton>;
 *		static ting::IntrusiveSingleton<MySingleton, MySingleton>::T_Instance instance;
 * 
 *	public:
 *		void DoSomething(){
 *			//...
 *		}
 *  };
 * 
 *	//define the static variable somewhere in .cpp file.
 *  ting::IntrusiveSingleton<MySingleton, MySingleton>::T_Instance MySingleton::instance;
 *
 *	int main(int, char**){
 *		MySingleton mySingleton;
 *
 *		MySingleton::Inst().DoSomething();
 *	}
 * @endcode
 */
template <class T, class T_InstanceOwner = T> class IntrusiveSingleton{

protected://use only as a base class
	IntrusiveSingleton(){
		if(T_InstanceOwner::instance != 0){
			throw ting::Exc("Singleton::Singleton(): instance is already created");
		}

		T_InstanceOwner::instance = static_cast<T*>(this);
	}

	typedef ting::Inited<T*, 0> T_Instance;
	
private:

	//copying is not allowed
	IntrusiveSingleton(const IntrusiveSingleton&);
	IntrusiveSingleton& operator=(const IntrusiveSingleton&);
	
public:
	
	/**
	 * @brief tells if singleton object is created or not.
	 * Note, this function is not thread safe.
	 * @return true if object is created.
	 * @return false otherwise.
	 */
	inline static bool IsCreated(){
		return T_InstanceOwner::instance != 0;
	}

	/**
	 * @brief get singleton instance.
	 * @return reference to singleton object instance.
	 */
	inline static T& Inst(){
		ASSERT_INFO(IsCreated(), "IntrusiveSingleton::Inst(): Singleton object is not created")
		return *T_InstanceOwner::instance;
	}

	~IntrusiveSingleton(){
		ASSERT(T_InstanceOwner::instance == static_cast<T*>(this))
		T_InstanceOwner::instance = 0;
	}
};



/**
 * @brief Singleton base class.
 * This is a basic non-intrusive singleton template.
 * Note, that Singleton inherits the IntrusiveSingleton class, so it inherits all
 * its static methods, the most important one is Inst().
 * Usage as follows:
 * @code
 *	class MySingleton : public ting::Singleton<MySingleton>{
 *	public:
 *		void DoSomething(){
 *			//...
 *		}
 *  };
 *
 *	int main(int, char**){
 *		MySingleton mySingleton;
 *
 *		MySingleton::Inst().DoSomething();
 *	}
 * @endcode
 */
template <class T> class Singleton : public IntrusiveSingleton<T, Singleton<T> >{
	friend class IntrusiveSingleton<T, Singleton<T> >;
public:
	inline Singleton(){}
	
private:

	//copying is not allowed
	Singleton(const Singleton&);
	Singleton& operator=(const Singleton&);

private:
	
	static typename IntrusiveSingleton<T, Singleton<T> >::T_Instance instance;
};

template <class T> typename ting::IntrusiveSingleton<T, Singleton<T> >::T_Instance ting::Singleton<T>::instance;

}//~namespace ting
