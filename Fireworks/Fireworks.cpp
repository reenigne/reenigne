#include "alfe/main.h"
#include "alfe/sdl2.h"
#include "alfe/bitmap.h"
#include "alfe/bitmap_png.h"
#include "SDL_syswm.h"

int windowWidth;
int windowHeight;

class SDLFullScreenWindow
{
public:
	SDLFullScreenWindow()
	{
		//_window = SDL_CreateWindow("",
		//	SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, /*SDL_WINDOW_FULLSCREEN*/ 0
		//);

		SDL_VideoInit(NULL);
		SDL_DisplayMode dm;
		if (SDL_GetDesktopDisplayMode(0, &dm) != 0) {
			printf("%s\n", SDL_GetError());
			exit(1);
		}
		dm.w = 1920;
		dm.h = 1080;
		_window = SDL_CreateWindow("",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, dm.w, dm.h, SDL_WINDOW_FULLSCREEN
		);
		IF_ZERO_THROW_SDL(_window, "Creating SDL window");
		SDL_GetWindowSize(_window, &windowWidth, &windowHeight);
	}
	~SDLFullScreenWindow() { SDL_DestroyWindow(_window); }
	SDL_Window* _window;
};

template<class T> class SDLFullScreenTextureT;

template<class T> class SDLFullScreenRendererT
{
public:
	SDLFullScreenRendererT(SDLFullScreenWindow* window)
	{
		_renderer = SDL_CreateRenderer(window->_window, -1, 0);
		IF_ZERO_THROW_SDL(_renderer, "Creating SDL renderer");
	}
	~SDLFullScreenRendererT() { SDL_DestroyRenderer(_renderer); }
	void renderTexture(SDLFullScreenTextureT<T>* texture)
	{
		// IF_NONZERO_THROW_SDL(SDL_RenderClear(_renderer), "Clearing target");
		IF_NONZERO_THROW_SDL(
			SDL_RenderCopy(_renderer, texture->_texture, NULL, NULL),
			"Rendering texture");
		SDL_RenderPresent(_renderer);
	}
	SDL_Renderer* _renderer;
};

typedef SDLFullScreenRendererT<void> SDLFullScreenRenderer;

template<class T> class SDLFullScreenTextureT
{
public:
	SDLFullScreenTextureT(SDLFullScreenRenderer* renderer)
	{
		_texture = SDL_CreateTexture(renderer->_renderer,
			SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, windowWidth, windowHeight);
		IF_ZERO_THROW_SDL(_texture, "Creating SDL texture");
	}
	~SDLFullScreenTextureT() { SDL_DestroyTexture(_texture); }
	void unlock() { SDL_UnlockTexture(_texture); }
	SDL_Texture* _texture;
};

typedef SDLFullScreenTextureT<void> SDLFullScreenTexture;


class SDLFullScreenTextureLock
{
public:
	SDLFullScreenTextureLock(SDLFullScreenTexture* texture) : _texture(texture)
	{
		IF_NONZERO_THROW_SDL(
			SDL_LockTexture(_texture->_texture, NULL, &_pixels, &_pitch),
			"Locking SDL texture");
	}
	~SDLFullScreenTextureLock() { _texture->unlock(); }
	SDLFullScreenTexture* _texture;
	void* _pixels;
	int _pitch;
};


static const int nParticles = 1000;

struct Particle {
	float x; 
	float y;
	float vx;
	float vy;
	float a;
	float av;
	int graphic;
	int state;
};

Particle particles[nParticles];

UInt8* row;
int pitch;

UInt32* tiles[2];

UInt32 compose(UInt32 background, UInt32 sprite)
{
	int alpha = (sprite >> 24) & 0xff;
	int b = (background & 0xff) * (255 - alpha) + (sprite & 0xff) * alpha;
	int g = ((background >> 8) & 0xff) * (255 - alpha) + ((sprite >> 8) & 0xff) * alpha;
	int r = ((background >> 16) & 0xff) * (255 - alpha) + ((sprite >> 16) & 0xff) * alpha;
	return clamp(0, b/255, 255) | (clamp(0, g/255, 255) << 8) | (clamp(0, r/255, 255) << 16);
}



