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


#include "Socket.hpp"
#include "IPAddress.hpp"




namespace ting{
namespace net{



//forward declarations
class TCPServerSocket;



/**
 * @brief a class which represents a TCP socket.
 */
class TCPSocket : public Socket{
	friend class ting::net::TCPServerSocket;
public:
	
	/**
	 * @brief Constructs an invalid TCP socket object.
	 */
	TCPSocket(){
//		TRACE(<< "TCPSocket::TCPSocket(): invoked " << this << std::endl)
	}

	
	
	/**
	 * @brief A copy constructor.
	 * Copy constructor creates a new socket object which refers to the same socket as s.
	 * After constructor completes the s becomes invalid.
	 * In other words, the behavior of copy constructor is similar to one of std::auto_ptr class from standard C++ library.
	 * @param s - other TCP socket to make a copy from.
	 */
	//copy constructor
	TCPSocket(const TCPSocket& s) :
			Socket(s)
	{
//		TRACE(<< "TCPSocket::TCPSocket(copy): invoked " << this << std::endl)
	}

	
	
	/**
	 * @brief Assignment operator, works similar to std::auto_ptr::operator=().
	 * After this assignment operator completes this socket object refers to the socket the s object referred, s become invalid.
	 * It works similar to std::auto_ptr::operator=() from standard C++ library.
	 * @param s - socket to assign from.
	 */
	inline TCPSocket& operator=(const TCPSocket& s){
		this->Socket::operator=(s);
		return *this;
	}

	
	
	/**
	 * @brief Connects the socket.
	 * This method connects the socket to remote TCP server socket.
	 * @param ip - IP address.
	 * @param disableNaggle - enable/disable Naggle algorithm.
	 */
	void Open(const IPAddress& ip, bool disableNaggle = false);



	/**
	 * @brief Send data to connected socket.
	 * Sends data on connected socket. This method does not guarantee that the whole
	 * buffer will be sent completely, it will return the number of bytes actually sent.
	 * @param buf - pointer to the buffer with data to send.
	 * @param offset - offset inside the buffer from where to start sending the data.
	 * @return the number of bytes actually sent.
	 */
	size_t Send(const ting::Buffer<u8>& buf, size_t offset = 0);



	/**
	 * @brief Receive data from connected socket.
	 * Receives data available on the socket.
	 * If there is no data available this function does not block, instead it returns 0,
	 * indicating that 0 bytes were received.
	 * If previous WaitSet::Wait() indicated that socket is ready for reading
	 * and TCPSocket::Recv() returns 0, then connection was closed by peer.
	 * @param buf - pointer to the buffer where to put received data.
	 * @param offset - offset inside the buffer where to start putting data from.
	 * @return the number of bytes written to the buffer.
	 */
	size_t Recv(ting::Buffer<u8>& buf, size_t offset = 0);

	
	
	/**
	 * @brief Get local IP address and port.
	 * @return IP address and port of the local socket.
	 */
	IPAddress GetLocalAddress();
	
	
	
	/**
	 * @brief Get remote IP address and port.
	 * @return IP address and port of the peer socket.
	 */
	IPAddress GetRemoteAddress();



#if M_OS == M_OS_WIN32 || M_OS_WIN64
private:
	//override
	void SetWaitingEvents(u32 flagsToWaitFor);
#endif

};//~class TCPSocket



}//~namespace
}//~namespace
