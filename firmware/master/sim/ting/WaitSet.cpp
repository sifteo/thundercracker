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



#include "WaitSet.hpp"



using namespace ting;



void WaitSet::Add(Waitable* w, Waitable::EReadinessFlags flagsToWaitFor){
//		TRACE(<< "WaitSet::Add(): enter" << std::endl)
	ASSERT(w)

	ASSERT(!w->isAdded)

#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
	ASSERT(this->numWaitables <= this->handles.Size())
	if(this->numWaitables == this->handles.Size()){
		throw ting::Exc("WaitSet::Add(): wait set is full");
	}

	//NOTE: Setting wait flags may throw an exception, so do that before
	//adding object to the array and incrementing number of added objects.
	w->SetWaitingEvents(flagsToWaitFor);

	this->handles[this->numWaitables] = w->GetHandle();
	this->waitables[this->numWaitables] = w;

#elif defined(__linux__)
	epoll_event e;
	e.data.fd = w->GetHandle();
	e.data.ptr = w;
	e.events =
			(u32(flagsToWaitFor) & Waitable::READ ? (EPOLLIN | EPOLLPRI) : 0)
			| (u32(flagsToWaitFor) & Waitable::WRITE ? EPOLLOUT : 0)
			| (EPOLLERR);
	int res = epoll_ctl(
			this->epollSet,
			EPOLL_CTL_ADD,
			w->GetHandle(),
			&e
		);
	if(res < 0){
		throw ting::Exc("WaitSet::Add(): epoll_ctl() failed");
	}
#elif defined(__APPLE__)
	if(u32(flagsToWaitFor) & Waitable::READ){
		AddEvent(w, EVENT_READ);
	}

	if(u32(flagsToWaitFor) & Waitable::WRITE){
		AddEvent(w, EVENT_WRITE);
	}
#else
	#error "Unsupported OS"
#endif

	++this->numWaitables;

	w->isAdded = true;
//		TRACE(<< "WaitSet::Add(): exit" << std::endl)
}



void WaitSet::Change(Waitable* w, Waitable::EReadinessFlags flagsToWaitFor){
	ASSERT(w)

	ASSERT(w->isAdded)

#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
	//check if the Waitable object is added to this wait set
	{
		unsigned i;
		for(i = 0; i < this->numWaitables; ++i){
			if(this->waitables[i] == w){
				break;
			}
		}
		ASSERT(i <= this->numWaitables)
		if(i == this->numWaitables){
			throw ting::Exc("WaitSet::Change(): the Waitable is not added to this wait set");
		}
	}

	//set new wait flags
	w->SetWaitingEvents(flagsToWaitFor);

#elif defined(__linux__)
	epoll_event e;
	e.data.fd = w->GetHandle();
	e.data.ptr = w;
	e.events =
			(u32(flagsToWaitFor) & Waitable::READ ? (EPOLLIN | EPOLLPRI) : 0)
			| (u32(flagsToWaitFor) & Waitable::WRITE ? EPOLLOUT : 0)
			| (EPOLLERR);
	int res = epoll_ctl(
			this->epollSet,
			EPOLL_CTL_MOD,
			w->GetHandle(),
			&e
		);
	if(res < 0){
		throw ting::Exc("WaitSet::Change(): epoll_ctl() failed");
	}
#elif defined(__APPLE__)
	if(u32(flagsToWaitFor) & Waitable::READ){
		AddEvent(w, EVENT_READ);
	}

	if(u32(flagsToWaitFor) & Waitable::WRITE){
		AddEvent(w, EVENT_WRITE);
	}
#else
	#error "Unsupported OS"
#endif
}



void WaitSet::Remove(Waitable* w)throw(){
	ASSERT(w)

	ASSERT(w->isAdded)

#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
	//remove object from array
	{
		unsigned i;
		for(i = 0; i < this->numWaitables; ++i){
			if(this->waitables[i] == w){
				break;
			}
		}
		ASSERT(i <= this->numWaitables)
		ASSERT_INFO(i != this->numWaitables, "WaitSet::Remove(): Waitable is not added to wait set")

		unsigned numObjects = this->numWaitables - 1;//decrease number of objects before shifting the object handles in the array
		//shift object handles in the array
		for(; i < numObjects; ++i){
			this->handles[i] = this->handles[i + 1];
			this->waitables[i] = this->waitables[i + 1];
		}
	}

	//clear wait flags (disassociate socket and Windows event)
	w->SetWaitingEvents(0);

#elif defined(__linux__)
	int res = epoll_ctl(
			this->epollSet,
			EPOLL_CTL_DEL,
			w->GetHandle(),
			0
		);
	if(res < 0){
		ASSERT_INFO(false, "WaitSet::Remove(): epoll_ctl failed, probably the Waitable was not added to the wait set")
	}
#elif defined(__APPLE__)
		RemoveEvent(w, EVENT_READ);
		RemoveEvent(w, EVENT_WRITE);
#else
	#error "Unsupported OS"
#endif

	--this->numWaitables;

	w->isAdded = false;
//		TRACE(<< "WaitSet::Remove(): completed successfuly" << std::endl)
}



