#pragma once

#include "Base.h"

void DialogTest();

class DialogView {
private:
	Cube* mCube;
	Vec2 mPosition;

public:
	DialogView(Cube* pCube);
	Cube* GetCube() const { return mCube; }
	void Show(const char* msg);
	void DrawGlyph(char ch);
	unsigned MeasureGlyph(char ch);
	void DrawText(const char* msg);
	unsigned MeasureText(const char* msg);
	void ResetY();
	void Erase();

};