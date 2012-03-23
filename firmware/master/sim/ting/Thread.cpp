/* The MIT License:

Copyright (c) 2008-2012 Ivan Gagis <igagis@gmail.com>

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



#include "Thread.hpp"



using namespace ting;



Mutex::Mutex(){
	M_MUTEX_TRACE(<< "Mutex::Mutex(): invoked " << this << std::endl)
#ifdef WIN32
	InitializeCriticalSection(&this->m);
#elif defined(__SYMBIAN32__)
	if(this->m.CreateLocal() != KErrNone){
		throw ting::Exc("Mutex::Mutex(): failed creating mutex (CreateLocal() failed)");
	}
#elif defined(__linux__) || defined(__APPLE__)
	if(pthread_mutex_init(&this->m, NULL) != 0){
		throw ting::Exc("Mutex::Mutex(): failed creating mutex (pthread_mutex_init() failed)");
	}
#else
#error "unknown system"
#endif
}



Mutex::~Mutex()throw(){
	M_MUTEX_TRACE(<< "Mutex::~Mutex(): invoked " << this << std::endl)
#ifdef WIN32
	DeleteCriticalSection(&this->m);
#elif defined(__SYMBIAN32__)
	this->m.Close();
#elif defined(__linux__) || defined(__APPLE__)
	#ifdef DEBUG
	int ret =
	#endif
	pthread_mutex_destroy(&this->m);
	#ifdef DEBUG
	if(ret != 0){
		std::stringstream ss;
		ss << "Mutex::~Mutex(): pthread_mutex_destroy() failed"
				<< " error code = " << ret << ": " << strerror(ret) << ".";
		if(ret == EBUSY){
			ss << " You are trying to destroy locked mutex.";
		}
		ASSERT_INFO_ALWAYS(false, ss.str())
	}
	#endif
#else
	#error "unknown system"
#endif
}



Semaphore::Semaphore(unsigned initialValue){
#ifdef WIN32
	if( (this->s = CreateSemaphore(NULL, initialValue, 0xffffff, NULL)) == NULL)
#elif defined(__SYMBIAN32__)
	if(this->s.CreateLocal(initialValue) != KErrNone)
#elif defined(__APPLE__)
	// Darwin/BSD/... semaphores are named semaphores, we need to create a 
	// different name for new semaphores.
	char name[256];
	// this n_name is shared among all semaphores, maybe it will be worth protect it
	// by a mutex or a CAS operation;
	static unsigned int n_name = 0;
	unsigned char p = 0;
	// fill the name
	for(unsigned char n = ++n_name, p = 0; n > 0;){
		unsigned char idx = n%('Z'-'A'+1);
		name[++p] = 'A'+idx;
		n -= idx;
	}
	// end it with null and create the semaphore
	name[p] = '\0';
	this->s = sem_open(name, O_CREAT, SEM_VALUE_MAX, initialValue);
	if (this->s == SEM_FAILED)
#elif defined(__linux__)
	if(sem_init(&this->s, 0, initialValue) < 0)
#else
#error "unknown system"
#endif
	{
		LOG(<< "Semaphore::Semaphore(): failed" << std::endl)
		throw ting::Exc("Semaphore::Semaphore(): creating semaphore failed");
	}
}



Semaphore::~Semaphore()throw(){
#ifdef WIN32
	CloseHandle(this->s);
#elif defined(__SYMBIAN32__)
	this->s.Close();
#elif defined(__APPLE__)
	sem_close(this->s);
#elif defined(__linux__)
	sem_destroy(&this->s);
#else
	#error "unknown system"
#endif
}



bool Semaphore::Wait(ting::u32 timeoutMillis){
#ifdef WIN32
	STATIC_ASSERT(INFINITE == 0xffffffff)
	switch(WaitForSingleObject(this->s, DWORD(timeoutMillis == INFINITE ? INFINITE - 1 : timeoutMillis))){
		case WAIT_OBJECT_0:
			return true;
		case WAIT_TIMEOUT:
			return false;
		default:
			throw ting::Exc("Semaphore::Wait(u32): wait failed");
			break;
	}
#elif defined(__APPLE__)
	int retVal = 0;

	// simulate the behavior of wait
	do{
		retVal = sem_trywait(this->s);
		if(retVal == 0){
			break; // OK leave the loop
		}else{
			if(errno == EAGAIN){ // the semaphore was blocked
				struct timespec amount;
				struct timespec result;
				int resultsleep;
				amount.tv_sec = timeoutMillis / 1000;
				amount.tv_nsec = (timeoutMillis % 1000) * 1000000;
				resultsleep = nanosleep(&amount, &result);
				// update timeoutMillis based on the output of the sleep call
				// if nanosleep() returns -1 the sleep was interrupted and result
				// struct is updated with the remaining un-slept time.
				if(resultsleep == 0){
					timeoutMillis = 0;
				}else{
					timeoutMillis = result.tv_sec * 1000 + result.tv_nsec / 1000000;
				}
			}else if(errno != EINTR){
				throw ting::Exc("Semaphore::Wait(): wait failed");
			}
		}
	}while(timeoutMillis > 0);

	// no time left means we reached the timeout
	if(timeoutMillis == 0){
		return false;
	}
#elif defined(__linux__)
	//if timeoutMillis is 0 then use sem_trywait() to avoid unnecessary time calculation for sem_timedwait()
	if(timeoutMillis == 0){
		if(sem_trywait(&this->s) == -1){
			if(errno == EAGAIN){
				return false;
			}else{
				throw ting::Exc("Semaphore::Wait(u32): error: sem_trywait() failed");
			}
		}
	}else{
		timespec ts;

		if(clock_gettime(CLOCK_REALTIME, &ts) == -1)
			throw ting::Exc("Semaphore::Wait(): clock_gettime() returned error");

		ts.tv_sec += timeoutMillis / 1000;
		ts.tv_nsec += (timeoutMillis % 1000) * 1000 * 1000;
		ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
		ts.tv_nsec = ts.tv_nsec % (1000 * 1000 * 1000);

		if(sem_timedwait(&this->s, &ts) == -1){
			if(errno == ETIMEDOUT){
				return false;
			}else{
				throw ting::Exc("Semaphore::Wait(u32): error: sem_timedwait() failed");
			}
		}
	}
#else
#error "unknown system"
#endif
	return true;
}



CondVar::CondVar(){
#if defined(WIN32) || defined(__SYMBIAN32__)
	this->numWaiters = 0;
	this->numSignals = 0;
#elif defined(__linux__) || defined(__APPLE__)
	pthread_cond_init(&this->cond, NULL);
#else
#error "unknown system"
#endif
}



CondVar::~CondVar()throw(){
#if defined(WIN32) || defined(__SYMBIAN32__)
	//do nothing
#elif defined(__linux__) || defined(__APPLE__)
	pthread_cond_destroy(&this->cond);
#else
	#error "unknown system"
#endif
}



Queue::Queue(){
	//can write will always be set because it is always possible to post a message to the queue
	this->SetCanWriteFlag();

#if defined(WIN32)
	this->eventForWaitable = CreateEvent(
			NULL, //security attributes
			TRUE, //manual-reset
			FALSE, //not signalled initially
			NULL //no name
		);
	if(this->eventForWaitable == NULL){
		throw ting::Exc("Queue::Queue(): could not create event (Win32) for implementing Waitable");
	}
#elif defined(__APPLE__) || defined(__ANDROID__) //TODO: for Android revert to using eventFD when it becomes available in Android NDK
	if(::pipe(&this->pipeEnds[0]) < 0){
		std::stringstream ss;
		ss << "Queue::Queue(): could not create pipe (*nix) for implementing Waitable,"
				<< " error code = " << errno << ": " << strerror(errno);
		throw ting::Exc(ss.str().c_str());
	}
#elif defined(__linux__)
	this->eventFD = eventfd(0, EFD_NONBLOCK);
	if(this->eventFD < 0){
		std::stringstream ss;
		ss << "Queue::Queue(): could not create eventfd (linux) for implementing Waitable,"
				<< " error code = " << errno << ": " << strerror(errno);
		throw ting::Exc(ss.str().c_str());
	}
#else
#error "Unsupported OS"
#endif
}



Queue::~Queue()throw(){
	//destroy messages which are currently on the queue
	{
		Mutex::Guard mutexGuard(this->mut);
		Message *msg = this->first;
		Message	*nextMsg;
		while(msg){
			nextMsg = msg->next;
			//use Ptr to kill messages instead of "delete msg;" because
			//the messages are passed to PushMessage() as Ptr, and thus, it is better
			//to use Ptr to delete them.
			{
				Ptr<Message> killer(msg);
			}

			msg = nextMsg;
		}
	}
#if defined(WIN32)
	CloseHandle(this->eventForWaitable);
#elif defined(__APPLE__) || defined(__ANDROID__) //TODO: for Android revert to using eventFD when it becomes available in Android NDK
	close(this->pipeEnds[0]);
	close(this->pipeEnds[1]);
#elif defined(__linux__)
	close(this->eventFD);
#else
	#error "Unsupported OS"
#endif
}



void Queue::PushMessage(Ptr<Message> msg)throw(){
	ASSERT(msg.IsValid())
	Mutex::Guard mutexGuard(this->mut);
	if(this->first){
		ASSERT(this->last && this->last->next == 0)
		this->last = this->last->next = msg.Extract();
		ASSERT(this->last->next == 0)
	}else{
		ASSERT(msg.IsValid())
		this->last = this->first = msg.Extract();

		//Set CanRead flag.
		//NOTE: in linux implementation with epoll(), the CanRead
		//flag will also be set in WaitSet::Wait() method.
		//NOTE: set CanRead flag before event notification/pipe write, because
		//if do it after then some other thread which was waiting on the WaitSet
		//may read the CanRead flag while it was not set yet.
		ASSERT(!this->CanRead())
		this->SetCanReadFlag();

#if defined(WIN32)
		if(SetEvent(this->eventForWaitable) == 0){
			ASSERT(false)
		}
#elif defined(__APPLE__) || defined(__ANDROID__) //TODO: for Android revert to using eventFD when it becomes available in Android NDK
		{
			u8 oneByteBuf[1];
			if(write(this->pipeEnds[1], oneByteBuf, 1) != 1){
				ASSERT(false)
			}
		}
#elif defined(__linux__)
		if(eventfd_write(this->eventFD, 1) < 0){
			ASSERT(false)
		}
#else
	#error "Unsupported OS"
#endif
	}

	ASSERT(this->CanRead())
	//NOTE: must do signaling while mutex is locked!!!
	this->sem.Signal();
}



Ptr<Message> Queue::PeekMsg(){
	Mutex::Guard mutexGuard(this->mut);
	if(this->first){
		ASSERT(this->CanRead())
		//NOTE: Decrement semaphore value, because we take one message from queue.
		//      The semaphore value should be > 0 here, so there will be no hang
		//      in Wait().
		//      The semaphore value actually reflects the number of Messages in
		//      the queue.
		this->sem.Wait();

		ASSERT(this->first)
		if(this->first->next == 0){//if we are taking away the last message from the queue
#if defined(WIN32)
			if(ResetEvent(this->eventForWaitable) == 0){
				ASSERT(false)
				throw ting::Exc("Queue::Wait(): ResetEvent() failed");
			}
#elif defined(__APPLE__) || defined(__ANDROID__) //TODO: for Android revert to using eventFD when it becomes available in Android NDK
			{
				u8 oneByteBuf[1];
				if(read(this->pipeEnds[0], oneByteBuf, 1) != 1){
					throw ting::Exc("Queue::Wait(): read() failed");
				}
			}
#elif defined(__linux__)
			{
				eventfd_t value;
				if(eventfd_read(this->eventFD, &value) < 0){
					throw ting::Exc("Queue::Wait(): eventfd_read() failed");
				}
				ASSERT(value == 1)
			}
#else
#error "Unsupported OS"
#endif
			this->ClearCanReadFlag();
		}else{
			ASSERT(this->CanRead())
		}

		Message* ret = this->first;
		this->first = this->first->next;
		
		return Ptr<Message>(ret);
	}
	return Ptr<Message>();
}



Ptr<Message> Queue::GetMsg(){
	M_QUEUE_TRACE(<< "Queue[" << this << "]::GetMsg(): enter" << std::endl)
	{
		Mutex::Guard mutexGuard(this->mut);
		if(this->first){
			ASSERT(this->CanRead())
			//NOTE: Decrement semaphore value, because we take one message from queue.
			//      The semaphore value should be > 0 here, so there will be no hang
			//      in Wait().
			//      The semaphore value actually reflects the number of Messages in
			//      the queue.
			this->sem.Wait();

			ASSERT(this->first)
			
			if(this->first->next == 0){//if we are taking away the last message from the queue
#if defined(WIN32)
				if(ResetEvent(this->eventForWaitable) == 0){
					ASSERT(false)
					throw ting::Exc("Queue::Wait(): ResetEvent() failed");
				}
#elif defined(__APPLE__) || defined(__ANDROID__) //TODO: for Android revert to using eventFD when it becomes available in Android NDK
				{
					u8 oneByteBuf[1];
					read(this->pipeEnds[0], oneByteBuf, 1);
				}
#elif defined(__linux__)
				{
					eventfd_t value;
					if(eventfd_read(this->eventFD, &value) < 0){
						throw ting::Exc("Queue::Wait(): eventfd_read() failed");
					}
					ASSERT(value == 1)
				}
#else
#error "Unsupported OS"
#endif
				this->ClearCanReadFlag();
			}else{
				ASSERT(this->CanRead())
			}
			
			Message* ret = this->first;
			this->first = this->first->next;

			M_QUEUE_TRACE(<< "Queue[" << this << "]::GetMsg(): exit without waiting on semaphore" << std::endl)
			return Ptr<Message>(ret);
		}
	}
	M_QUEUE_TRACE(<< "Queue[" << this << "]::GetMsg(): waiting" << std::endl)

	this->sem.Wait();

	M_QUEUE_TRACE(<< "Queue[" << this << "]::GetMsg(): signaled" << std::endl)
	{
		Mutex::Guard mutexGuard(this->mut);
		ASSERT(this->CanRead())
		ASSERT(this->first)

		if(this->first->next == 0){//if we are taking away the last message from the queue
#if defined(WIN32)
			if(ResetEvent(this->eventForWaitable) == 0){
				ASSERT(false)
				throw ting::Exc("Queue::Wait(): ResetEvent() failed");
			}
#elif defined(__APPLE__) || defined(__ANDROID__) //TODO: for Android revert to using eventFD when it becomes available in Android NDK
			{
				u8 oneByteBuf[1];
				read(this->pipeEnds[0], oneByteBuf, 1);
			}
#elif defined(__linux__)
			{
				eventfd_t value;
				if(eventfd_read(this->eventFD, &value) < 0){
					throw ting::Exc("Queue::Wait(): eventfd_read() failed");
				}
				ASSERT(value == 1)
			}
#else
#error "Unsupported OS"
#endif
			this->ClearCanReadFlag();
		}else{
			ASSERT(this->CanRead())
		}
		
		Message* ret = this->first;
		this->first = this->first->next;

		M_QUEUE_TRACE(<< "Queue[" << this << "]::GetMsg(): exit after waiting on semaphore" << std::endl)
		return Ptr<Message>(ret);
	}
}



#ifdef WIN32
//override
HANDLE Queue::GetHandle(){
	//return event handle
	return this->eventForWaitable;
}



//override
void Queue::SetWaitingEvents(u32 flagsToWaitFor){
	//It is not allowed to wait on queue for write,
	//because it is always possible to push new message to queue.
	//Error condition is not possible for Queue.
	//Thus, only possible flag values are READ and 0 (NOT_READY)
	if(flagsToWaitFor != 0 && flagsToWaitFor != ting::Waitable::READ){
		ASSERT_INFO(false, "flagsToWaitFor = " << flagsToWaitFor)
		throw ting::Exc("Queue::SetWaitingEvents(): flagsToWaitFor should be ting::Waitable::READ or 0, other values are not allowed");
	}

	this->flagsMask = flagsToWaitFor;
}



//returns true if signalled
//override
bool Queue::CheckSignalled(){
	//error condition is not possible for queue
	ASSERT((this->readinessFlags & ting::Waitable::ERROR_CONDITION) == 0)

/*
#ifdef DEBUG
	{
		Mutex::Guard mutexGuard(this->mut);
		if(this->first){
			ASSERT_ALWAYS(this->CanRead())

			//event should be in signalled state
			ASSERT_ALWAYS(WaitForSingleObject(this->eventForWaitable, 0) == WAIT_OBJECT_0)
		}

		if(this->CanRead()){
			ASSERT_ALWAYS(this->first)

			//event should be in signalled state
			ASSERT_ALWAYS(WaitForSingleObject(this->eventForWaitable, 0) == WAIT_OBJECT_0)
		}

		//if event is in signalled state
		if(WaitForSingleObject(this->eventForWaitable, 0) == WAIT_OBJECT_0){
			ASSERT_ALWAYS(this->CanRead())
			ASSERT_ALWAYS(this->first)
		}
	}
#endif
*/

	return (this->readinessFlags & this->flagsMask) != 0;
}

