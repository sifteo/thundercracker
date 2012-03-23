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



/**
 * @file Thread.hpp
 * @author Ivan Gagis <igagis@gmail.com>
 * @author Jose Luis Hidalgo <joseluis.hidalgo@gmail.com> - Mac OS X port
 * @brief Multithreading library.
 */

#pragma once

#include <cstring>
#include <sstream>

#include "config.hpp"
#include "debug.hpp"
#include "Ptr.hpp"
#include "types.hpp"
#include "Exc.hpp"
#include "WaitSet.hpp"


//=========
//= WIN32 =
//=========
#if defined(WIN32)

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

#include <process.h>



//===========
//= Symbian =
//===========
#elif defined(__SYMBIAN32__)
#include <string.h>
#include <e32std.h>
#include <hal.h>



//========================================
//= Linux, Mac OS (Apple), Solaris (Sun) =
//========================================
#elif defined(__linux__) || defined(__APPLE__) || defined(sun) || defined(__sun)

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <ctime>

//if we have Solaris
#if defined(sun) || defined(__sun)
#include <sched.h>	//	for sched_yield();
#endif

#if defined(__ANDROID__)
//TODO: revert to using sys/eventfd.h when it becomes available in Android NDK
#elif defined(__linux__)
#include <sys/eventfd.h>
#endif

#else
#error "Unsupported OS"
#endif



//if Microsoft MSVC compiler,
//then disable warning about throw specification is ignored.
#if M_COMPILER == M_COMPILER_MSVC
#pragma warning(push) //push warnings state
#pragma warning( disable : 4290)
#endif



//#define M_ENABLE_MUTEX_TRACE
#ifdef M_ENABLE_MUTEX_TRACE
#define M_MUTEX_TRACE(x) TRACE(<< "[MUTEX] ") TRACE(x)
#else
#define M_MUTEX_TRACE(x)
#endif


//#define M_ENABLE_QUEUE_TRACE
#ifdef M_ENABLE_QUEUE_TRACE
#define M_QUEUE_TRACE(x) TRACE(<< "[QUEUE] ") TRACE(x)
#else
#define M_QUEUE_TRACE(x)
#endif


namespace ting{

//forward declarations
class CondVar;
class Queue;
class Thread;
class QuitMessage;

/**
 * @brief Mutex object class
 * Mutex stands for "Mutual execution".
 */
class Mutex{
	friend class CondVar;

	//system dependent handle
#ifdef WIN32
	CRITICAL_SECTION m;
#elif defined(__SYMBIAN32__)
	RCriticalSection m;
#elif defined(__linux__) || defined(__APPLE__)
	pthread_mutex_t m;
#else
#error "unknown system"
#endif

private:
	//forbid copying
	Mutex(const Mutex& );
	Mutex& operator=(const Mutex& );

public:
	/**
	 * @brief Creates initially unlocked mutex.
	 */
	Mutex();

	~Mutex()throw();


	/**
	 * @brief Acquire mutex lock.
	 * If one thread acquired the mutex lock then all other threads
	 * attempting to acquire the lock on the same mutex will wait until the
	 * mutex lock will be released with Mutex::Unlock().
	 */
	inline void Lock()throw(){
		M_MUTEX_TRACE(<< "Mutex::Lock(): invoked " << this << std::endl)
#ifdef WIN32
		EnterCriticalSection(&this->m);
#elif defined(__SYMBIAN32__)
		this->m.Wait();
#elif defined(__linux__) || defined(__APPLE__)
		pthread_mutex_lock(&this->m);
#else
	#error "unknown system"
#endif
	}



	/**
	 * @brief Release mutex lock.
	 */
	inline void Unlock()throw(){
		M_MUTEX_TRACE(<< "Mutex::Unlock(): invoked " << this << std::endl)
#ifdef WIN32
		LeaveCriticalSection(&this->m);
#elif defined(__SYMBIAN32__)
		this->m.Signal();
#elif defined(__linux__) || defined(__APPLE__)
		pthread_mutex_unlock(&this->m);
#else
	#error "unknown system"
#endif
	}



	/**
	 * @brief Helper class which automatically Locks the given mutex.
	 * This helper class automatically locks the given mutex in the constructor and
	 * unlocks the mutex in destructor. This class is useful if the code between
	 * mutex lock/unlock may return or throw an exception,
	 * then the mutex be automaticlly unlocked in such case.
	 */
	class Guard{
		Mutex &mutex;