unsigned WaitSet::Wait(bool waitInfinitly, u32 timeout, Buffer<Waitable*>* out_events){
	if(this->numWaitables == 0){
		throw ting::Exc("WaitSet::Wait(): no Waitable objects were added to the WaitSet, can't perform Wait()");
	}

	if(out_events){
		if(out_events->Size() < this->numWaitables){
			throw ting::Exc("WaitSet::Wait(): passed out_events buffer is not large enough to hold all possible triggered objects");
		}
	}

#if M_OS == M_OS_WIN32 || M_OS == M_OS_WIN64
	DWORD waitTimeout = waitInfinitly ? (INFINITE) : DWORD(timeout);

	DWORD res = WaitForMultipleObjectsEx(
			this->numWaitables,
			this->handles.Begin(),
			FALSE, //do not wait for all objects, wait for at least one
			waitTimeout,
			FALSE
		);

	ASSERT(res != WAIT_IO_COMPLETION)//it is impossible because we supplied FALSE as last parameter to WaitForMultipleObjectsEx()

	//we are not expecting abandoned mutexes
	ASSERT(res < WAIT_ABANDONED_0 || (WAIT_ABANDONED_0 + this->numWaitables) <= res)

	if(res == WAIT_FAILED){
		throw ting::Exc("WaitSet::Wait(): WaitForMultipleObjectsEx() failed");
	}

	if(res == WAIT_TIMEOUT){
		return 0;
	}

	ASSERT(WAIT_OBJECT_0 <= res && res < (WAIT_OBJECT_0 + this->numWaitables ))

	//check for activities
	unsigned numEvents = 0;
	for(unsigned i = 0; i < this->numWaitables; ++i){
		if(this->waitables[i]->CheckSignalled()){
			if(out_events){
				ASSERT(numEvents < out_events->Size())
				out_events->operator[](numEvents) = this->waitables[i];
			}
			++numEvents;
		}else{
			//NOTE: sometimes the event is reported as signaled, but no read/write events indicated.
			//      Don't know why it happens.
//			ASSERT_INFO(i != (res - WAIT_OBJECT_0), "i = " << i << " (res - WAIT_OBJECT_0) = " << (res - WAIT_OBJECT_0) << " waitflags = " << this->waitables[i]->readinessFlags)
		}
	}

	//NOTE: Sometimes the event is reported as signaled, but no actual activity is there.
	//      Don't know why.
//		ASSERT(numEvents > 0)

	return numEvents;

#elif defined(__linux__)
	ASSERT(int(timeout) >= 0)
	int epollTimeout = waitInfinitly ? (-1) : int(timeout);

//		TRACE(<< "going to epoll_wait() with timeout = " << epollTimeout << std::endl)

	int res;

	while(true){
		res = epoll_wait(
				this->epollSet,
				this->revents.Begin(),
				this->revents.Size(),
				epollTimeout
			);

//			TRACE(<< "epoll_wait() returned " << res << std::endl)

		if(res < 0){
			//if interrupted by signal, try waiting again.
			if(errno == EINTR){
				continue;
			}

			std::stringstream ss;
			ss << "WaitSet::Wait(): epoll_wait() failed, error code = " << errno << ": " << strerror(errno);
			throw ting::Exc(ss.str().c_str());
		}
		break;
	};

	ASSERT(unsigned(res) <= this->revents.Size())

	unsigned numEvents = 0;
	for(
			epoll_event *e = this->revents.Begin();
			e < this->revents.Begin() + res;
			++e
		)
	{
		Waitable* w = static_cast<Waitable*>(e->data.ptr);
		ASSERT(w)
		if((e->events & EPOLLERR) != 0){
			w->SetErrorFlag();
		}
		if((e->events & (EPOLLIN | EPOLLPRI)) != 0){
			w->SetCanReadFlag();
		}
		if((e->events & EPOLLOUT) != 0){
			w->SetCanWriteFlag();
		}
		ASSERT(w->CanRead() || w->CanWrite() || w->ErrorCondition())
		if(out_events){
			ASSERT(numEvents < out_events->Size())
			out_events->operator[](numEvents) = w;
			++numEvents;
		}
	}

	ASSERT(res >= 0)//NOTE: 'res' can be zero, if no events happened in the specified timeout
	return unsigned(res);
#elif defined(__APPLE__)
	struct timespec tmout = {
		timeout / 1000, //seconds
		(timeout % 1000) * 1000000 //nanoseconds
	};

	//loop forever
	for(;;){
		int nev = kevent(
				kq_queue,
				0,
				0,
				&kq_output[0],
				kq_output.Size(),
				(waitInfinitly) ? 0 : &tmout
			);

		if(nev == -1 && errno != EINTR){
			throw ting::Exc("Error on kevent");
		}else if(nev == 0){
			return 0; // timeout
		}else if(nev > 0){
			for(int i = 0; i < nev; ++i){
				struct kevent &evt = kq_output[i];
				Waitable *w = reinterpret_cast<Waitable*>(evt.udata);
				if(evt.filter & EVFILT_WRITE){
					w->SetCanWriteFlag();
				}
				if(evt.filter & EVENT_READ){
					w->SetCanReadFlag();
				}
				//TODO: check for error condition?
				if(out_events){
					out_events->operator[](i) = w;
				}
			}
			return nev;
		}
	}
#else
	#error "Unsupported OS"
#endif
}
