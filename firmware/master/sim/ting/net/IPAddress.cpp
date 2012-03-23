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



#include "IPAddress.hpp"



using namespace ting::net;



namespace{

//This function modifies the pointer passed as argument (reference to a pointer).
//After successful completion of the function the pointer passed as argument
//points to the character which goes right after the IP address.
ting::u32 ParseIPAddressString(const char*& p){
	ting::u32 h = 0;//parsed host

	for(unsigned t = 0; t < 4; ++t){
		unsigned digits[3];
		unsigned numDgts;
		for(numDgts = 0; numDgts < 3; ++numDgts, ++p){
			if(*p < '0' || '9' < *p){
				if(numDgts == 0){
					throw ting::net::IPAddress::BadIPAddressFormatExc();
				}
				break;
			}
			digits[numDgts] = unsigned(*p) - unsigned('0');
		}

		if(t < 3){
			if(*p != '.'){//unexpected non-delimiter character
				throw ting::net::IPAddress::BadIPAddressFormatExc();
			}
			++p;
		}else{
			ASSERT(t == 3)
		}

		unsigned xxx = 0;
		for(unsigned i = 0; i < numDgts; ++i){
			unsigned ord = 1;
			for(unsigned j = 1; j < numDgts - i; ++j){
			   ord *= 10;
			}
			xxx += digits[i] * ord;
		}
		if(xxx > 0xff){
			throw ting::net::IPAddress::BadIPAddressFormatExc();
		}

		h |= (xxx << (8 * (3 - t)));
	}
	
	return h;
}

}//~namespace



IPAddress::IPAddress(const char* ip, u16 p) :
		host(ParseIPAddressString(ip)),
		port(p)
{}



IPAddress::IPAddress(const char* ip) :
		host(ParseIPAddressString(ip))
{
	if(*ip != ':'){
//		TRACE(<< "no colon, *ip = " << (*ip) << std::endl)
		throw ting::net::IPAddress::BadIPAddressFormatExc();
	}
	
	++ip;
	
	//move to the end of port number, maximum 5 digits.
	for(unsigned i = 0; '0' <= *ip && *ip <= '9' && i < 5; ++i, ++ip){
	}
	if('0' <= *ip && *ip <= '9'){//if still have one more digit
//		TRACE(<< "still have one more digit" << std::endl)
		throw ting::net::IPAddress::BadIPAddressFormatExc();
	}
	
	--ip;
	
	ting::u32 port = 0;
	
	for(unsigned i = 0; *ip != ':'; ++i, --ip){
		ting::u32 pow = 1;
		for(unsigned j = 0; j < i; ++j){
			pow *= 10;
		}
	
		ASSERT('0' <= *ip && *ip <= '9')
		
		port += (*ip - '0') * pow;
	}
	
	if(port > 0xffff){
//		TRACE(<< "port number is bigger than 0xffff" << std::endl)
		throw ting::net::IPAddress::BadIPAddressFormatExc();
	}
	
	this->port = ting::u16(port);
}