		//forbid copying
		Guard(const Guard& );
		Guard& operator=(const Guard& );
	public:
		Guard(Mutex &m)throw() :
				mutex(m)
		{
			this->mutex.Lock();
		}
		~Guard()throw(){
			this->mutex.Unlock();
		}
	};//~class Guard
};//~class Mutex



/**
 * @brief Semaphore class.
 * The semaphore is actually an unsigned integer value which can be incremented
 * (by Semaphore::Signal()) or decremented (by Semaphore::Wait()). If the value
 * is 0 then any try to decrement it will result in execution blocking of the current thread
 * until the value is incremented so the thread will be able to
 * decrement it. If there are several threads waiting for semaphore decrement and
 * some other thread increments it then only one of the hanging threads will be
 * resumed, other threads will remain waiting for next increment.
 */
class Semaphore{
	//system dependent handle
#ifdef WIN32
	HANDLE s;
#elif defined(__SYMBIAN32__)
	RSemaphore s;
#elif defined(__APPLE__)
	//TODO: consider using the MPCreateSemaphore
	sem_t *s;
#elif defined(__linux__)
	sem_t s;
#else
#error "unknown system"
#endif

	//forbid copying
	Semaphore(const Semaphore& );
	Semaphore& operator=(const Semaphore& );
    
public:

	/**
	 * @brief Create the semaphore with given initial value.
	 */
	Semaphore(unsigned initialValue = 0);

	~Semaphore()throw();

	
	/**
	 * @brief Wait on semaphore.
	 * Decrements semaphore value. If current value is 0 then this method will wait
	 * until some other thread signals the semaphore (i.e. increments the value)
	 * by calling Semaphore::Signal() on that semaphore.
	 */
	void Wait(){
#ifdef WIN32
		switch(WaitForSingleObject(this->s, DWORD(INFINITE))){
			case WAIT_OBJECT_0:
//				LOG(<<"Semaphore::Wait(): exit"<<std::endl)
				break;
			case WAIT_TIMEOUT:
				ASSERT(false)
				break;
			default:
				throw ting::Exc("Semaphore::Wait(): wait failed");
				break;
		}
#elif defined(__SYMBIAN32__)
		this->s.Wait();
#elif defined(__APPLE__)
		int retVal = 0;

		do{
			retVal = sem_wait(this->s);
		}while(retVal == -1 && errno == EINTR);

		if(retVal < 0){
			throw ting::Exc("Semaphore::Wait(): wait failed");
		}
#elif defined(__linux__)
		int retVal;
		do{
			retVal = sem_wait(&this->s);
		}while(retVal == -1 && errno == EINTR);
		if(retVal < 0){
			throw ting::Exc("Semaphore::Wait(): wait failed");
		}
#else
#error "unknown system"
#endif
	}
	


	/**
	 * @brief Wait on semaphore with timeout.
	 * Decrements semaphore value. If current value is 0 then this method will wait
	 * until some other thread signals the semaphore (i.e. increments the value)
	 * by calling Semaphore::Signal() on that semaphore.
	 * @param timeoutMillis - waiting timeout.
	 *                        If timeoutMillis is 0 (the default value) then this
	 *                        method will try to decrement the semaphore value and exit immediately.
	 * @return returns true if the semaphore value was decremented.
	 * @return returns false if the timeout was hit.
	 */
	bool Wait(ting::u32 timeoutMillis);



	/**
	 * @brief Signal the semaphore.
	 * Increments the semaphore value.
	 * The semaphore value is a 32bit unsigned integer, so it can be a pretty big values.
	 * But, if the maximum value is reached then subsequent calls to this method
	 * will not do any incrementing (because the maximum value is reached), i.e. there will
	 * be no semaphore value warp around to 0 again. Reaching such condition is
	 * considered as an error condition which, in theory, should never occur in the program.
	 * Because of that, in the debug mode (DEBUG macro defined) there are assertions to
	 * detect such a condition.
	 */
	inline void Signal()throw(){
//		TRACE(<< "Semaphore::Signal(): invoked" << std::endl)
#ifdef WIN32
		if(ReleaseSemaphore(this->s, 1, NULL) == 0){
			ASSERT(false)
		}
#elif defined(__SYMBIAN32__)
		this->s.Signal();
#elif defined(__APPLE__)
		if(sem_post(this->s) < 0){
			ASSERT(false)
		}
#elif defined(__linux__)
		if(sem_post(&this->s) < 0){
			ASSERT(false)
		}
#else
	#error "unknown system"
#endif
	}
};//~class Semaphore



class CondVar{
#if defined(WIN32) || defined(__SYMBIAN32__)
	Mutex cvMutex;
	Semaphore semWait;
	Semaphore semDone;
	unsigned numWaiters;
	unsigned numSignals;
#elif defined(__linux__) || defined(__APPLE__)
	//A pointer to store system dependent handle
	pthread_cond_t cond;
#else
#error "unknown system"
#endif

