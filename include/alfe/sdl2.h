#include "alfe/main.h"

#ifndef INCLUDED_SDL_H
#define INCLUDED_SDL_H

#ifdef _WIN32
#include "SDL.h"
#else
#include "SDL2/SDL.h"
#endif

// This is the exception that we throw if any SDL methods fail.
class SDLException : public Exception
{
public:
    SDLException(String message)
      : Exception(message + ": " + SDL_GetError()) { }
};

#define IF_ZERO_THROW_SDL(expr, msg) \
    IF_TRUE_THROW((expr) == 0, SDLException(msg))
#define IF_NONZERO_THROW_SDL(expr, msg) \
    IF_TRUE_THROW((expr) != 0, SDLException(msg))

class SDL
{
public:
    SDL(UInt32 flags = 0)
    {
        IF_NONZERO_THROW_SDL(SDL_Init(flags), "Initializing SDL");
    }
    ~SDL() { SDL_Quit(); }
};

class SDLWindow
{
public:
    SDLWindow()
    {
        _window = SDL_CreateWindow("CGA monitor",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 912, 525, 0);
        IF_ZERO_THROW_SDL(_window, "Creating SDL window");
    }
    ~SDLWindow() { SDL_DestroyWindow(_window); }
    SDL_Window* _window;
};

template<class T> class SDLTextureT;
typedef SDLTextureT<void> SDLTexture;

template<class T> class SDLRendererT
{
public:
    SDLRendererT(SDLWindow* window)
    {
        _renderer = SDL_CreateRenderer(window->_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        IF_ZERO_THROW_SDL(_renderer, "Creating SDL renderer");
    }
    ~SDLRendererT() { SDL_DestroyRenderer(_renderer); }
    void renderTexture(SDLTextureT<T>* texture)
    {
        // IF_NONZERO_THROW_SDL(SDL_RenderClear(_renderer), "Clearing target");
        IF_NONZERO_THROW_SDL(
            SDL_RenderCopy(_renderer, texture->_texture, NULL, NULL),
            "Rendering texture");
        SDL_RenderPresent(_renderer);
    }
    SDL_Renderer* _renderer;
};

typedef SDLRendererT<void> SDLRenderer;

template<class T> class SDLTextureT
{
public:
    SDLTextureT(SDLRenderer* renderer)
    {
        _texture = SDL_CreateTexture(renderer->_renderer,
            SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 912, 525);
        IF_ZERO_THROW_SDL(_texture, "Creating SDL texture");
    }
    ~SDLTextureT() { SDL_DestroyTexture(_texture); }
    void unlock() { SDL_UnlockTexture(_texture); }
    SDL_Texture* _texture;
};

class SDLTextureLock
{
public:
    SDLTextureLock(SDLTexture* texture) : _texture(texture)
    {
        IF_NONZERO_THROW_SDL(
            SDL_LockTexture(_texture->_texture, NULL, &_pixels, &_pitch),
            "Locking SDL texture");
    }
    ~SDLTextureLock() { _texture->unlock(); }
    SDLTexture* _texture;
    void* _pixels;
    int _pitch;
};

#endif // INCLUDED_SDL_H
