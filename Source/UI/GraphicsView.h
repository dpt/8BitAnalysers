#pragma once

#include "imgui.h"
#include <cstdint>

// Graphics View Code - TODO: Move
struct FGraphicsView
{
	int				Width = 0;
	int				Height = 0;
	uint32_t*		PixelBuffer = nullptr;
	ImTextureID		Texture = nullptr;
};

FGraphicsView *CreateGraphicsView(int width, int height);
void ClearGraphicsView(const FGraphicsView &graphicsView, const uint32_t col);
void DrawGraphicsView(const FGraphicsView &graphicsView, const ImVec2 &size);
void DrawGraphicsView(const FGraphicsView &graphicsView);