#elif defined(__APPLE__) || defined(__ANDROID__) //TODO: for Android revert to using eventFD when it becomes available in Android NDK
//override
int Queue::GetHandle(){
	//return read end of pipe
	return this->pipeEnds[0];
}

#elif defined(__linux__)
//override
int Queue::GetHandle(){
	return this->eventFD;
}

#else
#error "Unsupported OS"
#endif



namespace{
ting::Mutex threadMutex2;
}



//Tread Run function
//static
#ifdef WIN32
unsigned int __stdcall Thread::RunThread(void *data)
#elif defined(__SYMBIAN32__)
TInt Thread::RunThread(TAny *data)
#elif defined(__linux__) || defined(__APPLE__)
void* Thread::RunThread(void *data)
#else
#error "Unsupported OS"
#endif
{
	ting::Thread *thr = reinterpret_cast<ting::Thread*>(data);
	try{
		thr->Run();
	}catch(ting::Exc& e){
		ASSERT_INFO(false, "uncaught ting::Exc exception in Thread::Run(): " << e.What())
	}catch(std::exception& e){
		ASSERT_INFO(false, "uncaught std::exception exception in Thread::Run(): " << e.what())
	}catch(...){
		ASSERT_INFO(false, "uncaught unknown exception in Thread::Run()")
	}

	{
		//protect by mutex to avoid changing the
		//this->state variable before Start() has finished.
		ting::Mutex::Guard mutexGuard(threadMutex2);

		thr->state = STOPPED;
	}

#ifdef WIN32
	//Do nothing, _endthreadex() will be called   automatically
	//upon returning from the thread routine.
#elif defined(__linux__) || defined(__APPLE__)
	pthread_exit(0);
#else
#error "Unsupported OS"
#endif
	return 0;
}



