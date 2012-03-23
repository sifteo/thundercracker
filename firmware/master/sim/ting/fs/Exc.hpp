/* The MIT License:

Copyright (c) 2009-2012 Ivan Gagis

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
 * @file File abstract interface
 * @author Ivan Gagis <igagis@gmail.com>
 */

#pragma once

#include "../Exc.hpp"



namespace ting{
namespace fs{



/**
 * @brief Basic exception class.
 */
class Exc : public ting::Exc{
public:
	/**
	 * @brief Constructor.
	 * @param descr - human readable description of the error.
	 */
	Exc(const std::string& descr) :
			ting::Exc(std::string("[fs::Exc]: ") + descr)
	{}
};



}//~namespace
}//~namespace
