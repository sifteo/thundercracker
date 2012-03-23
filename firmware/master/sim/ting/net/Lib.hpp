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
 * @brief Socket network library singleton.
 */

#pragma once



#include "../Singleton.hpp"



namespace ting{

/**
 * @brief Namespace where all network-related classes are defined.
 */
namespace net{



/**
 * @brief Socket library singleton class.
 * This is a Socket library singleton class. Creating an object of this class initializes the library
 * while destroying this object de-initializes it. So, the convenient way of initializing the library
 * is to create an object of this class on the stack. Thus, when the object goes out of scope its
 * destructor will be called and the library will be de-initialized automatically.
 * This is what C++ RAII is all about.
 */
class Lib : public IntrusiveSingleton<Lib>{
	friend class IntrusiveSingleton<Lib>;
	static IntrusiveSingleton<Lib>::T_Instance instance;
	
public:
	Lib();

	~Lib()throw();
};



}//~namespace
}//~namespace


/*
 * @mainpage ting::Socket library
 *
 * @section sec_about About
 * <b>tin::Socket</b> is a simple cross platform C++ wrapper above sockets networking API designed for games.
 *
 * @section sec_getting_started Getting started
 * @ref page_usage_tutorial "library usage tutorial" - quick start tutorial
 */

/*
 * @page page_usage_tutorial ting::Socket usage tutorial
 *
 * TODO: write usage tutorial
 */
