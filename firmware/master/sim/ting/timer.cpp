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



#include "timer.hpp"



using namespace ting::timer;



ting::IntrusiveSingleton<Lib>::T_Instance Lib::instance;



bool Lib::TimerThread::RemoveTimer_ts(Timer* timer)throw(){
	ASSERT(timer)
	ting::Mutex::Guard mutexGuard(this->mutex);

	if(!timer->isRunning){
		//lock and unlock the 'expired' mutex to make sure that the timer's callback
		//has been called if the timer has expired and is awaiting the notification callback to be called.
		ting::Mutex::Guard mutexGuard(this->expiredTimersNotifyMutex);
		return false;
	}

	//if isStarted flag is set then the timer will be stopped now, so
	//change the flag
	timer->isRunning = false;

	ASSERT(timer->i != this->timers.end())

	//if that was the first timer, signal the semaphore about timer deletion in order to recalculate the waiting time
	if(this->timers.begin() == timer->i){
		this->sema.Signal();
	}

	this->timers.erase(timer->i);

	//was running
	return true;
}



void Lib::TimerThread::AddTimer_ts(Timer* timer, u32 timeout){
	ASSERT(timer)
	ting::Mutex::Guard mutexGuard(this->mutex);

	if(timer->isRunning){
		throw ting::Exc("Lib::TimerThread::AddTimer(): timer is already running!");
	}

	timer->isRunning = true;

	ting::u64 stopTicks = this->GetTicks() + ting::u64(timeout);

	timer->i = this->timers.insert(
			std::pair<ting::u64, Timer*>(stopTicks, timer)
		);

	ASSERT(timer->i != this->timers.end())
	ASSERT(timer->i->second)

	//signal the semaphore about new timer addition in order to recalculate the waiting time
	this->sema.Signal();
}



//override
void Lib::TimerThread::Run(){
	M_TIMER_TRACE(<< "Lib::TimerThread::Run(): enter" << std::endl)

	while(!this->quitFlag){
		ting::u32 millis;

		while(true){
			std::vector<Timer*> expiredTimers;

			{
				ting::Mutex::Guard mutexGuard(this->mutex);

				ting::u64 ticks = this->GetTicks();

				for(Timer::T_TimerIter b = this->timers.begin(); b != this->timers.end(); b = this->timers.begin()){
					if(b->first > ticks){
						break;//~for
					}

					Timer *timer = b->second;
					//add the timer to list of expired timers
					ASSERT(timer)
					expiredTimers.push_back(timer);

					//Change the expired timer state to not running.
					//This should be done before the expired signal of the timer will be emitted.
					timer->isRunning = false;

					this->timers.erase(b);
				}

				if(expiredTimers.size() == 0){
					ASSERT(this->timers.size() > 0) //if we have no expired timers here, then at least one timer should be running (the half-max-ticks timer).

					//calculate new waiting time
					ASSERT(this->timers.begin()->first > ticks)
					ASSERT(this->timers.begin()->first - ticks <= ting::u64(ting::u32(-1)))
					millis = ting::u32(this->timers.begin()->first - ticks);

					//zero out the semaphore for optimization purposes
					while(this->sema.Wait(0)){}

					break;//~while(true)
				}
				
				this->expiredTimersNotifyMutex.Lock();
			}

			try{
				//emit expired signal for expired timers
				for(std::vector<Timer*>::iterator i = expiredTimers.begin(); i != expiredTimers.end(); ++i){
					ASSERT(*i)
					(*i)->OnExpired();
				}
			}catch(...){
				//no exceptions should be thrown by this code. Especially we don't want them here because
				//mutex locking/unlocking is used without mutex guard.
				ASSERT(false)
			}
			
			this->expiredTimersNotifyMutex.Unlock();
		}

		this->sema.Wait(millis);
	}//~while(!this->quitFlag)

	M_TIMER_TRACE(<< "Lib::TimerThread::Run(): exit" << std::endl)
}//~Run()
