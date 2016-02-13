#include "alfe/sdl2.h"
#include "alfe/bitmap.h"
#include "alfe/bitmap_png.h"

#define TILESX 100
#define TILESY 100
#define TILEX 64
#define TILEY 64
#define TILETYPES 256
#define OUTERTILE
#define PLAYERTILE 7
#define ZOMBIETILE 9
#define KEYTILE 0xf1
#define SKYTILE 0xf0

class Program : public ProgramBase
{
public:
    void run()
    {
        Bitmap<DWORD> textures = PNGFileFormat<DWORD>().load(File("textures1.png", false));

        File level("level.dat");

        _tileGrid.allocate(TILESX*TILESY);
        for (int y = 0; y < TILESY; ++y) {
            for (int x = 0; x < TILESX; ++x) {
                if (x < 15 || x >= TILESX-15 || y < 15 || y >= TILESY-15)
                    _tileGrid[y*TILESX + x] = (y == TILESY-15 ? 2 : 3);
                else
                    _tileGrid[y*TILESX + x] = SKYTILE; //rand() % 253 + 2;
            }
        }
        try {
            String l = level.contents();
            for (int i = 0; i < TILESX*TILESY; ++i)
                _tileGrid[i] = l[i] & 0xff;
        } catch (...) { }

        SDLWindow window;
        SDLRenderer renderer(&window);
        SDLTexture texture(&renderer);
        SDL_Event e;
        windowWidth = 912;
        windowHeight = 525;
        _tileData.allocate(TILEX*TILEY*TILETYPES);
        bool editing = false;
        int lTile = 192;
        int rTile = 191;
        bool lButtonDown = false;
        bool rButtonDown = false;
        int mouseX;
        int mouseY;
        for (int i = 0; i < TILETYPES; ++i) {
            for (int y = 0; y < TILEY; ++y) {
                for (int x = 0; x < TILEX; ++x) {
                    UInt32 a = textures[Vector((i&15)*65 + x, (i/16)*65 + y)];
                    _tileData[(i*TILEY + y)*TILEX + x] = a;
                }
            }
        }

        player.x = 50*TILEX;
        player.y = 50*TILEY; //5120; //50*TILEY;
        playerFrac.x = 0;
        playerFrac.y = 0;
        vPlayer.x = 0;
        vPlayer.y = 0;
        gotKey = false;
        int t = 0;

        zombie.x = 15*TILEX;
        zombie.y = 5126;
        while (zombieColliding())
            ++zombie.x;

        int vDelta = 20000;
        int vMax = 1000000;
        int vMaxZombie = 500000;
        int zombieJumpV = 1000000;
        zombieGrounded = false;
        playerTile = PLAYERTILE;

        bool up = false, down = false, left = false, right = false;

        while (colliding())
            --player.x;

        _timestamps.allocate(60);
        do {
            UInt32 startTime = SDL_GetTicks();
            SDLTextureLock lock(&texture);
            UInt8* row = reinterpret_cast<UInt8*>(lock._pixels);
            int pitch = lock._pitch;

            renderTiles(row, pitch);

            row = reinterpret_cast<UInt8*>(lock._pixels);
            row += pitch*(windowHeight/2);
            for (int y = 0; y < TILEY; ++y) {
                UInt32* output = reinterpret_cast<UInt32*>(row);
                output += (windowWidth/2);
                for (int x = 0; x < TILEX; ++x) {
                    *output = compose(*output, _tileData[(playerTile*TILEY + y)*TILEX + x]);
                    ++output;
                }
                row += pitch;
            }

            Vector zScreen = zombie - player;
            Vector zOffset(0, 0);
            Vector zSize(TILEX, TILEY);
            if (zScreen.x < 0) {
                zOffset.x = -zScreen.x;
                zSize.x += zScreen.x;
                zScreen.x = 0;
            }
            if (zScreen.y < 0) {
                zOffset.y = -zScreen.y;
                zSize.y += zScreen.y;
                zScreen.y = 0;
            }
            if (zScreen.x + zSize.x > windowWidth)
                zSize.x = windowWidth - zScreen.x;
            if (zScreen.y + zSize.y > windowHeight)
                zSize.y = windowHeight - zScreen.y;
            if (zSize.x > 0 && zSize.y > 0) {
                //printf("zScreen = %i, %i, zOffset = %i, %i, zSize = %i, %i\n",zScreen.x,zScreen.y,zOffset.x,zOffset.y,zSize.x,zSize.y);
                // Draw zombie
                row = reinterpret_cast<UInt8*>(lock._pixels);
                row += pitch*zScreen.y;
                for (int y = 0; y < zSize.y; ++y) {
                    UInt32* output = reinterpret_cast<UInt32*>(row);
                    output += zScreen.x;
                    for (int x = 0; x < zSize.x; ++x) {
                        *output = compose(*output, _tileData[(ZOMBIETILE*TILEY + y + zOffset.y)*TILEX + x + zOffset.x]);
                        ++output;
                    }
                    row += pitch;
                }
            }

            if (gotKey) {
                // Draw key overlay
                row = reinterpret_cast<UInt8*>(lock._pixels);
                row += pitch*(windowHeight - TILEY);
                for (int y = 0; y < TILEY; ++y) {
                    UInt32* output = reinterpret_cast<UInt32*>(row);
                    for (int x = 0; x < TILEX; ++x) {
                        *output = compose(*output, _tileData[(KEYTILE*TILEY + y)*TILEX + x]);
                        ++output;
                    }
                    row += pitch;
                }
            }

            if (editing) {
                int paletteX = windowWidth - (4*TILEX + 16);
                int paletteY = 16;
                row = reinterpret_cast<UInt8*>(lock._pixels);
                row += pitch*paletteY;
                for (int y = 0; y < TILEY*4; ++y) {
                    UInt32* output = reinterpret_cast<UInt32*>(row) + paletteX;
                    for (int x = 0; x < TILEX*4; ++x) {
                        int xg = x/16;
                        int yg = y/16;
                        int xp = x&15;
                        int yp = y&15;
                        *output = _tileData[((yg*16 + xg)*TILEY + yp*4)*TILEX + xp*4];
                        ++output;
                    }
                    row += pitch;
                }
                int xl = (lTile & 15) * (TILEX/4) + paletteX;
                int yl = (lTile / 16) * (TILEY/4) + paletteY;
                int xr = (rTile & 15) * (TILEX/4) + paletteX;
                int yr = (rTile / 16) * (TILEY/4) + paletteY;
                for (int x = -1; x < TILEX/4 + 1; ++x) {
                    plot(&lock, xl + x, yl - 1, 0xff000000);
                    plot(&lock, xl + x, yl + TILEY/4, 0xff000000);
                    plot(&lock, xr + x, yr - 1, 0xffffffff);
                    plot(&lock, xr + x, yr + TILEY/4, 0xffffffff);
                }
                for (int y = -1; y < TILEX/4 + 1; ++y) {
                    plot(&lock, xl - 1, yl + y, 0xff000000);
                    plot(&lock, xl + TILEX/4, yl + y, 0xff000000);
                    plot(&lock, xr - 1, yr + y, 0xffffffff);
                    plot(&lock, xr + TILEX/4, yr + y, 0xffffffff);
                }

            }

            renderer.renderTexture(&texture);

            while (SDL_PollEvent(&e) != 0) {
                if (e.type == SDL_QUIT)
                    return;
                if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
                    switch (e.key.keysym.sym) {
                        case SDLK_UP:
                            up = (e.type == SDL_KEYDOWN);
                            break;
                        case SDLK_DOWN:
                            down = (e.type == SDL_KEYDOWN);
                            break;
                        case SDLK_LEFT:
                            left = (e.type == SDL_KEYDOWN);
                            break;
                        case SDLK_RIGHT:
                            right = (e.type == SDL_KEYDOWN);
                            break;
                        case SDLK_e:
                            editing = true;
                            break;
                        case SDLK_p:
                            if (e.type == SDL_KEYDOWN) {
                                playerTile = PLAYERTILE + PLAYERTILE + 1 - playerTile;
                                if (colliding())
                                    playerTile = PLAYERTILE + PLAYERTILE + 1 - playerTile;
                            }
                            break;
                        case SDLK_ESCAPE:
                            if (e.type == SDL_KEYUP)
                                break;
                            if (editing) {
                                editing = false;
                                break;
                            }
                            return;
                    }
                }
                if (editing) {
                    if (e.type == SDL_MOUSEBUTTONDOWN) {
                        if (e.button.button == SDL_BUTTON_LEFT)
                            lButtonDown = true;
                        if (e.button.button == SDL_BUTTON_RIGHT)
                            rButtonDown = true;
                        mouseX = e.button.x;
                        mouseY = e.button.y;
                    }
                    if (e.type == SDL_MOUSEBUTTONUP) {
                        if (e.button.button == SDL_BUTTON_LEFT)
                            lButtonDown = false;
                        if (e.button.button == SDL_BUTTON_RIGHT)
                            rButtonDown = false;
                        mouseX = e.button.x;
                        mouseY = e.button.y;
                    }
                    if (e.type == SDL_MOUSEMOTION) {
                        mouseX = e.motion.x;
                        mouseY = e.motion.y;
                    }
                    if (mouseX >= windowWidth - (4*TILEX + 16) && mouseX < windowWidth - 16 && mouseY >= 16 && mouseY < 4*TILEY + 16) {
                        mouseX -= windowWidth - (4*TILEX + 16);
                        mouseY -= 16;
                        mouseX /= 16;
                        mouseY /= 16;
                        if (lButtonDown)
                            lTile = mouseY*16 + mouseX;
                        if (rButtonDown)
                            rTile = mouseY*16 + mouseX;
                    }
                    else {
                        int mTileX = (mouseX + player.x)/TILEX;
                        int mTileY = (mouseY + player.y)/TILEY;
                        if (mTileX >= 15 && mTileX < TILESX-15 && mTileY >= 15 && mTileY < TILESY-15) {
                            int oldTile = _tileGrid[mTileY*TILESX + mTileX];
                            if (lButtonDown)
                                _tileGrid[mTileY*TILESX + mTileX] = lTile;
                            if (rButtonDown)
                                _tileGrid[mTileY*TILESX + mTileX] = rTile;
                            if (colliding())
                                _tileGrid[mTileY*TILESX + mTileX] = oldTile;
                        }
                    }
                }
            }
            if (up)
                vPlayer.y = max(-vMax, vPlayer.y - vDelta);
            else
                vPlayer.y = min(vMax, vPlayer.y + vDelta);
            if (left)
                vPlayer.x = max(-vMax, vPlayer.x - vDelta);
            else
                if (right)
                    vPlayer.x = min(vMax, vPlayer.x + vDelta);
                else {
                    if (vPlayer.x > 0)
                        vPlayer.x = max(0, vPlayer.x - vDelta);
                    else
                        vPlayer.x = min(0, vPlayer.x + vDelta);
                }

            playerFrac.x += vPlayer.x;
            player.x += playerFrac.x >> 16;
            playerFrac.x &= 0xffff;
            bool collided = false;
            while (colliding()) {
                if (vPlayer.x < 0)
                    ++player.x;
                else
                    --player.x;
                collided = true;
            }
            if (collided)
                vPlayer.x = 0;
            playerFrac.y += vPlayer.y;
            player.y += playerFrac.y >> 16;
            playerFrac.y &= 0xffff;
            collided = false;
            while (colliding()) {
                if (vPlayer.y < 0)
                    ++player.y;
                else
                    --player.y;
                collided = true;
            }
            if (collided)
                vPlayer.y = 0;
            doKeys();

            if (zombie.y > player.y + windowHeight/2 && zombieGrounded) {
                vZombie.y = -zombieJumpV; //max(-vMaxZombie, vZombie.y - vDelta);
                zombieGrounded = false;
            }
            else
                vZombie.y = min(vMaxZombie, vZombie.y + vDelta);
            if (zombie.x > player.x + windowWidth/2)
                vZombie.x = max(-vMaxZombie, vZombie.x - vDelta);
            else
                if (zombie.x < player.x + windowWidth/2)
                    vZombie.x = min(vMaxZombie, vZombie.x + vDelta);
                else {
                    if (vZombie.x > 0)
                        vZombie.x = max(0, vZombie.x - vDelta);
                    else
                        vZombie.x = min(0, vZombie.x + vDelta);
                }

            zombieFrac.x += vZombie.x;
            zombie.x += zombieFrac.x >> 16;
            zombieFrac.x &= 0xffff;
            collided = false;
            while (zombieColliding()) {
                if (vZombie.x < 0)
                    ++zombie.x;
                else
                    --zombie.x;
                collided = true;
            }
            if (collided)
                vZombie.x = 0;
            zombieFrac.y += vZombie.y;
            zombie.y += zombieFrac.y >> 16;
            zombieFrac.y &= 0xffff;
            collided = false;
            while (zombieColliding()) {
                if (vZombie.y < 0)
                    ++zombie.y;
                else {
                    --zombie.y;
                    zombieGrounded = true;
                }
                collided = true;
            }
            if (collided)
                vZombie.y = 0;



            ++t;
            if (t == 60)
                t = 0;
            UInt32 endTime = SDL_GetTicks();
            if (endTime - startTime < 16)
                SDL_Delay(16 - (endTime - startTime));

            //int64_t delta = _timestamps[t];
            //LARGE_INTEGER pc;
            //QueryPerformanceCounter(&pc);
            //_timestamps[t] = pc.QuadPart;
            //delta = _timestamps[t] - delta;
            //if (t == 0) {
            //    QueryPerformanceFrequency(&pc);
            //    double fps = pc.QuadPart*60.0/delta;
            //    printf("%f\n",fps);
            //}
        } while (true);
    }
    ~Program()
    {
        try {
            Array<UInt8> tg(TILESX*TILESY);
            for (int i = 0; i < TILESX*TILESY; ++i)
                tg[i] = _tileGrid[i];
            File("level.dat").save(tg);
        }
        catch (...) { }
    }