Thread::Thread(){
#if defined(WIN32)
	this->th = NULL;
#elif defined(__SYMBIAN32__)
	//do nothing
#elif defined(__linux__) || defined(__APPLE__)
	//do nothing
#else
#error "Unsuported OS"
#endif
}



Thread::~Thread()throw(){
	ASSERT_INFO(
			this->state == JOINED || this->state == NEW,
			"~Thread() destructor is called while the thread was not joined before. "
			<< "Make sure the thread is joined by calling Thread::Join() "
			<< "before destroying the thread object."
		)

	//NOTE: it is incorrect to put this->Join() to this destructor, because
	//thread shall already be stopped at the moment when this destructor
	//is called. If it is not, then the thread will be still running
	//when part of the thread object is already destroyed, since thread object is
	//usually a derived object from Thread class and the destructor of this derived
	//object will be called before ~Thread() destructor.
}



void Thread::Start(size_t stackSize){
	//Protect by mutex to avoid several Start() methods to be called
	//by concurrent threads simultaneously and to protect call to Join() before Start()
	//has returned.
	ting::Mutex::Guard mutexGuard1(this->mutex1);
	
	//Protect by mutex to avoid incorrect state changing in case when thread
	//exits before the Start() method returned.
	ting::Mutex::Guard mutexGuard2(threadMutex2);

	if(this->state != NEW){
		throw HasAlreadyBeenStartedExc();
	}

#ifdef WIN32
	this->th = reinterpret_cast<HANDLE>(
			_beginthreadex(
					NULL,
					stackSize > size_t(unsigned(-1)) ? unsigned(-1) : unsigned(stackSize),
					&RunThread,
					reinterpret_cast<void*>(this),
					0,
					NULL
				)
		);
	if(this->th == NULL){
		throw Exc("Thread::Start(): _beginthreadex failed");
	}
#elif defined(__SYMBIAN32__)
	if(this->th.Create(_L("ting thread"), &RunThread,
				stackSize == 0 ? KDefaultStackSize : stackSize,
				NULL, reinterpret_cast<TAny*>(this)) != KErrNone
			)
	{
		throw Exc("Thread::Start(): starting thread failed");
	}
	this->th.Resume();//start the thread execution
#elif defined(__linux__) || defined(__APPLE__)
	{
		pthread_attr_t attr;

		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		pthread_attr_setstacksize(&attr, stackSize);

		int res = pthread_create(&this->th, &attr, &RunThread, this);
		if(res != 0){
			pthread_attr_destroy(&attr);
			TRACE_AND_LOG(<< "Thread::Start(): pthread_create() failed, error code = " << res
					<< " meaning: " << strerror(res) << std::endl)
			std::stringstream ss;
			ss << "Thread::Start(): starting thread failed,"
					<< " error code = " << res << ": " << strerror(res);
			throw Exc(ss.str());
		}
		pthread_attr_destroy(&attr);
	}
#else
#error "Unsupported OS"
#endif
	this->state = RUNNING;
}



