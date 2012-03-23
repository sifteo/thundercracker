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
 * @file WaitSet.hpp
 * @author Ivan Gagis <igagis@gmail.com>
 * @author Jose Luis Hidalgo <joseluis.hidalgo@gmail.com> - Mac OS X port
 * @brief Wait set.
 */

#pragma once

#include <vector>
#include <sstream>
#include <cerrno>

#include "config.hpp"
#include "types.hpp"
#include "debug.hpp"
#include "Exc.hpp"
#include "Array.hpp"


#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64

//if _WINSOCKAPI_ macro is not defined then it means that the winsock header file
//has not been included. Here we temporarily define the macro in order to prevent
//inclusion of winsock.h from within the windows.h. Because it may later conflict with
//winsock2.h if it is included later.
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
	#include <windows.h>
#undef _WINSOCKAPI_
#else
	#include <windows.h>
#endif

#elif defined(__linux__)
	#include <sys/epoll.h>

#elif defined(__APPLE__)
	#include <sys/types.h>
	#include <sys/event.h>
	#include <sys/time.h>

#else
#error "Unsupported OS"
#endif


//if Microsoft MSVC compiler,
//then disable warning about throw specification is ignored.
#ifdef _MSC_VER
#pragma warning(push) //push warnings state
#pragma warning( disable : 4290)
#endif


namespace ting{



/**
 * @brief Base class for objects which can be waited for.
 * Base class for objects which can be used in wait sets.
 */
class Waitable{
	friend class WaitSet;

	ting::Inited<bool, false> isAdded;

	ting::Inited<void*, 0> userData;

public:
	enum EReadinessFlags{
		NOT_READY = 0,      // bin: 00000000
		READ = 1,           // bin: 00000001
		WRITE = 2,          // bin: 00000010
		READ_AND_WRITE = 3, // bin: 00000011
		ERROR_CONDITION = 4 // bin: 00000100
	};

protected:
	ting::Inited<u32, NOT_READY> readinessFlags;

	inline Waitable(){}



	inline bool IsAdded()const throw(){
		return this->isAdded;
	}



	/**
	 * @brief Copy constructor.
	 * It is not possible to copy a waitable which is currently added to WaitSet.
	 * Works as std::auto_ptr, i.e. the object it copied from becomes invalid.
	 * Use this copy constructor only if you really know what you are doing.
	 * @param w - Waitable object to copy.
	 */
	inline Waitable(const Waitable& w) :
			isAdded(false),
			userData(w.userData),
			readinessFlags(NOT_READY)//Treat copied Waitable as NOT_READY
	{
		//cannot copy from waitable which is added to WaitSet
		if(w.isAdded){
			throw ting::Exc("Waitable::Waitable(copy): cannot copy Waitable which is added to WaitSet");
		}

		const_cast<Waitable&>(w).ClearAllReadinessFlags();
		const_cast<Waitable&>(w).userData = 0;
	}



	/**
	 * @brief Assignment operator.
	 * It is not possible to assign a waitable which is currently added to WaitSet.
	 * Works as std::auto_ptr, i.e. the object it copied from becomes invalid.
	 * Use this copy constructor only if you really know what you are doing.
	 * @param w - Waitable object to assign to this object.
	 */
	inline Waitable& operator=(const Waitable& w){
		//cannot copy because this Waitable is added to WaitSet
		if(this->isAdded){
			throw ting::Exc("Waitable::Waitable(copy): cannot copy while this Waitable is added to WaitSet");
		}

		//cannot copy from waitable which is adde to WaitSet
		if(w.isAdded){
			throw ting::Exc("Waitable::Waitable(copy): cannot copy Waitable which is added to WaitSet");
		}

		ASSERT(!this->isAdded)

		//Clear readiness flags on copying.
		//Will need to wait for readiness again, using the WaitSet.
		this->ClearAllReadinessFlags();
		const_cast<Waitable&>(w).ClearAllReadinessFlags();

		this->userData = w.userData;
		const_cast<Waitable&>(w).userData = 0;
		return *this;
	}



	inline void SetCanReadFlag()throw(){
		this->readinessFlags |= READ;
	}

	inline void ClearCanReadFlag()throw(){
		this->readinessFlags &= (~READ);
	}

	inline void SetCanWriteFlag()throw(){
		this->readinessFlags |= WRITE;
	}

	inline void ClearCanWriteFlag()throw(){
		this->readinessFlags &= (~WRITE);
	}

	inline void SetErrorFlag()throw(){
		this->readinessFlags |= ERROR_CONDITION;
	}

	inline void ClearErrorFlag()throw(){
		this->readinessFlags &= (~ERROR_CONDITION);
	}