	//forbid copying
	CondVar(const CondVar& );
	CondVar& operator=(const CondVar& );
	
public:

	CondVar();

	~CondVar()throw();

	void Wait(Mutex& mutex){
#if defined(WIN32) || defined(__SYMBIAN32__)
		this->cvMutex.Lock();
		++this->numWaiters;
		this->cvMutex.Unlock();

		mutex.Unlock();

		this->semWait.Wait();

		this->cvMutex.Lock();
		if(this->numSignals > 0){
			this->semDone.Signal();
			--this->numSignals;
		}
		--this->numWaiters;
		this->cvMutex.Unlock();

		mutex.Lock();
#elif defined(__linux__) || defined(__APPLE__)
		pthread_cond_wait(&this->cond, &mutex.m);
#else
#error "unknown system"
#endif
	}

	void Notify()throw(){
#if defined(WIN32) || defined(__SYMBIAN32__)
		this->cvMutex.Lock();

		if(this->numWaiters > this->numSignals){
			++this->numSignals;
			this->semWait.Signal();
			this->cvMutex.Unlock();
			this->semDone.Wait();
		}else{
			this->cvMutex.Unlock();
		}
#elif defined(__linux__) || defined(__APPLE__)
		pthread_cond_signal(&this->cond);
#else
#error "unknown system"
#endif
	}
};



/**
 * @brief Message abstract class.
 * The messages are sent to message queues (see ting::Queue). One message instance cannot be sent to
 * two or more message queues, but only to a single queue. When sent, the message is
 * further owned by the queue (note the usage of ting::Ptr auto-pointers).
 */
class Message{
	friend class Queue;

	Message *next;//pointer to the next message in a single-linked list

protected:
	Message()throw() :
			next(0)
	{}

public:
	virtual ~Message()throw(){}

	/**
	 * @brief message handler function.
	 * This virtual method is called to handle the message. When deriving from ting::Message,
	 * override this method to define the message handler procedure.
	 */
	virtual void Handle() = 0;
};



/**
 * @brief Message queue.
 * Message queue is used for communication of separate threads by
 * means of sending messages to each other. Thus, when one thread sends a message to another one,
 * it asks that another thread to execute some code portion - handler code of the message.
 * NOTE: Queue implements Waitable interface which means that it can be used in conjunction
 * with ting::WaitSet. But, note, that the implementation of the Waitable is that it
 * shall only be used to wait for READ. If you are trying to wait for WRITE the behavior will be
 * undefined.
 */
class Queue : public Waitable{
	Semaphore sem;

	Mutex mut;

	ting::Inited<Message*, 0> first;
	ting::Inited<Message*, 0> last;

#if defined(WIN32)
	//use additional semaphore to implement Waitable on Windows
	HANDLE eventForWaitable;
#elif defined(__APPLE__) || defined(__ANDROID__) //TODO: for Android revert to using eventFD when it becomes available in Android NDK
	//use pipe to implement Waitable in *nix systems
	int pipeEnds[2];
#elif defined(__linux__)
	//use eventfd()
	int eventFD;
#else
#error "Unsupported OS"
#endif

	//forbid copying
	Queue(const Queue&);
	Queue& operator=(const Queue&);

public:
	/**
	 * @brief Constructor, creates empty message queue.
	 */
	Queue();

	
	/**
	 * @brief Destructor.
	 * When called, it also destroys all messages on the queue.
	 */
	~Queue()throw();



	/**
	 * @brief Pushes a new message to the queue.
	 * @param msg - the message to push into the queue.
	 */
	void PushMessage(Ptr<Message> msg)throw();



	/**
	 * @brief Get message from queue, does not block if no messages queued.
	 * This method gets a message from message queue. If there are no messages on the queue
	 * it will return invalid auto pointer.
	 * @return auto-pointer to Message instance.
	 * @return invalid auto-pointer if there are no messages in the queue.
	 */
	Ptr<Message> PeekMsg();



