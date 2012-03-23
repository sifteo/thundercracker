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

#include "Socket.hpp"
#include "IPAddress.hpp"



namespace ting{
namespace net{



/**
 * @brief UDP socket class.
 * Socket for User Datagram Protocol.
 * NOTE: Win32 specific: when using UDP socket with WaitSet be aware that waiting on UDP socket for writing does not work on Win32 OS.
 *       On other operating systems it works OK.
 */
class UDPSocket : public Socket{
public:
	UDPSocket(){}


	/**
	 * @brief Copy constructor.
	 * It works the same way as for copy constructor of TCPSocket.
	 * @param s - socket to copy from.
	 */
	UDPSocket(const UDPSocket& s) :
			Socket(s)
	{}



	/**
	 * @brief Assignment operator, works similar to std::auto_ptr::operator=().
	 * After this assignment operator completes this socket object refers to the socket the s object referred before, s become invalid.
	 * It works similar to std::auto_ptr::operator=() from standard C++ library.
	 * @param s - socket to assign from.
	 */
	UDPSocket& operator=(const UDPSocket& s){
		this->Socket::operator=(s);
		return *this;
	}


	
	/**
	 * @brief Open the socket.
	 * This method opens the socket, this socket can further be used to send or receive data.
	 * After the socket is opened it becomes a valid socket and Socket::IsValid() will return true for such socket.
	 * After the socket is closed it becomes invalid.
	 * In other words, a valid socket is an opened socket.
	 * In case of errors this method throws net::Exc.
	 * @param port - IP port number on which the socket will listen for incoming datagrams.
	 *               If 0 is passed then system will assign some free port if any. If there
	 *               are no free ports, then it is an error and an exception will be thrown.
	 * This is useful for server-side sockets, for client-side sockets use UDPSocket::Open().
	 */
	void Open(u16 port = 0);



	/**
	 * @brief Send datagram over UDP socket.
	 * The datagram is sent to UDP socket all at once. If the datagram cannot be
	 * sent at once at the current moment, 0 will be returned.
	 * Note, that underlying protocol limits the maximum size of the datagram,
	 * trying to send the bigger datagram will result in an exception to be thrown.
	 * @param buf - buffer containing the datagram to send.
	 * @param destinationIP - the destination IP address to send the datagram to.
	 * @return number of bytes actually sent. Actually it is either 0 or the size of the
	 *         datagram passed in as argument.
	 */
	size_t Send(const ting::Buffer<u8>& buf, const IPAddress& destinationIP);



	/**
	 * @brief Receive datagram.
	 * Writes a datagram to the given buffer at once if it is available.
	 * If there is no received datagram available a 0 will be returned.
	 * Note, that it will always write out the whole datagram at once. I.e. it is either all or nothing.
	 * Except for the case when the given buffer is not large enough to store the datagram,
	 * in which case the datagram is truncated to the size of the buffer and the rest of the data is lost.
	 * @param buf - reference to the buffer the received datagram will be stored to. The buffer
	 *              should be large enough to store the whole datagram. If datagram
	 *              does not fit the passed buffer, then the datagram tail will be truncated
	 *              and this tail data will be lost.
	 * @param out_SenderIP - reference to the IP-address structure where the IP-address
	 *                       of the sender will be stored.
	 * @return number of bytes stored in the output buffer.
	 */
	size_t Recv(ting::Buffer<u8>& buf, IPAddress &out_SenderIP);



#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
private:
	//override
	void SetWaitingEvents(u32 flagsToWaitFor);
#endif
};//~class UDPSocket


}//~namespace
}//~namespace