	inline void ClearAllReadinessFlags()throw(){
		this->readinessFlags = NOT_READY;
	}

public:
	virtual ~Waitable()throw(){
		ASSERT(!this->isAdded)
	}

	/**
	 * @brief Check if "Can read" flag is set.
	 * @return true if Waitable is ready for reading.
	 */
	inline bool CanRead()const throw(){
		return (this->readinessFlags & READ) != 0;
	}

	/**
	 * @brief Check if "Can write" flag is set.
	 * @return true if Waitable is ready for writing.
	 */
	inline bool CanWrite()const throw(){
		return (this->readinessFlags & WRITE) != 0;
	}

	/**
	 * @brief Check if "error" flag is set.
	 * @return true if Waitable is in error state.
	 */
	inline bool ErrorCondition()const throw(){
		return (this->readinessFlags & ERROR_CONDITION) != 0;
	}

	/**
	 * @brief Get user data associated with this Waitable.
	 * Returns the pointer to the user data which was previously set by SetUserData() method.
	 * @return pointer to the user data.
	 * @return zero pointer if the user data was not set.
	 */
	inline void* GetUserData()throw(){
		return this->userData;
	}

	/**
	 * @brief Set user data.
	 * See description of GetUserData() for more details.
	 * @param data - pointer to the user data to associate with this Waitable.
	 */
	inline void SetUserData(void* data)throw(){
		this->userData = data;
	}

#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
protected:
	virtual HANDLE GetHandle() = 0;

	virtual void SetWaitingEvents(u32 /*flagsToWaitFor*/){}

	//returns true if signalled
	virtual bool CheckSignalled(){
		return this->readinessFlags != 0;
	}



#elif defined(__linux__) || defined(__APPLE__)
protected:
	virtual int GetHandle() = 0;



#else
	#error "Unsupported OS"
#endif

};//~class Waitable





/**
 * @brief Set of Waitable objects to wait for.
 */
class WaitSet{
	const unsigned size;
	ting::Inited<unsigned, 0> numWaitables;//number of Waitables added

#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
	Array<Waitable*> waitables;
	Array<HANDLE> handles; //used to pass array of HANDLEs to WaitForMultipleObjectsEx()

#elif defined(__linux__) 
	int epollSet;

	Array<epoll_event> revents;//used for getting the result from epoll_wait()
#elif defined(__APPLE__)
	int kq_queue; // kqueue, file descriptor
	Array<struct kevent> kq_output; // events (size*2)

	//TODO: move to .cpp?
	void SetEvent(Waitable *w, bool read_write, bool add_remove){
		int16_t filter = (read_write) ? EVFILT_READ : EVFILT_WRITE;
		uint16_t flags = (add_remove) ? EV_ADD : EV_DELETE;
		struct kevent input, output;
		static const struct timespec tmout = {0, 0};//TODO: why static???

		//add a new event
		EV_SET(&input, w->GetHandle(), filter, flags | EV_RECEIPT, 0, 0, (void*)w);

		//now try to add this event to the kqueue
		int result = kevent(kq_queue, &input, 1, &output, 1, &tmout);
		if(result == -1){
			//TODO: ???
			//std::cout << "ERROR on Set " << errno << "!" << std::endl;
		}

		if(result == -1 && errno == ENOENT){
			//TODO: ???
			//std::cout << "blah" << std::endl;
		}

		//TODO: add assertion on result???
	}

	static const bool EVENT_READ  = true;
	static const bool EVENT_WRITE = false;

	void AddEvent(Waitable *w, bool rw){
		SetEvent(w, rw, true);
	}

	void RemoveEvent(Waitable *w, bool rw){
		SetEvent(w, rw, false);
	}
#else
#error "Unsupported OS"
#endif

public:

	/**
	 * @brief Constructor.
	 * @param maxSize - maximum number of Waitable objects can be added to this wait set.
	 */
	WaitSet(unsigned maxSize) :
			size(maxSize)
#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
			,waitables(maxSize)
			,handles(maxSize)
	{
		ASSERT_INFO(maxSize <= MAXIMUM_WAIT_OBJECTS, "maxSize should be less than " << MAXIMUM_WAIT_OBJECTS)
		if(maxSize > MAXIMUM_WAIT_OBJECTS){
			throw ting::Exc("WaitSet::WaitSet(): requested WaitSet size is too big");
		}
	}

#elif defined(__linux__)
			,revents(maxSize)
	{
		ASSERT(int(maxSize) > 0)
		this->epollSet = epoll_create(int(maxSize));
		if(this->epollSet < 0){
			throw ting::Exc("WaitSet::WaitSet(): epoll_create() failed");
		}
	}
#elif defined(__APPLE__)
			,kq_output(maxSize * 2)
	{
		this->kq_queue = kqueue();
		if(this->kq_queue == -1){
			throw ting::Exc("WaitSet::WaitSet(): kqueue creation failed");
		}
	}
#else
#error "Unsupported OS"
#endif



