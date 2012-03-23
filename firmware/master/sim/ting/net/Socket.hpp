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
 */

#pragma once


#include <string>
#include <sstream>

#include "../config.hpp"


#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
	#include <winsock2.h>
	#include <windows.h>

#elif M_OS == M_OS_LINUX || M_OS == M_OS_MACOSX || M_OS == M_OS_SOLARIS
	#include <sys/socket.h>

#else
	#error "Unsupported OS"
#endif



#include "Exc.hpp"
#include "../WaitSet.hpp"
#include "../debug.hpp"



/**
 * @brief the main namespace of ting library.
 * All the declarations of ting library are made inside this namespace.
 */
namespace ting{
namespace net{



/**
 * @brief Basic socket class.
 * This is a base class for all socket types such as TCP sockets or UDP sockets.
 */
class Socket : public Waitable{
protected:
#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
	typedef SOCKET T_Socket;

	inline static T_Socket DInvalidSocket(){
		return INVALID_SOCKET;
	}

	inline static int DSocketError(){
		return SOCKET_ERROR;
	}

	inline static int DEIntr(){
		return WSAEINTR;
	}
	
	inline static int DEAgain(){
		return WSAEWOULDBLOCK;
	}

	inline static int DEInProgress(){
		return WSAEWOULDBLOCK;
	}

#elif M_OS == M_OS_LINUX || M_OS == M_OS_MACOSX || M_OS == M_OS_SOLARIS
	typedef int T_Socket;

	inline static T_Socket DInvalidSocket(){
		return -1;
	}

	inline static T_Socket DSocketError(){
		return -1;
	}

	inline static int DEIntr(){
		return EINTR;
	}

	inline static int DEAgain(){
		return EAGAIN;
	}

	inline static int DEInProgress(){
		return EINPROGRESS;
	}
#else
	#error "Unsupported OS"
#endif

#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
	WSAEVENT eventForWaitable;
#endif

	T_Socket socket;

	Socket() :
#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
			eventForWaitable(WSA_INVALID_EVENT),
#endif
			socket(DInvalidSocket())
	{
//		TRACE(<< "Socket::Socket(): invoked " << this << std::endl)
	}



	//same as std::auto_ptr
	Socket& operator=(const Socket& s);



	void DisableNaggle();



	void SetNonBlockingMode();



public:
	Socket(const Socket& s);

	
	
	virtual ~Socket()throw();



	/**
	 * @brief Tells whether the socket is opened or not.
	 * @return Returns true if the socket is opened or false otherwise.
	 */
	inline bool IsValid()const{
		return this->socket != DInvalidSocket();
	}



	/**
	 * @brief Tells whether the socket is opened or not.
	 * @return inverse of IsValid().
	 */
	inline bool IsNotValid()const{
		return !this->IsValid();
	}



	/**
	 * @brief Closes the socket disconnecting it if necessary.
	 */
	void Close()throw();



	/**
	 * @brief Returns local port this socket is bound to.
	 * @return local port number to which this socket is bound,
	 *         0 means that the socket is not bound to a port.
	 */
	u16 GetLocalPort();



#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
private:
	//override
	HANDLE GetHandle();

	//override
	bool CheckSignalled();

protected:
	void CreateEventForWaitable();

	void CloseEventForWaitable();

	void SetWaitingEventsForWindows(long flags);



#elif M_OS == M_OS_LINUX || M_OS == M_OS_MACOSX || M_OS == M_OS_SOLARIS
private:
	//override
	int GetHandle();
#else
	#error "Unsupported OS"
#endif
};//~class Socket



}//~namespace
}//~namespace
