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



#include "../config.hpp"

#include "Lib.hpp"
#include "Exc.hpp"
#include "HostNameResolver.hpp"


#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
	#include <winsock2.h>
	#include <windows.h>

#elif M_OS == M_OS_LINUX || M_OS == M_OS_SOLARIS || M_OS == M_OS_MACOSX
	#include <signal.h>

#else
	#error "Unsupported OS"
#endif



using namespace ting::net;



ting::IntrusiveSingleton<Lib>::T_Instance Lib::instance;



Lib::Lib(){
#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
	WORD versionWanted = MAKEWORD(2,2);
	WSADATA wsaData;
	if(WSAStartup(versionWanted, &wsaData) != 0 ){
		throw net::Exc("SocketLib::SocketLib(): Winsock 2.2 initialization failed");
	}
#elif M_OS == M_OS_LINUX || M_OS == M_OS_SOLARIS || M_OS == M_OS_MACOSX
	// SIGPIPE is generated when a remote socket is closed
	void (*handler)(int);
	handler = signal(SIGPIPE, SIG_IGN);
	if(handler != SIG_DFL){
		signal(SIGPIPE, handler);
	}
#else
	#error "Unknown OS"
#endif
}



Lib::~Lib()throw(){
	//check that there are no active dns lookups and finish the DNS request thread
	HostNameResolver::CleanUp();
	
#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
	// Clean up windows networking
	if(WSACleanup() == SOCKET_ERROR){
		if(WSAGetLastError() == WSAEINPROGRESS){
			WSACancelBlockingCall();
			WSACleanup();
		}
	}
#elif M_OS == M_OS_LINUX || M_OS == M_OS_SOLARIS || M_OS == M_OS_MACOSX
	// Restore the SIGPIPE handler
	void (*handler)(int);
	handler = signal(SIGPIPE, SIG_DFL);
	if(handler != SIG_IGN){
		signal(SIGPIPE, handler);
	}
#else
	#error "Unknown OS"
#endif
}
