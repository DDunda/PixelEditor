#include <SDL.h>
#include <SDL_image.h>

#include <queue>

#define ERROR_LOGGING
#include "SDLG.h"

#include "Drawing primitives.h"

#include "AbstractedAccess.h"
#include "RenderableElement.h"
#include "InteractiveElement.h"
#include "Generic.h"

#define swap(a,b) a ^= (b ^= (a ^= b))

#define TRANSPARENT_CHECKER_1 (0xFFFFFFFF)
#define TRANSPARENT_CHECKER_2 (0xFFBFBFBF)
#define SHADOW (0x7F000000)

using namespace SDLG;

int main(int argc, char* argv[]) { return StartSDL(); }

void DrawTexture(SDL_Texture* txt, SDL_FRect dst) {
	SDL_RenderCopyF(gameRenderer, txt, NULL, &dst);
}

void DrawTexture(SDL_Texture* txt, SDL_Rect dst) {
	SDL_RenderCopy(gameRenderer, txt, NULL, &dst);
}

struct textCharacter {
	SDL_Rect src;
	SDL_FRect dst; // x/y is the offset, and w/h is a proportion of the scale, both relative to the scale of the text.
};

struct textLayout {
	textCharacter characters[256];
};

class TextRenderer {
private:
	SDL_Texture** fontSrc;
	SDL_Texture* drawTarget;
	std::string currentText;

	int boundsWidth, boundsHeight;

	bool textWrap = true;

	void RenderWord(std::string& word, textLayout& font) {

	}

	SDL_Texture* RenderText(std::string& text, textLayout& font, int textScale, int maxWidth, int maxHeight, bool textCutoff = false, bool monospace = false) {
		if (maxWidth <= 0 || maxHeight <= 0) return NULL;

		SDL_CreateTexture(gameRenderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, maxWidth, maxHeight);
	}

public:
};

SDL_Rect ToRect(SDL_FRect rect) {
	return { (int)round(rect.x), (int)round(rect.y), (int)round(rect.w), (int)round(rect.h) };
}
SDL_FRect GetFrameRect(frame& f) {
	SDL_FRect parentRect;

	if (f.parent == NULL) parentRect = { 0, 0, (float)windowWidth, (float)windowHeight };
	else parentRect = GetFrameRect(*f.parent);

	float width = f.relativeScale.x * parentRect.w + f.absoluteScale.x;
	float height = f.relativeScale.y * parentRect.h + f.absoluteScale.y;
	float x = parentRect.x + parentRect.w * f.parentRelativeOrigin.x - width * f.selfRelativeOrigin.x + f.absoluteOffset.x;
	float y = parentRect.y + parentRect.h * f.parentRelativeOrigin.y - height * f.selfRelativeOrigin.y + f.absoluteOffset.y;

	return { x,y,width,height };
}

void RenderSprite(sprite& src, frame& dst) {
	if (src.texture == NULL || *src.texture == NULL) {
		SetDrawColour(255, 0, 255); // A simple magenta box
		FillRect(GetFrameRect(dst));
		return;
	}

	SDL_FRect dstRect = GetFrameRect(dst);

	SDL_RenderCopyF(gameRenderer, *src.texture, &src.src, &dstRect);
}

enum class ScreenState {
	CreateImage,
	DrawImage
};

ScreenState gameState = ScreenState::CreateImage;

class DrawCanvas : public RenderableElement {
protected:
	Uint8* appliedData;
	Uint8* modifiedData;
	SDL_Surface* surface;
	SDL_Texture* renderedSurface;
	SDL_Colour palette[256];
	SDL_Palette* surfacePalette;
	frame canvasArea;
	unsigned width, height, zoom;
	bool rendered = false;

	constexpr unsigned GetIndex(unsigned x, unsigned y) {
		return x + y * width;
	}

public:
	unsigned GetImageWidth() {
		return width;
	}

	unsigned GetImageHeight() {
		return height;
	}

	SDL_Rect GetBounds() {
		return ToRect(GetFrameRect(canvasArea));
	}

	void RenderCanvas() {
		/*SDL_Colour* pixels;
		int pitch;

		if (SDL_LockTexture(surface, NULL, (void**)&pixels, &pitch) == -1)
			return;
		unsigned long i = 0;
		for (unsigned y = 0; y < height; y++)
			for (unsigned x = 0; x < width; x++)
				*(pixels++) = palette[modifiedData[i++]];
		SDL_UnlockTexture(surface);*/

		rendered = true;
		SDL_Surface* tmpSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
		SDL_UpdateTexture(renderedSurface, NULL, tmpSurface->pixels, tmpSurface->pitch);
	}

