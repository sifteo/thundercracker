#pragma once

#include "TotalsCube.h"

namespace TotalsGame
{

class DialogWindow {
private:
    TotalsCube* mCube;
    Vector2<int> mPosition;
    uint16_t fg, bg;


    const char* Show(const char* msg);
    void DrawGlyph(char ch);
    unsigned MeasureGlyph(char ch);
    void DrawText(const char* msg);
    void MeasureText(const char *str, unsigned *outCount, unsigned *outPx);
    void Erase();
    void Fade();


public:
    DialogWindow(TotalsCube *mCube);
    TotalsCube* GetCube() const { return mCube; }
    void SetBackgroundColor(unsigned r, unsigned g, unsigned b);
    void SetForegroundColor(unsigned r, unsigned g, unsigned b);
    void DoDialog(const char *text, int yTop, int ySize);
    void EndIt();
};

}
