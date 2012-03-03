#pragma once

#define CORO_YIELD(delay) do{float t=System::clock();;do{Game::UpdateCubeViews();Game::PaintCubeViews();System::paint();Game::UpdateDt();}while(System::clock()<t+delay);}while(0)