	SDL_Colour GetPaletteColour(Uint8 index) {
		return palette[index];
	}

	void SetPaletteColour(SDL_Colour colour, Uint8 index) {
		/*palette[index] = colour;

		SDL_Colour* pixels;
		int pitch;

		SDL_LockTexture(surface, NULL, (void**)&pixels, &pitch);
		unsigned long i = 0;
		for (unsigned y = 0; y < height; y++)
			for (unsigned x = 0; x < width; x++) {
				if (modifiedData[i++] == index) *pixels = colour;
				pixels++;
			}
		SDL_UnlockTexture(surface);*/

		palette[index] = colour;

		SDL_SetPaletteColors(surface->format->palette, palette + index, index, 1);

		rendered = false;
	}

	void DrawPoint(Uint8 colourIndex, unsigned x, unsigned y) {
		/*if (!InBounds({ 0,0,(int)width,(int)height }, x, y)) return;

		modifiedData[GetIndex(x,y)] = colourIndex;
		
		SDL_Colour* pixels;
		int pitch;

		SDL_LockTexture(renderedSurface, NULL, (void**)&pixels, &pitch);
		pixels[GetIndex(x,y)] = palette[colourIndex];
		SDL_UnlockTexture(renderedSurface);*/

		if (!InBounds({ 0,0,(int)width,(int)height }, x, y)) return;

		modifiedData[GetIndex(x,y)] = colourIndex;

		rendered = false;
	}

	int GetPixel(unsigned x, unsigned y) {
		if (!InBounds({ 0,0,(int)width,(int)height }, x, y)) return -1;

		return modifiedData[GetIndex(x, y)];
	}

	constexpr unsigned GetZoom() {
		return zoom;
	}

	void SetZoom(int zoom) {
		if (zoom <= 0) zoom = 1;
		this->zoom = zoom;
		canvasArea = {
			{0.5, 0.5},
			{0.5, 0.5},

			{0.0, 0.0},

			{(float)width * zoom, (float)height * zoom},
			{0.0, 0.0},
		};
	}

	DrawCanvas(unsigned W, unsigned H) {
		width = W;
		height = H;

		appliedData = new Uint8[W * H];
		memset(appliedData, 0, W * H);
		modifiedData = new Uint8[W * H];
		memcpy(modifiedData, appliedData, W * H);

		surface = SDL_CreateRGBSurfaceFrom(modifiedData, W, H, 8, W, 0, 0, 0, 0);

		renderedSurface = SDL_CreateTexture(gameRenderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, W, H);

		surfacePalette = SDL_AllocPalette(256);
		SDL_SetPaletteColors(surfacePalette, palette, 0, 256);
		SDL_SetSurfacePalette(surface, surfacePalette);

		SetPaletteColour({ 255,   0,   0, 255 }, 0);
		SetPaletteColour({ 255, 255, 255, 255 }, 1);
		SetPaletteColour({   0, 255, 255, 255 }, 2);

		SetZoom(16);
	}

	~DrawCanvas() {
		delete[] appliedData;
		delete[] modifiedData;
	}

	void render(SDL_Renderer* r) {
		if(!rendered) RenderCanvas();
		DrawTexture(renderedSurface,GetFrameRect(canvasArea));
	}

	void Fill(int x, int y, Uint8 newColour) {
		rendered = false;

		Uint8 oldColour = modifiedData[GetIndex(x, y)];
		if (oldColour == newColour)
			return;

		std::queue<SDL_Point> Q;
		Q.push({ x,y });

		SDL_Rect bounds = { 0,0,width,height };

		while (!Q.empty()) {
			SDL_Point w, e;
			e = Q.front();
			w = e;
			Q.pop();

			int i = GetIndex(w.x, w.y);

			if (modifiedData[i] == newColour) continue;

			int i1, i2;
			i1 = i;
			i2 = i1;
			while (w.x - 1 >= 0 && modifiedData[i1 - 1] == oldColour) {
				w.x--;
				i1--;
			}
			while (e.x + 1 < width && modifiedData[i2 + 1] == oldColour) {
				e.x++;
				i2++;
			}

			int x = w.x;
			for (int i = i1; i <= i2; i++, x++) {
				modifiedData[i] = newColour;
				if (w.y + 1 < height && modifiedData[i + width] == oldColour) Q.push({ x, w.y + 1 });
				if (w.y - 1 >= 0 && modifiedData[i - width] == oldColour) Q.push({ x, w.y - 1 });
			}
		}
	}