	/**
	 * @brief Get message from queue, blocks if no messages queued.
	 * This method gets a message from message queue. If there are no messages on the queue
	 * it will wait until any message is posted to the queue.
	 * Note, that this method, due to its implementation, is not intended to be called from
	 * multiple threads simultaneously (unlike Queue::PeekMsg()).
	 * If it is called from multiple threads the behavior will be undefined.
	 * It is also forbidden to call GetMsg() from one thread and PeekMsg() from another
	 * thread on the same Queue instance, because it will also lead to undefined behavior.
	 * @return auto-pointer to Message instance.
	 */
	Ptr<Message> GetMsg();

private:
#ifdef WIN32
	//override
	HANDLE GetHandle();

	u32 flagsMask;//flags to wait for

	//override
	virtual void SetWaitingEvents(u32 flagsToWaitFor);

	//returns true if signalled
	//override
	virtual bool CheckSignalled();

#elif defined(__linux__)
	//override
	int GetHandle();

#elif defined(__APPLE__) //Mac OS
	//override
	int GetHandle();

#else
#error "Unsupported OS"
#endif
};//~class Queue



/**
 * @brief a base class for threads.
 * This class should be used as a base class for thread objects, one should override the
 * Thread::Run() method.
 */
class Thread{
//Tread Run function
#ifdef WIN32
	static unsigned int __stdcall RunThread(void *data);
#elif defined(__SYMBIAN32__)
	static TInt RunThread(TAny *data);
#elif defined(__linux__) || defined(__APPLE__)
	static void* RunThread(void *data);
#else
#error "Unsupported OS"
#endif


	ting::Mutex mutex1;


	enum E_State{
		NEW,
		RUNNING,
		STOPPED,
		JOINED
	};
	
	ting::Inited<volatile E_State, NEW> state;

	//system dependent handle
#if defined(WIN32)
	HANDLE th;
#elif defined(__SYMBIAN32__)
	RThread th;
#elif defined(__linux__) || defined(__APPLE__)
	pthread_t th;
#else
#error "Unsupported OS"
#endif

	//forbid copying
	Thread(const Thread& );
	Thread& operator=(const Thread& );

public:
	
	/**
	 * @brief Basic exception type thrown by Thread class.
	 * @param msg - human friendly exception description.
	 */
	class Exc : public ting::Exc{
	public:
		Exc(const std::string& msg) :
				ting::Exc(msg)
		{}
	};
	
	class HasAlreadyBeenStartedExc : public Exc{
	public:
		HasAlreadyBeenStartedExc() :
				Exc("The thread has already been started.")
		{}
	};
	
	Thread();
	
	
	virtual ~Thread()throw();



	/**
	 * @brief This should be overridden, this is what to be run in new thread.
	 * Pure virtual method, it is called in new thread when thread runs.
	 */
	virtual void Run() = 0;



	/**
	 * @brief Start thread execution.
	 * Starts execution of the thread. Thread's Thread::Run() method will
	 * be run as separate thread of execution.
	 * @param stackSize - size of the stack in bytes which should be allocated for this thread.
	 *                    If stackSize is 0 then system default stack size is used
	 *                    (stack size depends on underlying OS).
	 */
	void Start(size_t stackSize = 0);



	/**
	 * @brief Wait for thread to finish its execution.
	 * This function waits for the thread finishes its execution,
	 * i.e. until the thread returns from its Thread::Run() method.
	 * Note: it is safe to call Join() on not started threads,
	 *       in that case it will return immediately.
	 */
	void Join()throw();



	/**
	 * @brief Suspend the thread for a given number of milliseconds.
	 * Suspends the thread which called this function for a given number of milliseconds.
	 * This function guarantees that the calling thread will be suspended for
	 * AT LEAST 'msec' milliseconds.
	 * @param msec - number of milliseconds the thread should be suspended.
	 */
	static void Sleep(unsigned msec = 0)throw(){
#ifdef WIN32
		SleepEx(DWORD(msec), FALSE);// Sleep() crashes on MinGW (I do not know why), this is why SleepEx() is used here.
#elif defined(__SYMBIAN32__)
		User::After(msec * 1000);
#elif defined(sun) || defined(__sun) || defined(__APPLE__) || defined(__linux__)
		if(msec == 0){
	#if defined(sun) || defined(__sun) || defined(__APPLE__) || defined(__ANDROID__)
			sched_yield();
	#elif defined(__linux__)
			pthread_yield();
	#else
	#error "Should not get here"
	#endif
		}else{
			usleep(msec * 1000);
		}
#else
	#error "Unsupported OS"
#endif
	}



