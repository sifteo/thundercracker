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



#include "Socket.hpp"



#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64


#elif M_OS == M_OS_LINUX || M_OS == M_OS_MACOSX || M_OS == M_OS_SOLARIS
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <fcntl.h>

#else
	#error "Unsupported OS"
#endif



using namespace ting::net;



Socket::Socket(const Socket& s) :
		ting::Waitable(),
		//NOTE: operator=() will call Close, so the socket should be in invalid state!!!
		//Therefore, initialize variables to invalid values.
#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
		eventForWaitable(WSA_INVALID_EVENT),
#endif
		socket(DInvalidSocket())
{
//		TRACE(<< "Socket::Socket(copy): invoked " << this << std::endl)
	this->operator=(s);
}



Socket::~Socket()throw(){
	this->Close();
}



void Socket::Close()throw(){
//		TRACE(<< "Socket::Close(): invoked " << this << std::endl)
	ASSERT_INFO(!this->IsAdded(), "Socket::Close(): trying to close socket which is added to the WaitSet. Remove the socket from WaitSet before closing.")
	
	if(this->IsValid()){
		ASSERT(!this->IsAdded()) //make sure the socket is not added to WaitSet

#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
		//Closing socket in Win32.
		//refer to http://tangentsoft.net/wskfaq/newbie.html#howclose for details
		shutdown(this->socket, SD_BOTH);
		closesocket(this->socket);

		this->CloseEventForWaitable();
#elif M_OS == M_OS_LINUX || M_OS == M_OS_MACOSX || M_OS == M_OS_SOLARIS
		close(this->socket);
#else
	#error "Unsupported OS"
#endif
	}
	this->ClearAllReadinessFlags();
	this->socket = DInvalidSocket();
}



//same as std::auto_ptr
Socket& Socket::operator=(const Socket& s){
//	TRACE(<< "Socket::operator=(): invoked " << this << std::endl)
	if(this == &s){//detect self-assignment
		return *this;
	}

	//first, assign as Waitable, it may throw an exception
	//if the waitable is added to some waitset
	this->Waitable::operator=(s);

	this->Close();
	this->socket = s.socket;

#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
	this->eventForWaitable = s.eventForWaitable;
	const_cast<Socket&>(s).eventForWaitable = WSA_INVALID_EVENT;
#endif

	const_cast<Socket&>(s).socket = DInvalidSocket();
	return *this;
}



void Socket::DisableNaggle(){
	if(!this->IsValid()){
		throw net::Exc("Socket::DisableNaggle(): socket is not valid");
	}

#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64 || M_OS == M_OS_LINUX || M_OS == M_OS_MACOSX || M_OS == M_OS_SOLARIS
	{
		int yes = 1;
		setsockopt(this->socket, IPPROTO_TCP, TCP_NODELAY, (char*)&yes, sizeof(yes));
	}
#else
	#error "Unsupported OS"
#endif
}



void Socket::SetNonBlockingMode(){
	if(!this->IsValid()){
		throw net::Exc("Socket::SetNonBlockingMode(): socket is not valid");
	}

#if  M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
	{
		u_long mode = 1;
		if(ioctlsocket(this->socket, FIONBIO, &mode) != 0){
			throw net::Exc("Socket::SetNonBlockingMode(): ioctlsocket(FIONBIO) failed");
		}
	}
	
#elif M_OS == M_OS_LINUX || M_OS == M_OS_MACOSX || M_OS == M_OS_SOLARIS
	{
		int flags = fcntl(this->socket, F_GETFL, 0);
		if(flags == -1){
			throw net::Exc("Socket::SetNonBlockingMode(): fcntl(F_GETFL) failed");
		}
		if(fcntl(this->socket, F_SETFL, flags | O_NONBLOCK) != 0){
			throw net::Exc("Socket::SetNonBlockingMode(): fcntl(F_SETFL) failed");
		}
	}
#else
	#error "Unsupported OS"
#endif
}



