#pragma once

#include "TotalsCube.h"

namespace TotalsGame
{

class DialogWindow {
private:
    TotalsCube* mCube;
    Int2 mPosition;
    Sifteo::RGB565 fg, bg;


    const char* Show(const char* msg);
    void DrawGlyph(char ch);
    unsigned MeasureGlyph(char ch);
    void DrawText(const char* msg);
    void MeasureText(const char *str, unsigned *outCount, unsigned *outPx);

public:
    DialogWindow(TotalsCube *mCube);
    TotalsCube* GetCube() const { return mCube; }
    void SetBackgroundColor(uint8_t r, uint8_t g, uint8_t b);
    void SetForegroundColor(uint8_t r, uint8_t g, uint8_t b);
    void DoDialog(const char *text, int yTop, int ySize);
    void EndIt();
};

}