	/**
	 * @brief Thread ID type.
	 * Thread ID type is used to identify a thread.
	 * The type supports operator==() and operator!=() operators.
	 */
	typedef unsigned long int T_ThreadID;



	/**
	 * @brief get current thread ID.
	 * Returns unique identifier of the currently executing thread. This ID can further be used
	 * to make assertions to make sure that some code is executed in a specific thread. E.g.
	 * assert that methods of some object are executed in the same thread where this object was
	 * created.
	 * @return unique thread identifier.
	 */
	static inline T_ThreadID GetCurrentThreadID()throw(){
#ifdef WIN32
		return T_ThreadID(GetCurrentThreadId());
#elif defined(__APPLE__) || defined(__linux__)
		pthread_t t = pthread_self();
		STATIC_ASSERT(sizeof(pthread_t) <= sizeof(T_ThreadID))
		return T_ThreadID(t);
#else
	#error "Unsupported OS"
#endif
	}
};



/**
 * @brief a thread with message queue.
 * This is just a facility class which already contains message queue and boolean 'quit' flag.
 */
class MsgThread : public Thread{
	friend class QuitMessage;
	
protected:
	/**
	 * @brief Flag indicating that the thread should exit.
	 * This is a flag used to stop thread execution. The implementor of
	 * Thread::Run() method usually would want to use this flag as indicator
	 * of thread exit request. If this flag is set to true then the thread is requested to exit.
	 * The typical usage of the flag is as follows:
	 * @code
	 * class MyThread : public ting::MsgThread{
	 *     ...
	 *     void MyThread::Run(){
	 *         while(!this->quitFlag){
	 *             //get and handle thread messages, etc.
	 *             ...
	 *         }
	 *     }
	 *     ...
	 * };
	 * @endcode
	 */
	ting::Inited<volatile bool, false> quitFlag;//looks like it is not necessary to protect this flag by mutex, volatile will be enough

	Queue queue;
	
	ting::Ptr<ting::QuitMessage> quitMessage;

public:
	inline MsgThread();

	
	
	/**
	 * @brief Send preallocated 'Quit' message to thread's queue.
	 * This function throws no exceptions. It can send the quit message only once.
	 */
	void PushPreallocatedQuitMessage()throw();
	
	
	
	/**
	 * @brief Send 'Quit' message to thread's queue.
	 */
	inline void PushQuitMessage();//see implementation below



	/**
	 * @brief Send "no operation" message to thread's queue.
	 */
	inline void PushNopMessage();//see implementation below



	/**
	 * @brief Send a message to thread's queue.
	 * @param msg - a message to send.
	 */
	inline void PushMessage(Ptr<ting::Message> msg)throw(){
		this->queue.PushMessage(msg);
	}
};



/**
 * @brief Tells the thread that it should quit its execution.
 * The handler of this message sets the quit flag (Thread::quitFlag)
 * of the thread which this message is sent to.
 */
class QuitMessage : public Message{
	MsgThread *thr;
public:
	QuitMessage(MsgThread* thread) :
			thr(thread)
	{
		if(!this->thr){
			throw ting::Exc("QuitMessage::QuitMessage(): thread pointer passed is 0");
		}
	}

	//override
	void Handle(){
		this->thr->quitFlag = true;
	}
};



/**
 * @brief No operation message.
 * The handler of this message does nothing when the message is handled. This message
 * can be used to unblock thread which is waiting infinitely on its message queue.
 */
class NopMessage : public Message{
public:
	NopMessage(){}

	//override
	void Handle(){
		//Do nothing
	}
};



inline MsgThread::MsgThread() :
		quitMessage(new ting::QuitMessage(this))
{}


inline void MsgThread::PushNopMessage(){
	this->PushMessage(Ptr<Message>(new NopMessage()));
}



inline void MsgThread::PushQuitMessage(){
	this->PushMessage(Ptr<Message>(new QuitMessage(this)));
}



}//~namespace ting

//NOTE: do not put semicolon after namespace, some compilers issue a warning on this thinking that it is a declaration.



//if Microsoft MSVC compiler, restore warnings state
#if M_COMPILER == M_COMPILER_MSVC
#pragma warning(pop) //pop warnings state
#endif