	// https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
	void DrawLine(int x0, int y0, int x1, int y1, Uint8 colour) {
		rendered = false;

		int dx = abs(x1 - x0);
		int sx = x0 < x1 ? 1 : -1;
		int dy = -abs(y1 - y0);
		int sy = y0 < y1 ? 1 : -1;
		int err = dx + dy;
		while (1) {
			modifiedData[GetIndex(x0,y0)] = colour;
			if (x0 == x1 && y0 == y1) return;
			int e2 = 2 * err;
			if (e2 >= dy) {
				err += dy;
				x0 += sx;
			}
			if (e2 <= dx) {
				err += dx;
				y0 += sy;
			}
		}
	}

	SDL_Point MapToTexture(SDL_Point screenspace) {
		SDL_FRect canvasBounds = GetFrameRect(canvasArea);
		return{
			(screenspace.x - (int)canvasBounds.x) * (int)width / (int)canvasBounds.w,
			(screenspace.y - (int)canvasBounds.y) * (int)height / (int)canvasBounds.h
		};
	}

	void ApplyChanges() {

	}

	void RevertChanges() {

	}
};

class PaletteRenderer : public RenderableElement {
protected:
	SDL_FRect frameRect{ 0,0,0,0 };
	frame drawFrame;
	SDL_Colour pPalette[256];
	SDL_Surface* rawPaletteSurface;
	SDL_Texture* paletteArea = NULL;
	SDL_Texture* transparentLayer = NULL;
	SDL_Colour gridColour = {12, 23, 39, 255};

	unsigned scale = 0;

	void CreatePaletteSurface() {
		rawPaletteSurface = SDL_CreateRGBSurface(0, 16, 16, 8, 0, 0, 0, 0);
		if (rawPaletteSurface->format->palette == NULL) SDL_SetSurfacePalette(rawPaletteSurface, SDL_AllocPalette(256));
		SDL_SetPaletteColors(rawPaletteSurface->format->palette, pPalette, 0, 256);
		Uint8* ptr = (Uint8*)(rawPaletteSurface->pixels);
		for (int i = 0; i < 256; i++) *(ptr++) = i;
	}

	void RenderPalette() {
		SDL_SetPaletteColors(rawPaletteSurface->format->palette, pPalette, 0, 256);

		SDL_Surface* tmpSurface = SDL_CreateRGBSurfaceWithFormat(0, 16, 16, 32, SDL_PIXELFORMAT_RGBA32);
		SDL_FillRect(tmpSurface, NULL, 0);
		SDL_Rect src{ 0,0,16,16 };
		SDL_BlitSurface(rawPaletteSurface, &src, tmpSurface, &src);
		
		if (paletteArea != NULL) SDL_DestroyTexture(paletteArea);

		paletteArea = SDL_CreateTextureFromSurface(gameRenderer, tmpSurface);
		SDL_SetTextureBlendMode(paletteArea, SDL_BlendMode::SDL_BLENDMODE_BLEND);
		SDL_FreeSurface(tmpSurface);
	}

	void RenderTransparentLayer() {
		if (transparentLayer != NULL) return;

		SDL_Surface* tmpSurface = SDL_CreateRGBSurfaceWithFormat(0, 32, 32, 32, SDL_PIXELFORMAT_RGBA32);

		Uint32* pixels = (Uint32*)(tmpSurface->pixels);
		for (int y = 0; y < 32; y++)
			for (int x = 0; x < 32; x++)
				*(pixels++) = (x ^ y) & 1 ? TRANSPARENT_CHECKER_1 : TRANSPARENT_CHECKER_2;

		transparentLayer = SDL_CreateTextureFromSurface(gameRenderer, tmpSurface);
		SDL_SetTextureBlendMode(transparentLayer, SDL_BlendMode::SDL_BLENDMODE_BLEND);
	}

	bool PaletteChanged() {
		bool changes;
		for (int i = 0; i < 256; i++) {
			SDL_Colour pColour = pPalette[i];
			SDL_Colour colour = parent->GetPaletteColour(i);
			if (
				pColour.r != colour.r ||
				pColour.g != colour.g ||
				pColour.b != colour.b ||
				pColour.a != colour.a
				) {
				changes = true;
				pPalette[i] = colour;
			}
		}
		return changes;
	}

	void DrawGrid() {
		SetDrawColour(gridColour);

		SDL_Point upperleft = { frameRect.x, frameRect.y };

		// Horizontal
		for (int y = 0; y < 17; y++) DrawLine(upperleft.x, upperleft.y + y * 17, upperleft.x + 272, upperleft.y + y * 17);

		// Vertical
		for (int x = 0; x < 17; x++) DrawLine(upperleft.x + x * 17, upperleft.y, upperleft.x + x * 17, upperleft.y + 272);
	}

