#pragma once

#define CORO_PARAMS int mState;
#define CORO_RESET mState=0
#define CORO_BEGIN switch(mState) { case 0:;
#define CORO_YIELD do{mState=__LINE__; return; case __LINE__:;}while(0)
#define CORO_END mState=-1;case -1:;}

#define CORO_WAIT(delay) do{time=System::clock();do{do{mState=__LINE__; return; case __LINE__:;}while(0);}while(System::clock()-time<delay);}while(0)