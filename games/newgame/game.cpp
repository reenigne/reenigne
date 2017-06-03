#include "alfe/sdl2.h"
#include "alfe/bitmap.h"
#include "alfe/bitmap_png.h"
#include "resource.h"
#include "SDL_syswm.h"

#define TILESX 100
#define TILESY 100
#define TILEX 64
#define TILEY 64
#define TILETYPES 256
#define OUTERTILE
#define SHKGTILE 7
#define LIANTILE 15
#define ZOMBIERTILE0 0
#define ZOMBIERTILE1 13
#define ZOMBIERTILE2 14
#define ZOMBIELTILE0 5
#define ZOMBIELTILE1 11
#define ZOMBIELTILE2 12
#define KEYTILE 0xf1
#define SKYTILE 0xf0

bool opaque(int tile) { return tile < 128; }

UInt32 compose(UInt32 background, UInt32 sprite)
{
    int alpha = (sprite >> 24) & 0xff;
    int b = (background & 0xff) * (255 - alpha) + (sprite & 0xff) * alpha;
    int g = ((background >> 8) & 0xff) * (255 - alpha) + ((sprite >> 8) & 0xff) * alpha;
    int r = ((background >> 16) & 0xff) * (255 - alpha) + ((sprite >> 16) & 0xff) * alpha;
    return clamp(0, b/255, 255) | (clamp(0, g/255, 255) << 8) | (clamp(0, r/255, 255) << 16);
}

template<class T> class GameT;
typedef GameT<void> Game;

template<class T> class CharacterT;
typedef CharacterT<void> Character;

template<class T> class ZombieT;
typedef ZombieT<void> Zombie;

template<class T> class PlayerT;
typedef PlayerT<void> Player;

template<class T> class CharacterT
{
public:
    CharacterT()
      : _frac(0, 0), _velocity(0, 0), _grounded(false), _acceleration(20000)
    { }
    void init(Game* game, Vector position)
    {
        _position = position;
        _game = game;
        _tileGrid = &(_game->_tileGrid[0]);
        _tileData = &(_game->_tileData[0]);
        _windowSize = _game->_windowSize;
    }
    void move()
    {
        _frac.x += _velocity.x;
        _position.x += _frac.x >> 16;
        _frac.x &= 0xffff;
        bool collided = false;
        while (colliding()) {
            if (_velocity.x < 0)
                ++_position.x;
            else
                --_position.x;
            collided = true;
        }
        if (collided)
            _velocity.x = 0;
        _frac.y += _velocity.y;
        int oldy = _position.y;
        _position.y += _frac.y >> 16;
        if (oldy != _position.y)
            _grounded = false;
        _frac.y &= 0xffff;
        collided = false;
        while (colliding()) {
            if (_velocity.y < 0)
                ++_position.y;
            else {
                --_position.y;
                _grounded = true;
            }
            collided = true;
        }
        if (collided)
            _velocity.y = 0;
    }
    virtual bool colliding()
    {
        Vector pp = _position%Vector(TILEX, TILEY);
        for (int yy = 0; yy < 2; ++yy)
            for (int xx = 0; xx < 2; ++xx) {
                Vector p = _position/Vector(TILEX, TILEY) + Vector(xx, yy);
                if (!opaque(_tileGrid[p.y*TILESX + p.x]))
                    continue;
                int xl = 0;
                int xh = TILEX;
                int yl = 0;
                int yh = TILEY;
                int xo = 0;
                int yo = 0;
                if (xx == 0)
                    xh = TILEX - pp.x;
                else
                    xl = TILEX - pp.x;
                if (yy == 0)
                    yh = TILEY - pp.y;
                else
                    yl = TILEX - pp.y;
                for (int y = yl; y < yh; ++y) {
                    for (int x = xl; x < xh; ++x) {
                        UInt32 a = _tileData[(_tile*TILEY + y)*TILEX + x];
                        if (static_cast<int>(a >> 24) > 0x80)
                            return true;
                    }
                }
            }
        return false;
    }

    void draw(SDLTextureLock* lock)
    {
        UInt32* tile = &_tileData[_tile*TILEY*TILEX];
        Vector screen = _position + _windowSize/2 - _game->_player._position;
        Vector offset(0, 0);
        Vector size(TILEX, TILEY);
        if (screen.x < 0) {
            offset.x = -screen.x;
            size.x += screen.x;
            screen.x = 0;
        }
        if (screen.y < 0) {
            offset.y = -screen.y;
            size.y += screen.y;
            screen.y = 0;
        }
        if (screen.x + size.x > _windowSize.x)
            size.x = _windowSize.x - screen.x;
        if (screen.y + size.y > _windowSize.y)
            size.y = _windowSize.y - screen.y;
        if (size.x > 0 && size.y > 0) {
            auto row = reinterpret_cast<UInt8*>(lock->_pixels);
            int pitch = lock->_pitch;
            row += pitch*screen.y;
            for (int y = offset.y; y < offset.y + size.y; ++y) {
                UInt32* output = reinterpret_cast<UInt32*>(row);
                output += screen.x;
                for (int x = offset.x; x < offset.x + size.x; ++x) {
                    *output = compose(*output, tile[y*TILEX + x]);
                    ++output;
                }
                row += pitch;
            }
        }
    }

    Vector _position;
    Vector _frac;
    Vector _velocity;
    int _tile;
    Game* _game;
    Byte* _tileGrid;
    UInt32* _tileData;
    Vector _windowSize;
    int _acceleration;
    int _maximumVelocity;
    bool _grounded;
};