void Thread::Join() throw(){
//	TRACE(<< "Thread::Join(): enter" << std::endl)

	//protect by mutex to avoid several Join() methods to be called by concurrent threads simultaneously.
	//NOTE: excerpt from pthread docs: "If multiple threads simultaneously try to join with the same thread, the results are undefined."
	ting::Mutex::Guard mutexGuard(this->mutex1);

	if(this->state == NEW){
		//thread was not started, do nothing
		return;
	}

	if(this->state == JOINED){
		return;
	}

	ASSERT(this->state == RUNNING || this->state == STOPPED)
	
	ASSERT_INFO(T_ThreadID(this->th) != ting::Thread::GetCurrentThreadID(), "tried to call Join() on the current thread")

#ifdef WIN32
	WaitForSingleObject(this->th, INFINITE);
	CloseHandle(this->th);
	this->th = NULL;
#elif defined(__SYMBIAN32__)
	TRequestStatus reqStat;
	this->th.Logon(reqStat);
	User::WaitForRequest(reqStat);
	this->th.Close();
#elif defined(__linux__) || defined(__APPLE__)
#ifdef DEBUG
	int res =
#endif
			pthread_join(this->th, 0);
	ASSERT_INFO(res == 0, "res = " << strerror(res))
#else
#error "Unsupported OS"
#endif

	//NOTE: at this point the thread's Run() method should already exit and state
	//should be set to STOPPED
	ASSERT_INFO(this->state == STOPPED, "this->state = " << this->state)

	this->state = JOINED;

//	TRACE(<< "Thread::Join(): exit" << std::endl)
}



namespace{
ting::Mutex quitMessageMutex;
}



void MsgThread::PushPreallocatedQuitMessage()throw(){
	ting::Mutex::Guard mutexGuard(quitMessageMutex);
	
	if(this->quitMessage.IsNotValid()){
		return;
	}
	
	this->queue.PushMessage(this->quitMessage);
}

