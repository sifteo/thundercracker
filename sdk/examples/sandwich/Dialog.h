#pragma once

#include "Common.h"
#include "Content.h"

class DialogView {
private:
    Cube* mCube;
    Vec2 mPosition;

public:
    DialogView(Cube *mCube);
    void Init();
    Cube* GetCube() const { return mCube; }
    void ShowAll(const char* lines);
    const char* Show(const char* msg);
    void Erase();
    void Fade();
    void SetFadeAmount(uint8_t alpha);

private:    
    void DrawGlyph(char ch);
    unsigned MeasureGlyph(char ch);
    void DrawText(const char* msg);
    void MeasureText(const char *str, unsigned *outCount, unsigned *outPx);

};

void DoDialog(const DialogData& data, Cube* cube=0);