private:
    UInt32 compose(UInt32 background, UInt32 sprite)
    {
        int alpha = (sprite >> 24) & 0xff;
        int b = (background & 0xff) * (255 - alpha) + (sprite & 0xff) * alpha;
        int g = ((background >> 8) & 0xff) * (255 - alpha) + ((sprite >> 8) & 0xff) * alpha;
        int r = ((background >> 16) & 0xff) * (255 - alpha) + ((sprite >> 16) & 0xff) * alpha;
        return clamp(0, b/255, 255) | (clamp(0, g/255, 255) << 8) | (clamp(0, r/255, 255) << 16);
    }
    void plot(SDLTextureLock* lock, int x, int y, UInt32 colour)
    {
        *(reinterpret_cast<UInt32*>(reinterpret_cast<UInt8*>(lock->_pixels) + y*lock->_pitch) + x) = colour;
    }
    void renderTiles(UInt8* row, int pitch)
    {
        for (int y = 0; y < windowHeight; ++y) {
            UInt32* output = reinterpret_cast<UInt32*>(row);
            for (int x = 0; x < windowWidth; ++x) {
                Vector v = Vector(x, y) + player;
                Vector tile = v/Vector(TILEX, TILEY);
                int tn = _tileGrid[tile.y*TILESX + tile.x];
                v -= tile*Vector(TILEX, TILEY);
                *output = _tileData[(tn*TILEY + v.y)*TILEX + v.x];
                ++output;
            }
            row += pitch;
        }
    }
    bool opaque(int tile) { return tile < 128; }
    bool colliding()
    {
        for (int y = 0; y < TILEY; ++y)
            for (int x = 0; x < TILEX; ++x) {
                UInt32 a = _tileData[(playerTile*TILEY + y)*TILEX + x];
                if (static_cast<int>(a >> 24) > 0x80) {
                    Vector v = Vector(x, y) + player + Vector(windowWidth/2, windowHeight/2);
                    v /= Vector(TILEX, TILEY);
                    if (opaque(_tileGrid[v.y*TILESX + v.x]))
                        return true;
                }
            }
        return false;
    }
    bool zombieColliding()
    {
        for (int y = 0; y < TILEY; ++y)
            for (int x = 0; x < TILEX; ++x) {
                UInt32 a = _tileData[(ZOMBIETILE*TILEY + y)*TILEX + x];
                if (static_cast<int>(a >> 24) > 0x80) {
                    Vector v = Vector(x, y) + zombie;
                    v /= Vector(TILEX, TILEY);
                    if (opaque(_tileGrid[v.y*TILESX + v.x]))
                        return true;
                }
            }
        return false;
    }
    void doKeys()
    {
        for (int y = 0; y < TILEY; ++y)
            for (int x = 0; x < TILEX; ++x) {
                UInt32 a = _tileData[(playerTile*TILEY + y)*TILEX + x];
                if (a != 0xffffffff) {
                    Vector v = Vector(x, y) + player + Vector(windowWidth/2, windowHeight/2);
                    v /= Vector(TILEX, TILEY);
                    if (_tileGrid[v.y*TILESX + v.x] == KEYTILE) {
                        _tileGrid[v.y*TILESX + v.x] = SKYTILE;
                        gotKey = true;
                    }
                }
            }
    }

    int playerTile;
    bool zombieGrounded;
    bool gotKey;
    int windowHeight;
    int windowWidth;
    Vector player;  // Actually the position of the top-left of the window in the arena
    Vector playerFrac;
    Vector vPlayer;
    Vector zombie;
    Vector zombieFrac;
    Vector vZombie;
    Array<int> _tileGrid;
    Array<UInt32> _tileData;
    Array<int64_t> _timestamps;
};