void drawSprite(float x, float y, int graphic, float a)
{
	int xp = static_cast<int>(x);
	int yp = static_cast<int>(y);
	if (xp < 0 || xp >= windowWidth - 64 || yp < 0 || yp >= windowHeight - 64)
		return;
	UInt8* p = row + yp*pitch;
	UInt32* tile = tiles[graphic];
	for (int yy = 0; yy < 64; ++yy) {
		UInt32* output = reinterpret_cast<UInt32*>(p);
		output += xp;
		for (int xx = 0; xx < 64; ++xx) {
			int c = tile[yy*64 + xx];
			//if ((c & 0xff000000) != 0)
			//	*output = c & 0x00ffffff;
			*output = compose(*output, c);
			++output;
		}
		p += pitch;
	}
}

void drawSprite2(float x, float y, int graphic, float a)
{
	int xp = static_cast<int>(x);
	int yp = static_cast<int>(y);
	if (xp < 0 || xp >= windowWidth - 128 || yp < 0 || yp >= windowHeight - 128)
		return;
	UInt8* p = row + yp*pitch;
	UInt32* tile = tiles[graphic];
	for (int yy = 0; yy < 128; ++yy) {
		UInt32* output = reinterpret_cast<UInt32*>(p);
		output += xp;
		for (int xx = 0; xx < 128; ++xx) {
			int c = tile[(yy/2)*64 + (xx/2)];
			//if ((c & 0xff000000) != 0)
			//	*output = c & 0x00ffffff;
			*output = compose(*output, c);
			++output;
		}
		p += pitch;
	}
}

const char *scrollText = 
"CRTC presents - a partyhack for Nova 2019. Code and graphics by alex2006 "
"(his first prod and his first demoparty). Code by reenigne. "
"Rendered entirely in software! Ran out of time for music, sorry...";

