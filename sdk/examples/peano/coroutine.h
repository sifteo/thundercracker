#pragma once

#define CORO_PARAMS
#define CORO_RESET
#define CORO_BEGIN do{OnSetup();}while(0)
#define CORO_YIELD(delay) do{float t=System::clock();;do{Game::UpdateCubeViews();Game::PaintCubeViews();System::paint();Game::UpdateDt();}while(System::clock()<t+delay);}while(0)
#define CORO_END do{OnDispose();}while(0)

#define UPDATE_CORO(c) do{c();}while(0)