template<class T> class PlayerT : public Character
{
public:
    PlayerT()
      : _gotKey(false), _animation(0), _jumpVelocity(650000), _isLian(false),
		_lastRight(false)
    {
        _tile = SHKGTILE;
        _maximumVelocity = 1000000;
    }
    void init(Game* game, Vector position)
    {
        Character::init(game, position);
        while (colliding())
            --_position.x;
    }
    void move()
    {
		if (_game->_up && _grounded)
			_velocity.y = -_jumpVelocity;

        //if (_game->_up)
        //    _velocity.y = max(-_maximumVelocity, _velocity.y - _acceleration);
        else
            _velocity.y = min(_maximumVelocity, _velocity.y + _acceleration);
        if (_game->_left)
            _velocity.x = max(-_maximumVelocity, _velocity.x - _acceleration);
        else {
            if (_game->_right)
                _velocity.x = min(_maximumVelocity, _velocity.x + _acceleration);
            else {
                if (_velocity.x > 0)
                    _velocity.x = max(0, _velocity.x - _acceleration);
                else
                    _velocity.x = min(0, _velocity.x + _acceleration);
            }
        }
        if (_velocity.x > 0)
            _lastRight = true;
        if (_velocity.x < 0)
            _lastRight = false;
        _tile = 18;
        Character::move();
        for (int y = 0; y < TILEY; ++y)
            for (int x = 0; x < TILEX; ++x) {
                UInt32 a = _tileData[(_tile*TILEY + y)*TILEX + x];
                if (a != 0xffffffff) {
                    Vector v = Vector(x, y) + _position;
                    v /= Vector(TILEX, TILEY);
                    if (_tileGrid[v.y*TILESX + v.x] == KEYTILE) {
                        _tileGrid[v.y*TILESX + v.x] = SKYTILE;
                        _gotKey = true;
                    }
                }
            }

        setTile();
    }
    void setTile()
    {
        if (_isLian) {
            _tile = LIANTILE;
            return;
        }

        if (!_grounded) {
            if (_velocity.x > 0)
                _tile = 24;
            else
                _tile = 25;
            return;
        }
        if (_velocity.x == 0) {
            if (_lastRight)
                _tile = 19;
            else
                _tile = 18;
            return;
        }
        _animation += _velocity.x;
        int z = (_animation / 1000000)%3;
        if (_velocity.x > 0) {
            switch(z) {
                case 0: _tile = 19; break;
                case 1: _tile = 22; break;
                case 2: _tile = 23; break;
            }
        }
        if (_velocity.x < 0) {
            switch(z) {
                case 0: _tile = 18; break;
                case 1: _tile = 20; break;
                case 2: _tile = 21; break;
            }
        }
    }
    void toggleLian()
    {
        _isLian = !_isLian;
        setTile();
        if (colliding())
            _isLian = !_isLian;
    }
	int _jumpVelocity;
	bool _gotKey;
    bool _lastRight;
    unsigned int _animation;
    bool _isLian;
};

