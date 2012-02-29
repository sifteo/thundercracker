#pragma once

#define CORO_PARAMS int mState;float mTimer;
#define CORO_RESET mState=0, mTimer=0;
#define CORO_BEGIN switch(mState) { case 0:;
#define CORO_YIELD(result) do{mState=__LINE__; return(result); case __LINE__:;}while(0)
#define CORO_END mState=-1;case -1:;}
#if 0
#define CORO_WAIT(delay) do{time=System::clock();do{do{mState=__LINE__; return; case __LINE__:;}while(0);}while(System::clock()-time<delay);}while(0)
#endif

#define UPDATE_CORO(c) do{mTimer-=Game::dt;if(mTimer<=0){mTimer=c();}}while(0)