	/**
	 * @brief Destructor.
	 * Note, that destructor will check if the wait set is empty. If it is not, then an assert
	 * will be triggered.
	 * It is user's responsibility to remove any waitable objects from the waitset
	 * before the wait set object is destroyed.
	 */
	~WaitSet()throw(){
		//assert the wait set is empty
		ASSERT_INFO(this->numWaitables == 0, "attempt to destroy WaitSet containig Waitables")
#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
		//do nothing
#elif defined(__linux__)
		close(this->epollSet);
#elif defined(__APPLE__)
		close(this->kq_queue);
#else
	#error "Unsupported OS"
#endif
	}



	/**
	 * @brief Get maximum size of the wait set.
	 * @return maximum number of Waitables this WaitSet can hold.
	 */
	inline unsigned Size()const throw(){
		return this->size;
	}

	/**
	 * @brief Get number of Waitables already added to this WaitSet.
	 * @return number of Waitables added to this WaitSet.
	 */
	inline unsigned NumWaitables()const throw(){
		return this->numWaitables;
	}


	/**
	 * @brief Add Waitable object to the wait set.
	 * @param w - pointer to the Waitable object.
	 * @param flagsToWaitFor - determine events waiting for which we are interested.
	 * @throw ting::Exc - in case the wait set is full or other error occurs.
	 */
	void Add(Waitable* w, Waitable::EReadinessFlags flagsToWaitFor);



	/**
	 * @brief Change wait flags for a given Waitable.
	 * Changes wait flags for a given waitable, which is in this waitset.
	 * @param w - pointer to Waitable for which the changing of wait flags is needed.
	 * @param flagsToWaitFor - new wait flags to be set for the given Waitable.
	 * @throw ting::Exc - in case the given Waitable object is not added to this wait set or
	 *                    other error occurs.
	 */
	void Change(Waitable* w, Waitable::EReadinessFlags flagsToWaitFor);



	/**
	 * @brief Remove Waitable from wait set.
	 * @param w - pointer to Waitable object to be removed from the wait set.
	 * @throw ting::Exc - in case the given Waitable is not added to this wait set or
	 *                    other error occurs.
	 */
	void Remove(Waitable* w)throw();



	/**
	 * @brief wait for event.
	 * This function blocks calling thread exectution until one of the Waitable objects in the WaitSet
	 * triggers. Upon return from the function, pointers to triggered objects are placed in the
	 * 'out_events' buffer and the return value from the function indicates number of these objects
	 * which have triggered.
	 * @param out_events - pointer to buffer where to put pointers to triggered Waitable objects.
	 *                     The buffer will not be initialized to 0's by this function.
	 *                     The buffer shall be large enough to hold maxmimum number of Waitables
	 *                     this WaitSet can hold.
	 *                     It is valid to pass 0 pointer, in that case this argument will not be used.
	 * @return number of objects triggered.
	 *         NOTE: for some reason, on Windows it can return 0 objects triggered.
	 * @throw ting::Exc - in case of errors.
	 */
	inline unsigned Wait(Buffer<Waitable*>* out_events = 0){
		return this->Wait(true, 0, out_events);
	}


	/**
	 * @brief wait for event with timeout.
	 * The same as Wait() function, but takes wait timeout as parameter. Thus,
	 * this function will wait for any event or timeout. Note, that it guarantees that
	 * it will wait AT LEAST for specified number of milliseconds, or more. This is because of
	 * implementation for linux, if wait is interrupted by signal it will start waiting again,
	 * and so on.
	 * @param timeout - maximum time in milliseconds to wait for event.
	 * @param out_events - pointer to buffer where to put pointers to triggered Waitable objects.
	 *                     The buffer size must be equal or greater than the number ow waitables
	 *                     currently added to the wait set.
	 *                     This pointer can be 0, if you are not interested in list of triggered waitables.
	 * @return number of objects triggered. If 0 then timeout was hit.
	 *         NOTE: for some reason, on Windows it can return 0 before timeout was hit.
	 * @throw ting::Exc - in case of errors.
	 */
	inline unsigned WaitWithTimeout(u32 timeout, Buffer<Waitable*>* out_events = 0){
		return this->Wait(false, timeout, out_events);
	}



private:
	unsigned Wait(bool waitInfinitly, u32 timeout, Buffer<Waitable*>* out_events);
};//~class WaitSet



}//~namespace ting


//if Microsoft MSVC compiler, restore warnings state
#ifdef _MSC_VER
#pragma warning(pop) //pop warnings state
#endif

