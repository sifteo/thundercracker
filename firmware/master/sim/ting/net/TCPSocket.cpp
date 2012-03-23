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



#include "TCPSocket.hpp"

#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64

#elif M_OS == M_OS_LINUX || M_OS == M_OS_MACOSX || M_OS == M_OS_SOLARIS
	#include <netinet/in.h>

#else
	#error "Unsupported OS"
#endif



using namespace ting::net;



void TCPSocket::Open(const IPAddress& ip, bool disableNaggle){
	if(this->IsValid()){
		throw net::Exc("TCPSocket::Open(): socket already opened");
	}

	//create event for implementing Waitable
#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
	this->CreateEventForWaitable();
#endif

	this->socket = ::socket(AF_INET, SOCK_STREAM, 0);
	if(this->socket == DInvalidSocket()){
#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
		this->CloseEventForWaitable();
#endif
		throw net::Exc("TCPSocket::Open(): Couldn't create socket");
	}

	//Disable Naggle algorithm if required
	if(disableNaggle){
		this->DisableNaggle();
	}

	this->SetNonBlockingMode();

	this->ClearAllReadinessFlags();

	//Connecting to remote host
	sockaddr_in sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = htonl(ip.host);
	sockAddr.sin_port = htons(ip.port);

	// Connect to the remote host
	if(connect(
			this->socket,
			reinterpret_cast<sockaddr *>(&sockAddr),
			sizeof(sockAddr)
		) == DSocketError())
	{
#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
		int errorCode = WSAGetLastError();
#elif M_OS == M_OS_LINUX || M_OS == M_OS_MACOSX || M_OS == M_OS_SOLARIS
		int errorCode = errno;
#else
	#error "Unsupported OS"
#endif
		if(errorCode == DEIntr()){
			//do nothing, for non-blocking socket the connection request still should remain active
		}else if(errorCode == DEInProgress()){
			//do nothing, this is not an error, we have non-blocking socket
		}else{
			std::stringstream ss;
			ss << "TCPSocket::Open(): connect() failed, error code = " << errorCode << ": ";
#if M_COMPILER == M_COMPILER_MSVC
			{
				const size_t msgbufSize = 0xff;
				char msgbuf[msgbufSize];
				strerror_s(msgbuf, msgbufSize, errorCode);
				msgbuf[msgbufSize - 1] = 0;//make sure the string is null-terminated
				ss << msgbuf;
			}
#else
			ss << strerror(errorCode);
#endif
			this->Close();
			throw net::Exc(ss.str());
		}
	}
}



size_t TCPSocket::Send(const ting::Buffer<u8>& buf, size_t offset){
	if(!this->IsValid()){
		throw net::Exc("TCPSocket::Send(): socket is not opened");
	}

	this->ClearCanWriteFlag();

	ASSERT_INFO((
			(buf.Begin() + offset) <= (buf.End() - 1)) ||
			((buf.Size() == 0) && (offset == 0))
		,
			"buf.Begin() = " << reinterpret_cast<const void*>(buf.Begin())
			<< " offset = " << offset
			<< " buf.End() = " << reinterpret_cast<const void*>(buf.End())
		)

	ssize_t len;

	while(true){
		len = send(
				this->socket,
				reinterpret_cast<const char*>(buf.Begin() + offset),
				buf.Size() - offset,
				0
			);
		if(len == DSocketError()){
#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
			int errorCode = WSAGetLastError();
#else
			int errorCode = errno;
#endif
			if(errorCode == DEIntr()){
				continue;
			}else if(errorCode == DEAgain()){
				//can't send more bytes, return 0 bytes sent
				len = 0;
			}else{
				std::stringstream ss;
				ss << "TCPSocket::Send(): send() failed, error code = " << errorCode << ": ";
#if M_COMPILER == M_COMPILER_MSVC
				{
					const size_t msgbufSize = 0xff;
					char msgbuf[msgbufSize];
					strerror_s(msgbuf, msgbufSize, errorCode);
					msgbuf[msgbufSize - 1] = 0;//make sure the string is null-terminated
					ss << msgbuf;
				}
#else
				ss << strerror(errorCode);
#endif
				throw net::Exc(ss.str());
			}
		}
		break;
	}//~while

	ASSERT(len >= 0)
	return size_t(len);
}



