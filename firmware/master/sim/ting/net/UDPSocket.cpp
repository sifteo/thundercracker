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



#include "UDPSocket.hpp"

#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64

#elif M_OS == M_OS_LINUX || M_OS == M_OS_MACOSX || M_OS == M_OS_SOLARIS
	#include <netinet/in.h>

#else
	#error "Unsupported OS"
#endif


using namespace ting::net;



void UDPSocket::Open(u16 port){
	if(this->IsValid()){
		throw net::Exc("UDPSocket::Open(): the socket is already opened");
	}

#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
	this->CreateEventForWaitable();
#endif

	this->socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if(this->socket == DInvalidSocket()){
#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
		this->CloseEventForWaitable();
#endif
		throw net::Exc("UDPSocket::Open(): ::socket() failed");
	}

	//Bind locally, if appropriate
	if(port != 0){
		struct sockaddr_in sockAddr;
		memset(&sockAddr, 0, sizeof(sockAddr));
		sockAddr.sin_family = AF_INET;
		sockAddr.sin_addr.s_addr = INADDR_ANY;
		sockAddr.sin_port = htons(port);

		// Bind the socket for listening
		if(::bind(
				this->socket,
				reinterpret_cast<struct sockaddr*>(&sockAddr),
				sizeof(sockAddr)
			) == DSocketError())
		{
			this->Close();
			throw net::Exc("UDPSocket::Open(): could not bind to local port");
		}
	}

	this->SetNonBlockingMode();

	//Allow broadcasting
#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64 || M_OS == M_OS_LINUX || M_OS == M_OS_MACOSX || M_OS == M_OS_SOLARIS
	{
		int yes = 1;
		if(setsockopt(
				this->socket,
				SOL_SOCKET,
				SO_BROADCAST,
				reinterpret_cast<char*>(&yes),
				sizeof(yes)
			) == DSocketError())
		{
			this->Close();
			throw net::Exc("UDPSocket::Open(): failed setting broadcast option");
		}
	}
#else
	#error "Unsupported OS"
#endif

	this->ClearAllReadinessFlags();
}



size_t UDPSocket::Send(const ting::Buffer<u8>& buf, const IPAddress& destinationIP){
	if(!this->IsValid()){
		throw net::Exc("UDPSocket::Send(): socket is not opened");
	}

	this->ClearCanWriteFlag();

	sockaddr_in sockAddr;
	int sockLen = sizeof(sockAddr);

	sockAddr.sin_addr.s_addr = htonl(destinationIP.host);
	sockAddr.sin_port = htons(destinationIP.port);
	sockAddr.sin_family = AF_INET;


	ssize_t len;

	while(true){
		len = ::sendto(
				this->socket,
				reinterpret_cast<const char*>(buf.Begin()),
				buf.Size(),
				0,
				reinterpret_cast<struct sockaddr*>(&sockAddr),
				sockLen
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
				ss << "UDPSocket::Send(): sendto() failed, error code = " << errorCode << ": ";
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

	ASSERT(buf.Size() <= size_t(ting::DMaxInt))
	ASSERT_INFO(len <= int(buf.Size()), "res = " << len)
	ASSERT_INFO((len == int(buf.Size())) || (len == 0), "res = " << len)

	ASSERT(len >= 0)
	return size_t(len);
}



size_t UDPSocket::Recv(ting::Buffer<u8>& buf, IPAddress &out_SenderIP){
	if(!this->IsValid()){
		throw net::Exc("UDPSocket::Recv(): socket is not opened");
	}

	//The "can read" flag shall be cleared even if this function fails.
	//This is to avoid subsequent calls to Recv() because of it indicating
	//that there's an activity.
	//So, do it at the beginning of the function.
	this->ClearCanReadFlag();

	sockaddr_in sockAddr;

#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
	int sockLen = sizeof(sockAddr);
#elif M_OS == M_OS_LINUX || M_OS == M_OS_MACOSX || M_OS == M_OS_SOLARIS
	socklen_t sockLen = sizeof(sockAddr);
#else
	#error "Unsupported OS"
#endif

	ssize_t len;

	while(true){
		len = ::recvfrom(
				this->socket,
				reinterpret_cast<char*>(buf.Begin()),
				buf.Size(),
				0,
				reinterpret_cast<sockaddr*>(&sockAddr),
				&sockLen
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
				ss << "UDPSocket::Recv(): recvfrom() failed, error code = " << errorCode << ": ";
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

	ASSERT(buf.Size() <= size_t(ting::DMaxInt))
	ASSERT_INFO(len <= int(buf.Size()), "len = " << len)

	out_SenderIP.host = ntohl(sockAddr.sin_addr.s_addr);
	out_SenderIP.port = ntohs(sockAddr.sin_port);
	
	ASSERT(len >= 0)
	return size_t(len);
}



#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
//override
void UDPSocket::SetWaitingEvents(u32 flagsToWaitFor){
	long flags = FD_CLOSE;
	if((flagsToWaitFor & Waitable::READ) != 0){
		flags |= FD_READ;
	}
	if((flagsToWaitFor & Waitable::WRITE) != 0){
		flags |= FD_WRITE;
	}
	this->SetWaitingEventsForWindows(flags);
}
#endif