template<class T> class ZombieT : public Character
{
public:
    ZombieT() : _animation(0), _jumpVelocity(500000), _previous(0), _next(0)
    {
        _tile = ZOMBIERTILE0;
        _maximumVelocity = 500000;
    }
    void init(Game* game, Vector position)
    {
        Character::init(game, position);
        while (colliding())
            ++_position.x;
    }
    void move()
    {
        Vector player = _game->_player._position;
        if (_position.y > player.y && _grounded)
            _velocity.y = -_jumpVelocity;
        else
            _velocity.y = min(_maximumVelocity, _velocity.y + _acceleration);
        if (_position.x > player.x)
            _velocity.x = max(-_maximumVelocity, _velocity.x - _acceleration);
        else {
            if (_position.x < player.x)
                _velocity.x = min(_maximumVelocity, _velocity.x + _acceleration);
            else {
                if (_velocity.x > 0)
                    _velocity.x = max(0, _velocity.x - _acceleration);
                else
                    _velocity.x = min(0, _velocity.x + _acceleration);
            }
        }
        // Remove from grid
        if (_previous != 0) {
            *_previous = _next;
            if (_next != 0)
                _next->_previous = _previous;
        }
        Character::move();
        addToGrid();

        _animation += _velocity.x;
        int z = (_animation / 1000000)%3;
        if (_velocity.x > 0) {
            switch(z) {
                case 0: _tile = ZOMBIERTILE0; break;
                case 1: _tile = ZOMBIERTILE1; break;
                case 2: _tile = ZOMBIERTILE2; break;
            }
        }
        else {
            _tile = ZOMBIELTILE0;
            if (_velocity.x < 0) {
                switch(z) {
                    case 0: _tile = ZOMBIELTILE0; break;
                    case 1: _tile = ZOMBIELTILE1; break;
                    case 2: _tile = ZOMBIELTILE2; break;
                }
            }
        }
    }
    bool colliding()
    {
        if (Character::colliding())
            return true;

        Vector tile = _position/Vector(TILEX, TILEY);
        for (int y = -1; y <= 1; ++y)
            for (int x = -1; x <= 1; ++x)
                if (gridColliding(tile + Vector(x, y)))
                    return true;
        return false;
    }
    bool gridColliding(Vector v)
    {
        Zombie* z = _game->_zombieGrid[v.y*TILESX + v.x];
        for (; z != 0; z = z->_next) {
            if (z == this)
                continue;
            Vector other = z->_position - _position;
            if (other.x >= TILEX || other.x + TILEX <= 0 || other.y >= TILEY ||
                other.y + TILEX <= 0)
                continue;
            for (int y = 0; y < TILEY; ++y) {
                if (y < other.y || y >= other.y + TILEY)
                    continue;
                for (int x = 0; x < TILEX; ++x) {
                    if (x < other.x || x >= other.x + TILEX)
                        continue;
                    UInt32 a = _tileData[(ZOMBIELTILE0*TILEY + y)*TILEX + x];  // _tile
                    if (static_cast<int>(a >> 24) <= 0x80)
                        continue;
                    Vector d = Vector(x, y) - other;
                    UInt32 b = _tileData[(ZOMBIELTILE0*TILEY + d.y)*TILEX + d.x];  // z._tile
                    if (static_cast<int>(b >> 24) > 0x80)
                        return true;
                }
            }
        }
        return false;
    }
    void addToGrid()
    {
        Vector tile = _position/Vector(TILEX, TILEY);
        int ti = tile.y*TILESX + tile.x;
        _next = _game->_zombieGrid[ti];
        _previous = &(_game->_zombieGrid[ti]);
        *_previous = this;
        if (_next != 0)
            _next->_previous = &_next;
    }
    int _jumpVelocity;
    unsigned int _animation;
    Zombie** _previous;
    Zombie* _next;
};