	void DrawShadow() {
		SDL_SetRenderDrawBlendMode(gameRenderer, SDL_BLENDMODE_BLEND);
		SetDrawColour(shadowColour);

		FillRect(SDL_FRect{
			frameRect.x + shadowOffset.x,
			frameRect.y + shadowOffset.y,
			frameRect.w,
			frameRect.h
			});
	}

public:
	DrawCanvas* parent;
	bool visible = true;
	SDL_Colour shadowColour = {0,0,0,127};
	SDL_FPoint shadowOffset = {8,8};

	void SetScale(unsigned s) {
		if (s != scale) {
			drawFrame = {
				{1, 1},
				{1, 1},

				{0,0},

				{ s * 16.0f + 1, s * 16.0f + 1 },
				{ -16, -16 }
			};
		}
	}

	PaletteRenderer(DrawCanvas& p, unsigned s = 17) : parent(&p) {
		RenderTransparentLayer();
		CreatePaletteSurface();
		RenderPalette();

		SetScale(s);
	}

	~PaletteRenderer() {
		SDL_DestroyTexture(paletteArea);
		SDL_DestroyTexture(transparentLayer);
		SDL_FreeSurface(rawPaletteSurface);
	}

	void render(SDL_Renderer* r) {
		if (PaletteChanged()) RenderPalette();
		frameRect = GetFrameRect(drawFrame);
		DrawShadow();
		DrawTexture(transparentLayer, frameRect);
		DrawTexture(paletteArea, frameRect);
		DrawGrid();
	}
};

DrawCanvas* canvas;
PaletteRenderer* palette;

enum class ToolType {
	Pencil,
	Line,
	Fill
};

ToolType currentTool = ToolType::Pencil;
Uint8 LeftColour = 1;
Uint8 RightColour = 2;
bool LeftDrawing = false;
bool RightDrawing = false;

void DisablePencil() {
	LeftDrawing = false;
	RightDrawing = false;
}

void DisableFill() {}

void EnablePencil() {}

void EnableFill() {}

void PencilLogic() {
	SDL_Rect canvasBounds = canvas->GetBounds();

	bool mouseMovement = mouseXDelta || mouseYDelta;

	if (!RightDrawing && buttonPressed(SDL_BUTTON_LEFT) && InBounds(canvasBounds, mouseX, mouseY)) {
		LeftDrawing = true;

		SDL_Point coords = canvas->MapToTexture({ mouseX, mouseY });

		canvas->DrawPoint(LeftColour, coords.x, coords.y);
	}
	if (buttonReleased(SDL_BUTTON_LEFT)) {
		LeftDrawing = false;
	}
	if (LeftDrawing && mouseMovement) {
		SDL_Point coords1 = canvas->MapToTexture({ mouseXPrev, mouseYPrev });
		SDL_Point coords2 = canvas->MapToTexture({ mouseX, mouseY });

		canvas->DrawLine(coords1.x, coords1.y, coords2.x, coords2.y, LeftColour);
	}

	if (!LeftDrawing && buttonPressed(SDL_BUTTON_RIGHT) && InBounds(canvasBounds, mouseX, mouseY)) {
		RightDrawing = true;
		canvas->DrawPoint(RightColour,
			(mouseX - canvasBounds.x) * canvas->GetImageWidth() / canvasBounds.w,
			(mouseY - canvasBounds.y) * canvas->GetImageHeight() / canvasBounds.h
		);
	}
	if (RightDrawing && buttonReleased(SDL_BUTTON_RIGHT)) {
		RightDrawing = false;
	}
	if (RightDrawing && mouseMovement) {

		int mX1 = (mouseXPrev - canvasBounds.x) * (int)canvas->GetImageWidth() / canvasBounds.w;
		int mX2 = (mouseX - canvasBounds.x) * (int)canvas->GetImageWidth() / canvasBounds.w;

		int mY1 = (mouseYPrev - canvasBounds.y) * (int)canvas->GetImageHeight() / canvasBounds.h;
		int mY2 = (mouseY - canvasBounds.y) * (int)canvas->GetImageHeight() / canvasBounds.h;

		canvas->DrawLine(mX1, mY1, mX2, mY2, RightColour);
	}
}