ting::u16 Socket::GetLocalPort(){
	if(!this->IsValid()){
		throw net::Exc("Socket::GetLocalPort(): socket is not valid");
	}

	sockaddr_in addr;

#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
	int len = sizeof(addr);
#elif M_OS == M_OS_LINUX || M_OS == M_OS_MACOSX || M_OS == M_OS_SOLARIS
	socklen_t len = sizeof(addr);
#else
	#error "Unsupported OS"
#endif

	if(getsockname(
			this->socket,
			reinterpret_cast<sockaddr*>(&addr),
			&len
		) < 0)
	{
		throw net::Exc("Socket::GetLocalPort(): getsockname() failed");
	}

	return ting::u16(ntohs(addr.sin_port));
}



#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64

//override
HANDLE Socket::GetHandle(){
	//return event handle
	return this->eventForWaitable;
}



//override
bool Socket::CheckSignalled(){
	WSANETWORKEVENTS events;
	memset(&events, 0, sizeof(events));
	ASSERT(this->IsValid())
	if(WSAEnumNetworkEvents(this->socket, this->eventForWaitable, &events) != 0){
		throw net::Exc("Socket::CheckSignalled(): WSAEnumNetworkEvents() failed");
	}

	//NOTE: sometimes no events are reported, don't know why.
//		ASSERT(events.lNetworkEvents != 0)

	if((events.lNetworkEvents & FD_CLOSE) != 0){
		this->SetErrorFlag();
	}

	if((events.lNetworkEvents & FD_READ) != 0){
		this->SetCanReadFlag();
		if(events.iErrorCode[ASSCOND(FD_READ_BIT, < FD_MAX_EVENTS)] != 0){
			this->SetErrorFlag();
		}
	}

	if((events.lNetworkEvents & FD_ACCEPT) != 0){
		this->SetCanReadFlag();
		if(events.iErrorCode[ASSCOND(FD_ACCEPT_BIT, < FD_MAX_EVENTS)] != 0){
			this->SetErrorFlag();
		}
	}

	if((events.lNetworkEvents & FD_WRITE) != 0){
		this->SetCanWriteFlag();
		if(events.iErrorCode[ASSCOND(FD_WRITE_BIT, < FD_MAX_EVENTS)] != 0){
			this->SetErrorFlag();
		}
	}

	if((events.lNetworkEvents & FD_CONNECT) != 0){
		this->SetCanWriteFlag();
		if(events.iErrorCode[ASSCOND(FD_CONNECT_BIT, < FD_MAX_EVENTS)] != 0){
			this->SetErrorFlag();
		}
	}

#ifdef DEBUG
	//if some event occurred then some of readiness flags should be set
	if(events.lNetworkEvents != 0){
		ASSERT_ALWAYS(this->readinessFlags != 0)
	}
#endif

	return this->Waitable::CheckSignalled();
}



void Socket::CreateEventForWaitable(){
	ASSERT(this->eventForWaitable == WSA_INVALID_EVENT)
	this->eventForWaitable = WSACreateEvent();
	if(this->eventForWaitable == WSA_INVALID_EVENT){
		throw net::Exc("Socket::CreateEventForWaitable(): could not create event (Win32) for implementing Waitable");
	}
}



void Socket::CloseEventForWaitable(){
	ASSERT(this->eventForWaitable != WSA_INVALID_EVENT)
	WSACloseEvent(this->eventForWaitable);
	this->eventForWaitable = WSA_INVALID_EVENT;
}



void Socket::SetWaitingEventsForWindows(long flags){
	ASSERT_INFO(this->IsValid() && (this->eventForWaitable != WSA_INVALID_EVENT), "HINT: Most probably, you are trying to remove the _closed_ socket from WaitSet. If so, you should first remove the socket from WaitSet and only then call the Close() method.")

	if(WSAEventSelect(
			this->socket,
			this->eventForWaitable,
			flags
		) != 0)
	{
		throw net::Exc("Socket::SetWaitingEventsForWindows(): could not associate event (Win32) with socket");
	}
}



#elif M_OS == M_OS_LINUX || M_OS == M_OS_MACOSX || M_OS == M_OS_SOLARIS

//override
int Socket::GetHandle(){
	return this->socket;
}

#else
	#error "unsupported OS"
#endif