class Program : public ProgramBase
{
public:
	void run()
	{
		int stl = strlen(scrollText);

		auto star =
			PNGFileFormat<DWORD>().load(File("STAR 3.png", false));
		tiles[0] = reinterpret_cast<UInt32*>(star.data());

		auto rocket =
			PNGFileFormat<DWORD>().load(File("firework rocket.png", false));
		tiles[1] = reinterpret_cast<UInt32*>(rocket.data());

		auto background =
			PNGFileFormat<DWORD>().load(File("background.png", false));

		Array<Byte> font;
		File("5788005.u33", true).readIntoArray(&font);

		SDLFullScreenWindow window;
		SDLFullScreenRenderer renderer(&window);
		SDLFullScreenTexture texture(&renderer);
		SDL_Event e;

		for (int p = 0; p < nParticles; ++p) {
			float v = 10*static_cast<float>(rand())/RAND_MAX;
			float a = static_cast<float>(rand())/RAND_MAX;

			Particle* pp = &particles[p];
			pp->x = static_cast<float>(windowWidth/2);
			pp->y = static_cast<float>(windowHeight/2);
			pp->vx = static_cast<float>(v*sin(a*2*M_PI));
			pp->vy = static_cast<float>(v*cos(a*2*M_PI));
			pp->a = 0;
			pp->av = 0;
			pp->graphic = 0;
			pp->state = 0;
		}


		for (int t = 0; t<30*60; ++t) {
			UInt32 startTime = SDL_GetTicks();

			// Every second, launch a new projectile if there are enough particles
			if ((t % (1*60)) == 0) {
				for (int p = 0; p<nParticles; ++p) {
					Particle* pp = &particles[p];
					if (pp->state == 0) {
						pp->state = 1;
						pp->graphic = 1;
						pp->y = static_cast<float>(windowHeight);
						pp->x = static_cast<float>(windowWidth*rand())/RAND_MAX;
						pp->vx = static_cast<float>(rand())/RAND_MAX;
						pp->vy = -10 + static_cast<float>(rand())/RAND_MAX;
						break;
					}
				}
			}

			SDLFullScreenTextureLock lock(&texture);
			row = reinterpret_cast<UInt8*>(lock._pixels);
			pitch = lock._pitch;

			UInt8* p = row;
			UInt8* pb = background.data();
			for (int y = 0; y < windowHeight; ++y) {
				UInt32* output = reinterpret_cast<UInt32*>(p);
				UInt32* ob = reinterpret_cast<UInt32*>(pb);
				for (int x = 0; x < windowWidth; ++x) {
					*output = *ob; // xffffff;
					++output;
					++ob;
				}
				p += pitch;
				pb += background.stride();
			}

			for (int p = 0; p<nParticles; ++p) {
				Particle* pp = &particles[p];
				pp->x = pp->x+pp->vx;
				pp->y = pp->y+pp->vy;
				pp->a = pp->a+pp->av;
				pp->vy = pp->vy + 0.1f;
				if (pp->state == 1)
					drawSprite2(pp->x, pp->y, pp->graphic, pp->a);
				if (pp->state == 2)
					drawSprite(pp->x, pp->y, pp->graphic, pp->a);
				if (pp->state == 1 && pp->vy >= 0) {
					// Explode
					int n = 300;
					for (int p1 = 0; p1<nParticles; ++p1) {
						Particle* pp1 = &particles[p1];
						if (pp1->state == 0) {
							float v = 10*cos(static_cast<float>(rand())*M_PI/RAND_MAX);
							float a = static_cast<float>(rand())/RAND_MAX;

							Particle* pp = &particles[p];
							pp1->x = pp->x;
							pp1->y = pp->y;
							pp1->vx = pp->vx + static_cast<float>(v*sin(a*2*M_PI));
							pp1->vy = pp->vy + static_cast<float>(v*cos(a*2*M_PI));

							--n;
							if (n == 0)
								break;
							pp1->state = 2;
							pp1->graphic = 0;
						}
					}
					pp->state = 2;
				}
				if (pp->state == 2 && pp->y >= windowHeight)
					pp->state = 0;
			}
			for (int c = 0; c < stl; ++c) {
				int ch = scrollText[c];
				int x = c*64 - t*8;
				if (x >= windowWidth || x < -64)
					continue;
				UInt8* p = row + (windowHeight*3/4 + static_cast<int>(sin((x + t)*0.01)*64))*pitch;
				for (int y = 0; y < 64; ++y) {
					int hue = c + y + t;
					UInt32* output = reinterpret_cast<UInt32*>(p) + x;
					int rr = 128 + 127*sin(hue*0.1);
					int gg = 128 + 127*sin(hue*0.1 + M_PI*2/3);
					int bb = 128 + 127*sin(hue*0.1 + M_PI*4/3);
					int col = (rr << 16) + (gg << 8) + bb;
					for (int xx = 0; xx < 64; ++xx) {
						int xs = x + xx;
						if (xs >= 0 && xs < windowWidth)
							if ((font[ch*8 + (y/8) + 0x1800] & (128 >> (xx / 8))) != 0)
								*output = col;
						++output;
					}
					p += pitch;
				}
			}

			renderer.renderTexture(&texture);

			while (SDL_PollEvent(&e) != 0) {
				if (e.type == SDL_QUIT)
					return;
				if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
					switch (e.key.keysym.sym) {
						case SDLK_ESCAPE:
							if (e.type == SDL_KEYUP)
								return;
					}
				}
			}


			UInt32 endTime = SDL_GetTicks();
			if (endTime - startTime < 16)
				SDL_Delay(16 - (endTime - startTime));

		}
	}
};