void FillLogic() {
	SDL_Rect canvasBounds = canvas->GetBounds();
	SDL_Point coords = canvas->MapToTexture({ mouseX,mouseY });

	if (!InBounds(canvasBounds, mouseX, mouseY)) return;

	if (buttonPressed(SDL_BUTTON_LEFT))
		canvas->Fill(coords.x, coords.y, LeftColour);

	if (buttonPressed(SDL_BUTTON_RIGHT))
		canvas->Fill(coords.x, coords.y, RightColour);
}

void SwitchTool(ToolType type) {
	if (currentTool == type) return;

	switch (currentTool)
	{
	case ToolType::Pencil:
		DisablePencil();
		break;
	case ToolType::Line:
		break;
	case ToolType::Fill:
		DisableFill();
		break;
	default:
		break;
	}

	switch (type)
	{
	case ToolType::Pencil:
		EnablePencil();
		break;
	case ToolType::Line:
		break;
	case ToolType::Fill:
		EnableFill();
		break;
	default:
		break;
	}

	currentTool = type;
}

void DrawLogic() {
	if (mouseWheelYDelta)
		canvas->SetZoom(canvas->GetZoom() + mouseWheelYDelta);

	switch (currentTool)
	{
	case ToolType::Pencil:
		PencilLogic();
		break;
	case ToolType::Line:
		break;
	case ToolType::Fill:
		FillLogic();
		break;
	}
}

SDL_Colour HSVColour(int hue, float saturation, float value) {
	//   _    _
	//R:  \__/

	//    __
	//G: /  \__

	//      __
	//B: __/  \

	//Hue ->

	Uint8 R = 0, G = 0, B = 0;
	hue = (hue % (255 * 6)) + (255 * 6) % (255 * 6);

	if (hue < 256) {
		R = 255;
		G = hue;
	}
	else if ((hue -= 255) < 256) {
		R = 255 - hue;
		G = 255;
	}
	else if ((hue -= 255) < 256) {
		G = 255;
		B = hue;
	}
	else if ((hue -= 255) < 256) {
		G = 255 - hue;
		B = 255;
	}
	else if ((hue -= 255) < 256) {
		R = hue;
		B = 255;
	}
	else {
		R = 255;
		B = 255 - hue;
	}

	R = 255 - (255 - R) * saturation;
	G = 255 - (255 - G) * saturation;
	B = 255 - (255 - B) * saturation;

	R *= value;
	G *= value;
	B *= value;

	return { R,G,B,255 };
}

void DoLogic() {
	InteractiveElement::UpdateElementFocus();
	RenderableElement::UpdateAllElements();
	Updater::updateAllSources();

	if (keyPressed(SDLK_f))
		SwitchTool(ToolType::Fill);

	if (keyPressed(SDLK_p))
		SwitchTool(ToolType::Pencil);

	switch (gameState)
	{
	case ScreenState::CreateImage:
		break;
	case ScreenState::DrawImage:
		canvas->SetPaletteColour(HSVColour(currentTime * (6 * 256) / 5000, 1, 1), 1);
		DrawLogic();
		break;
	}
}

void DoDraw() {
	switch (gameState)
	{
	case ScreenState::CreateImage: {
		SetDrawColour(0, 0, 0);
		Clear();

		frame Rect1Border = {
			{0.25f, 0.5f},
			{0.5f,  0.5f},

			{0.25f, 0.5f},

			{10, 10},
			{0, 0}
		};

		SetDrawColour(145, 145, 145);
		FillRect(GetFrameRect(Rect1Border));

		frame Rect1Fill = {
			{0.5f,  0.5f},
			{0.5f,  0.5f},

			{1.0f,  1.0f},

			{-8, -8},
			{0,   0},

			&Rect1Border
		};

		SetDrawColour(81, 81, 81);
		FillRect(GetFrameRect(Rect1Fill));
	}
		 break;

	case ScreenState::DrawImage: {
		SetDrawColour(12, 23, 39);
		Clear();
		SetDrawColour(6, 11, 19);
		SDL_Rect dst = canvas->GetBounds();

		int zoom = canvas->GetZoom();

		dst.x += 8 * zoom - 2;
		dst.y += 8 * zoom - 2;
		dst.w += 2;
		dst.h += 2;
		FillRect(dst);
	}
		break;
	}

	RenderableElement::RenderAllElements(gameRenderer);
}

void SDLG::OnStart() {
	minFrameDelta = 1000 / 60;
	gameState = ScreenState::DrawImage;
	canvas = new DrawCanvas(100, 100);

	palette = new PaletteRenderer(*canvas);
}

void SDLG::OnFrame() {
	DoLogic();

	DoDraw();

	SDL_RenderPresent(gameRenderer);
}

void SDLG::OnQuit() {
	delete canvas;
}