template<class T> class GameT
{
public:
    void run(HINSTANCE hInst)
    {
        auto textures =
            PNGFileFormat<DWORD>().load(File("textures1.png", false));

        File level("level.dat");

        _windowSize = Vector(912, 525);
        _tileData.allocate(TILEX*TILEY*TILETYPES);
        _tileGrid.allocate(TILESX*TILESY);
		_objectGrid.allocate(TILESX*TILESY);
        _zombieGrid.allocate(TILESX*TILESY);

        _player.init(this, 50*Vector(TILEX, TILEY) + _windowSize/2);
        spawnTile = (_player._position.x/TILEX) +
            (_player._position.y/TILEY)*TILESX;

        _spawnGrid.allocate(TILESX*TILESY);
		// Default level
        for (int y = 0; y < TILESY; ++y) {
            for (int x = 0; x < TILESX; ++x) {
                int ch;
                if (x < 15 || x >= TILESX-15 || y < 15 || y >= TILESY-15)
                    ch = (y == TILESY-15 ? 2 : 3);
                else
                    ch = SKYTILE;
                int i = y*TILESX + x;
                _tileGrid[i] = ch;
				_objectGrid[i] = 0;
                _spawnGrid[i] = ch;
                _zombieGrid[i] = 0;
            }
        }

		// Loaded level
        try {
            String l = level.contents();
            for (int i = 0; i < TILESX*TILESY; ++i) {
                int ch = l[i] & 0xff;
				_spawnGrid[i] = ch;
				Vector v = Vector((i % TILESX)*TILEX, (i / TILESX)*TILEY);
                if (ch == SHKGTILE) {
                    _player._position = v;
                    ch = SKYTILE;
                    spawnTile = i;
                }
                if (ch == ZOMBIELTILE0) {
                    Zombie zombie;
                    zombie.init(this, v);
                    _zombies.append(zombie);
                    ch = SKYTILE;
                }
                _tileGrid[i] = ch;

				if ()
				ch = l[i + TILESX*TILESY] & 0xff;

            }
        } catch (...) { }

        for (auto& z : _zombies)
            z.addToGrid();

        SDLWindow window;

        HICON hicon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
        if (hicon != 0) {
            struct SDL_SysWMinfo wmInfo;
            SDL_VERSION(&wmInfo.version);
            if (SDL_GetWindowWMInfo(window._window, &wmInfo) == -1)
                throw Exception("SDL_GetWindowWMInfo failed");
            HWND hWnd = wmInfo.info.win.window;
            LPARAM l = reinterpret_cast<LPARAM>(hicon);
            SendMessage(hWnd, WM_SETICON, ICON_BIG, l);
            SendMessage(hWnd, WM_SETICON, ICON_SMALL, l);
        }

        SDLRenderer renderer(&window);
        SDLTexture texture(&renderer);
        SDL_Event e;
        editing = false;
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

        int t = 0;

        do {
            UInt32 startTime = SDL_GetTicks();
            SDLTextureLock lock(&texture);
            UInt8* row = reinterpret_cast<UInt8*>(lock._pixels);
            int pitch = lock._pitch;
            Array<Byte>* grid = editing ? &_spawnGrid : &_tileGrid;
            Vector offset = _player._position - _windowSize/2;
            for (int y = 0; y < _windowSize.y; ++y) {
                UInt32* output = reinterpret_cast<UInt32*>(row);
                for (int x = 0; x < _windowSize.x; ++x) {
                    Vector v = Vector(x, y) + offset;
                    Vector tile = v/Vector(TILEX, TILEY);
                    int tn = (*grid)[tile.y*TILESX + tile.x];
                    v -= tile*Vector(TILEX, TILEY);
                    *output = _tileData[(tn*TILEY + v.y)*TILEX + v.x];
                    ++output;
                }
                row += pitch;
            }
            _player.draw(&lock);
            for (auto z : _zombies)
                z.draw(&lock);

            if (_player._gotKey) {
                // Draw key overlay
                row = reinterpret_cast<UInt8*>(lock._pixels);
                row += pitch*(_windowSize.y - TILEY);
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
                int paletteX = _windowSize.x - (4*TILEX + 16);
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
                        case SDLK_SPACE:
                            _up = (e.type == SDL_KEYDOWN);
                            break;
                        case SDLK_DOWN:
                            _down = (e.type == SDL_KEYDOWN);
                            break;
                        case SDLK_LEFT:
                            _left = (e.type == SDL_KEYDOWN);
                            break;
                        case SDLK_RIGHT:
                            _right = (e.type == SDL_KEYDOWN);
                            break;
                        case SDLK_e:
                            editing = true;
                            break;
                        case SDLK_p:
                            if (e.type == SDL_KEYDOWN)
                                _player.toggleLian();
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
                    if (mouseX >= _windowSize.x - (4*TILEX + 16) && mouseX < _windowSize.x - 16 && mouseY >= 16 && mouseY < 4*TILEY + 16) {
                        mouseX -= _windowSize.x - (4*TILEX + 16);
                        mouseY -= 16;
                        mouseX /= 16;
                        mouseY /= 16;
                        if (lButtonDown)
                            lTile = mouseY*16 + mouseX;
                        if (rButtonDown)
                            rTile = mouseY*16 + mouseX;
                    }
                    else {
                        Vector mTile = (Vector(mouseX, mouseY) + _player._position - _windowSize/2)/Vector(TILEX, TILEY);
                        if (mTile.x >= 15 && mTile.x < TILESX-15 && mTile.y >= 15 && mTile.y < TILESY-15) {
                            int i = mTile.y*TILESX + mTile.x;
                            int oldTile = _tileGrid[i];
                            int oldSpawn = _spawnGrid[i];
                            if (lButtonDown) {
                                _tileGrid[i] = lTile;
                                _spawnGrid[i] = lTile;
                            }
                            if (rButtonDown) {
                                _tileGrid[i] = rTile;
                                _spawnGrid[i] = rTile;
                            }
                            if (_player.colliding()) {
                                _tileGrid[i] = oldTile;
                                _spawnGrid[i] = oldSpawn;
                            }
                        }
                    }
                }
            }
            _player.move();
            for (auto& z : _zombies)
                z.move();

            UInt32 endTime = SDL_GetTicks();
            if (endTime - startTime < 16)
                SDL_Delay(16 - (endTime - startTime));
        } while (true);
    }
    ~GameT()
    {
        try {
            for (int i = 0; i < TILESX*TILESY; ++i) {
                if (_spawnGrid[i] == SHKGTILE)
                    spawnTile = i;
            }
            _spawnGrid[spawnTile] = SHKGTILE;
            File("level.dat").save(_spawnGrid);
        }
        catch (...) { }
    }
private:
    void plot(SDLTextureLock* lock, int x, int y, UInt32 colour)
    {
        *(reinterpret_cast<UInt32*>(reinterpret_cast<UInt8*>(lock->_pixels) + y*lock->_pitch) + x) = colour;
    }

    bool editing;
    int spawnTile;
    bool gotKey;
    Array<Byte> _spawnGrid;
public:
    Array<Zombie*> _zombieGrid;
    AppendableArray<Zombie> _zombies;
    bool _up;
    bool _down;
    bool _left;
    bool _right;
    Array<Byte> _tileGrid;
    Array<UInt32> _tileData;
    Vector _windowSize;
    Player _player;
};

class Program : public ProgramBase
{
public:
    void run()
    {
        Game game;
        game.run(_hInst);
    }
};