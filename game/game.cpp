#include "alfe/sdl2.h"
#include "alfe/bitmap.h"
#include "alfe/bitmap_png.h"

#define TILESX 100
#define TILESY 100
#define TILEX 64
#define TILEY 64
#define TILETYPES 256
#define PLAYERTILE 0
#define ZOMBIETILE 1
#define KEYTILE 2

class Program : public ProgramBase
{
public:
    void run()
    {
        Bitmap<SRGB> bricks =
            PNGFileFormat<SRGB>().load(File("brick_texture64x64.png", true));
        Bitmap<SRGB> shkg =
            PNGFileFormat<SRGB>().load(File("Image1shkg_64.png", true));
        Bitmap<SRGB> log =
            PNGFileFormat<SRGB>().load(File("64x64logtexture.png", true));
        Bitmap<SRGB> grass =
            PNGFileFormat<SRGB>().load(File("grass_texture64x64.png", true));
        Bitmap<SRGB> mud =
            PNGFileFormat<SRGB>().load(File("mud_texture64x64.png", true));
        Bitmap<SRGB> key =
            PNGFileFormat<SRGB>().load(File("Image7key_64.png", true));
        Bitmap<SRGB> zomb =
            PNGFileFormat<SRGB>().load(File("Image11zomie_64.png", true));

        SDLWindow window;
        SDLRenderer renderer(&window);
        SDLTexture texture(&renderer);
        SDL_Event e;
        windowWidth = 912;
        windowHeight = 525;
        _tileGrid.allocate(TILESX*TILESY);
        _tileData.allocate(TILEX*TILEY*TILETYPES);
        for (int i = 0; i < TILETYPES; ++i) {
            for (int y = 0; y < TILEY; ++y) {
                for (int x = 0; x < TILEX; ++x) {
                    Vector v(x, y);
                    SRGB a;
                    a.x = x*4;
                    a.y = y*4;
                    a.z = i;
                    if (i == PLAYERTILE)
                        a = shkg[v];
                    if (i == KEYTILE) {
                        SRGB k = key[v];
                        if (k.x != 255 || k.y != 255 || k.z != 255)
                            a = k;
                    }
                    if (i == ZOMBIETILE)
                        a = zomb[v];
                    if (i >= 192)
                        a = log[v];
                    if (i >= 224)
                        a = bricks[v];
                    if (i == 254)
                        a = mud[v];
                    if (i == 255)
                        a = grass[v];
                    _tileData[(i*TILEY + y)*TILEX + x] = 0xff000000 | (a.x << 16) | (a.y << 8) | a.z;
                }
            }
        }
        for (int y = 0; y < TILESY; ++y) {
            for (int x = 0; x < TILESX; ++x) {
                if (x < 15 || x >= TILESX-15 || y < 15 || y >= TILESY-15)
                    _tileGrid[y*TILESX + x] = (y == TILESY-15 ? 255 : 254);
                else
                    _tileGrid[y*TILESX + x] = rand() % 253 + 2;
            }
        }

        player.x = 50*TILEX;
        player.y = 5126; //50*TILEY;
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
                    UInt32 a = _tileData[(PLAYERTILE*TILEY + y)*TILEX + x];
                    if (a != 0xffffffff)
                        *output = a;
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
                        UInt32 a = _tileData[(ZOMBIETILE*TILEY + y + zOffset.y)*TILEX + x + zOffset.x];
                        if (a != 0xffffffff)
                            *output = a;
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
                        UInt32 a = _tileData[(KEYTILE*TILEY + y)*TILEX + x];
                        if (a != 0xffffffff)
                            *output = a;
                        ++output;
                    }
                    row += pitch;
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
                        case SDLK_ESCAPE:
                            return;
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
private:
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
    bool colliding()
    {
        for (int y = 0; y < TILEY; ++y)
            for (int x = 0; x < TILEX; ++x) {
                UInt32 a = _tileData[(PLAYERTILE*TILEY + y)*TILEX + x];
                if (a != 0xffffffff) {
                    Vector v = Vector(x, y) + player + Vector(windowWidth/2, windowHeight/2);
                    v /= Vector(TILEX, TILEY);
                    if (_tileGrid[v.y*TILESX + v.x] >= 192)
                        return true;
                }
            }
        return false;
    }
    bool zombieColliding()
    {
        for (int y = 0; y < TILEY; ++y)
            for (int x = 0; x < TILEX; ++x) {
                UInt32 a = _tileData[(PLAYERTILE*TILEY + y)*TILEX + x];
                if (a != 0xffffffff) {
                    Vector v = Vector(x, y) + zombie;
                    v /= Vector(TILEX, TILEY);
                    if (_tileGrid[v.y*TILESX + v.x] >= 192)
                        return true;
                }
            }
        return false;
    }
    void doKeys()
    {
        for (int y = 0; y < TILEY; ++y)
            for (int x = 0; x < TILEX; ++x) {
                UInt32 a = _tileData[(PLAYERTILE*TILEY + y)*TILEX + x];
                if (a != 0xffffffff) {
                    Vector v = Vector(x, y) + player + Vector(windowWidth/2, windowHeight/2);
                    v /= Vector(TILEX, TILEY);
                    if (_tileGrid[v.y*TILESX + v.x] == KEYTILE) {
                        _tileGrid[v.y*TILESX + v.x] = 3;
                        gotKey = true;
                    }
                }
            }
    }

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