size_t TCPSocket::Recv(ting::Buffer<u8>& buf, size_t offset){
	//the 'can read' flag shall be cleared even if this function fails to avoid subsequent
	//calls to Recv() because it indicates that there's activity.
	//So, do it at the beginning of the function.
	this->ClearCanReadFlag();

	if(!this->IsValid()){
		throw net::Exc("TCPSocket::Send(): socket is not opened");
	}

	ASSERT_INFO(
			((buf.Begin() + offset) <= (buf.End() - 1)) ||
			((buf.Size() == 0) && (offset == 0))
		,
			"buf.Begin() = " << reinterpret_cast<void*>(buf.Begin())
			<< " offset = " << offset
			<< " buf.End() = " << reinterpret_cast<void*>(buf.End())
		)

	ssize_t len;

	while(true){
		len = recv(
				this->socket,
				reinterpret_cast<char*>(buf.Begin() + offset),
				buf.Size() - offset,
				0
			);
		if(len == DSocketError()){
#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
			int errorCode = WSAGetLastError();
#else
			int errorCode = errno;
#endif

			if(errorCode == DEIntr()){
				continue;
			}else if(errorCode == DEAgain()){
				//no data available, return 0 bytes received
				len = 0;
			}else{
				std::stringstream ss;
				ss << "TCPSocket::Recv(): recv() failed, error code = " << errorCode << ": ";
#if M_COMPILER == M_COMPILER_MSVC
				{
					const size_t msgbufSize = 0xff;
					char msgbuf[msgbufSize];
					strerror_s(msgbuf, msgbufSize, errorCode);
					msgbuf[msgbufSize - 1] = 0;//make sure the string is null-terminated
					ss << msgbuf;
				}
#else
				ss << strerror(errorCode);
#endif
				throw net::Exc(ss.str());
			}
		}
		break;
	}//~while

	ASSERT(len >= 0)
	return size_t(len);
}



IPAddress TCPSocket::GetLocalAddress(){
	if(!this->IsValid()){
		throw net::Exc("Socket::GetLocalPort(): socket is not valid");
	}

	sockaddr_in addr;

#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
	int len = sizeof(addr);
#else
	socklen_t len = sizeof(addr);
#endif

	if(getsockname(
			this->socket,
			reinterpret_cast<sockaddr*>(&addr),
			&len
		) < 0)
	{
		throw net::Exc("Socket::GetLocalPort(): getsockname() failed");
	}

	return IPAddress(
			u32(ntohl(addr.sin_addr.s_addr)),
			u16(ntohs(addr.sin_port))
		);
}



IPAddress TCPSocket::GetRemoteAddress(){
	if(!this->IsValid()){
		throw net::Exc("TCPSocket::GetRemoteAddress(): socket is not valid");
	}

	sockaddr_in addr;

#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
	int len = sizeof(addr);
#else
	socklen_t len = sizeof(addr);
#endif

	if(getpeername(
			this->socket,
			reinterpret_cast<sockaddr*>(&addr),
			&len
		) < 0)
	{
		throw net::Exc("TCPSocket::GetRemoteAddress(): getpeername() failed");
	}

	return IPAddress(
			u32(ntohl(addr.sin_addr.s_addr)),
			u16(ntohs(addr.sin_port))
		);
}



#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
//override
void TCPSocket::SetWaitingEvents(u32 flagsToWaitFor){
	long flags = FD_CLOSE;
	if((flagsToWaitFor & Waitable::READ) != 0){
		flags |= FD_READ;
		//NOTE: since it is not a TCPServerSocket, FD_ACCEPT is not needed here.
	}
	if((flagsToWaitFor & Waitable::WRITE) != 0){
		flags |= FD_WRITE | FD_CONNECT;
	}
	this->SetWaitingEventsForWindows(flags);
}
#endif
