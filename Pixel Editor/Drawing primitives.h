#pragma once

#include <SDL.h>
#include "SDLG.h"

using namespace SDLG;

static void SetDrawColour(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255) {
	SDL_SetRenderDrawColor(gameRenderer, r, g, b, a);
}
static void SetDrawColour(SDL_Colour colour) {
	SetDrawColour(colour.r, colour.g, colour.b, colour.a);
}
 
static void Clear() {
	SDL_RenderClear(gameRenderer);
}
 
static void DrawRect(SDL_Rect r) {
	SDL_RenderDrawRect(gameRenderer, &r);
}
static void DrawRect(SDL_FRect r) {
	SDL_RenderDrawRectF(gameRenderer, &r);
}
 
static void FillRect(SDL_Rect r) {
	SDL_RenderFillRect(gameRenderer, &r);
}
static void FillRect(SDL_FRect r) {
	SDL_RenderFillRectF(gameRenderer, &r);
}

static void DrawPoint(int x, int y) {
	SDL_RenderDrawPoint(gameRenderer, x, y);
}
static void DrawPoint(SDL_Point& point) {
	DrawPoint(point.x, point.y);
}