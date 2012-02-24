#include "alfe/main.h"

#ifndef INCLUDED_DIRECTX_H
#define INCLUDED_DIRECTX_H

#include <d3d9.h>
#ifdef _DEBUG
#include <dxerr.h>
#endif
#include <vector>
#include "alfe/vectors.h"
#include "alfe/user.h"
#include "alfe/pool.h"
#include "alfe/fix.h"
#include "alfe/com.h"

typedef Fixed<16, Int32, Int64> Fix16p16;

// This is the exception that we throw if any DirectX methods fail.
class DirectXException : public Exception
{
public:
    DirectXException(HRESULT hr)
#ifdef _DEBUG
      : Exception(DXGetErrorDescription(hr))
#endif
    { }
};

#define IF_ERROR_THROW_DX(expr) CODE_MACRO( \
    HRESULT hrMacro = (expr); \
    IF_TRUE_THROW(FAILED(hrMacro), DirectXException(hrMacro)); \
)


class Direct3D : Uncopyable
{
public:
    Direct3D()
    {
        // The *& causes the smart pointer to avoid the AddRef and take over
        // ownership of the raw pointer, causing the interface to be released
        // when the Direct3D object is destructed.
        *&_direct3D = Direct3DCreate9(D3D_SDK_VERSION);
        IF_FALSE_THROW(_direct3D.valid());
    }
private:
    COMPointer<IDirect3D9> _direct3D;

    friend class Device;
};


struct Vertex
{
    float x, y, z;
    float rhw;
    float u, v;
};


class Device : Uncopyable
{
public:
    Device(bool clampTextures = true) : _clampTextures(clampTextures),
        _filterType(D3DTEXF_FORCE_DWORD),
        _gammaCorrection(false),
        _backBufferSize(0, 0),
        _adapter(D3DADAPTER_DEFAULT)
    {
        ZeroMemory(&_parameters, sizeof(D3DPRESENT_PARAMETERS));
        _parameters.BackBufferCount = 1;
        _parameters.MultiSampleType = D3DMULTISAMPLE_NONE;
        _parameters.MultiSampleQuality = 0;
        _parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
        _parameters.Windowed = TRUE;
        _parameters.EnableAutoDepthStencil = FALSE;
        _parameters.AutoDepthStencilFormat = D3DFMT_D16;
        _parameters.Flags = 0;
        _parameters.FullScreen_RefreshRateInHz = 0;
        _parameters.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    }

    bool resetNeeded(Vector size)
    {
        return size.x > static_cast<int>(_parameters.BackBufferWidth) ||
            size.y > static_cast<int>(_parameters.BackBufferHeight);
    }

    void setSize(Vector size)
    {
        if (resetNeeded(size)) {
            growBackBuffer(size);
            reset();
        }
        D3DVIEWPORT9 viewport;
        viewport.X = 0;
        viewport.Y = 0;
        viewport.Width = size.x;
        viewport.Height = size.y;
        viewport.MinZ = 0.0f;
        viewport.MaxZ = 1.0f;
        IF_ERROR_THROW_DX(_device->SetViewport(&viewport));
        _size = size;
    }

    void create(Direct3D* direct3D, Window* window)
    {
        _direct3D = direct3D;
        initBackBuffer();
        _size = _backBufferSize;
        _parameters.hDeviceWindow = *window;
        // D3DCREATE_MULTITHREADED causes the Direct3D runtime to enter a
        // critical section for the duration of every Direct3D call, preventing
        // multiple Direct3D calls happening at once. It is not actually
        // needed here, since we are careful always to use our own locks to
        // prevent multiple Direct3D calls from happening at once. However, the
        // debug version of the Direct3D runtime complains if we call Direct3D
        // from multiple threads without this flag.
        IF_ERROR_THROW_DX(direct3D->_direct3D->CreateDevice(
            D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL,
            NULL,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING |
                D3DCREATE_FPU_PRESERVE /*| D3DCREATE_MULTITHREADED*/,
            &_parameters,
            &_device));
        postReset();
    }

    void destination()
    {
        IF_ERROR_THROW_DX(_device->SetRenderTarget(0, _renderTarget));
    }

    void setFilter(D3DTEXTUREFILTERTYPE type)
    {
        if (_filterType == type)
            return;
        _filterType = type;
        IF_ERROR_THROW_DX(
            _device->SetSamplerState(0, D3DSAMP_MINFILTER, type));
        IF_ERROR_THROW_DX(
            _device->SetSamplerState(0, D3DSAMP_MAGFILTER, type));
    }

    void setGammaCorrection(bool enabled)
    {
        if (_gammaCorrection == enabled)
            return;
        _gammaCorrection = enabled;
        IF_ERROR_THROW_DX(_device->SetRenderState(
            D3DRS_SRGBWRITEENABLE, enabled ? TRUE : FALSE));
    }

    void setTextureAddressingMode(D3DTEXTUREADDRESS mode)
    {
        IF_ERROR_THROW_DX(_device->SetSamplerState(0, D3DSAMP_ADDRESSU, mode));
        IF_ERROR_THROW_DX(_device->SetSamplerState(0, D3DSAMP_ADDRESSV, mode));
    }

    void reset()
    {
        _renderTarget = 0;
        initBackBuffer();
        IF_ERROR_THROW_DX(_device->Reset(&_parameters));
        postReset();
    }

    void present()
    {
        HRESULT hr = _device->Present(
            NULL,    // pSourceRect
            NULL,    // pDestRect
            NULL,    // hDestWindowOverride
            NULL);   // pDirtyRegion
        if (hr == D3DERR_DEVICELOST)
            reset();
        else
            IF_ERROR_THROW_DX(hr);
    }

    Vector2<float> transform() const
    {
        return Vector2Cast<float>(_backBufferSize)/Vector2Cast<float>(_size);
    }

private:
    void growBackBuffer(Vector size)
    {
        _parameters.BackBufferWidth = _backBufferSize.x =
            max(static_cast<int>(_parameters.BackBufferWidth), size.x);
        _parameters.BackBufferHeight = _backBufferSize.y =
            max(static_cast<int>(_parameters.BackBufferHeight), size.y);
    }

    void initBackBuffer()
    {
        D3DDISPLAYMODE displayMode;
        IF_ERROR_THROW_DX(_direct3D->_direct3D->
            GetAdapterDisplayMode(_adapter, &displayMode));
        _parameters.BackBufferFormat = displayMode.Format;
        growBackBuffer(Vector(displayMode.Width, displayMode.Height));
    }

    void postReset()
    {
        IF_ERROR_THROW_DX(_device->GetRenderTarget(0, &_renderTarget));
        IF_ERROR_THROW_DX(_device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1));
        IF_ERROR_THROW_DX(_device->SetRenderState(D3DRS_LIGHTING, FALSE));
        if (_clampTextures)
            setTextureAddressingMode(D3DTADDRESS_CLAMP);
    }

    Direct3D* _direct3D;
    UINT _adapter;
    COMPointer<IDirect3DDevice9> _device;
    D3DPRESENT_PARAMETERS _parameters;
    COMPointer<IDirect3DSurface9> _renderTarget;
    bool _clampTextures;
    D3DTEXTUREFILTERTYPE _filterType;
    bool _gammaCorrection;
    Vector _backBufferSize;
    Vector _size;

    friend class Texture;
    friend class Geometry;
    friend class Scene;
    friend class GPUTexture;
    friend class CPUTexture;
    friend class Tile;
    template<class TexelSource> friend class GridRenderer;
};


class Geometry : Uncopyable
{
public:
    void create(Device* device, int size, D3DPRIMITIVETYPE type)
    {
        _device = device;
        _size = size;
        _type = type;
        postReset();
    }

    void draw()
    {
        IF_ERROR_THROW_DX(_device->_device->SetStreamSource(
            0,                 // StreamNumber
            _vertexBuffer,     // pStreamData
            0,                 // OffsetInBytes
            sizeof(Vertex)));  // Stride
        IF_ERROR_THROW_DX(
            _device->_device->DrawPrimitive(_type, 0, _size - 2));
    }

    void preReset() { _vertexBuffer = 0; }

    void postReset()
    {
        IF_ERROR_THROW_DX(_device->_device->CreateVertexBuffer(
            sizeof(Vertex)*_size,                   // Length
            D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,  // Usage
            D3DFVF_XYZRHW | D3DFVF_TEX1,            // FVF
            D3DPOOL_DEFAULT,                        // Pool
            &_vertexBuffer,                         // ppVertexBuffer
            NULL));                                 // pSharedHandle
    }

private:
    Device* _device;
    int _size;
    D3DPRIMITIVETYPE _type;
    COMPointer<IDirect3DVertexBuffer9> _vertexBuffer;

    friend class GeometryLock;
};


class GeometryLock : Uncopyable
{
public:
    GeometryLock(Geometry* geometry) : _geometry(geometry)
    {
        VOID* data;
        // Lock the entire geometry.
        IF_ERROR_THROW_DX(_geometry->_vertexBuffer->Lock(
            0,                  // OffsetToLock
            0,                  // SizeToLock
            &data,              // ppbData
            D3DLOCK_DISCARD));  // Flags
        _vertices = reinterpret_cast<Vertex*>(data);
        for (int i = 0; i < _geometry->_size; ++i) {
            _vertices[i].rhw = 1.0f;
            _vertices[i].z = 0.0f;
        }
    }

    ~GeometryLock() { _geometry->_vertexBuffer->Unlock(); }

    void setXY(int n, Vector2<float> position)
    {
        _vertices[n].x = position.x;
        _vertices[n].y = position.y;
    }

    void setUV(int n, Vector2<float> texturePosition)
    {
        _vertices[n].u = texturePosition.x;
        _vertices[n].v = texturePosition.y;
    }
private:
    Geometry* _geometry;
    Vertex* _vertices;
};


class Quad : public Geometry
{
public:
    void create(Device* device)
    {
        Geometry::create(device, 4, D3DPT_TRIANGLESTRIP);
    }
};


// Base class for CPUTexture and GPUTexture.
class Texture : Uncopyable
{
protected:
    void create(Device* device, Vector2<int> size, DWORD usage, D3DPOOL pool)
    {
        _device = device;
        IF_ERROR_THROW_DX(device->_device->CreateTexture(
            size.x,           // Width
            size.y,           // Height
            1,                // Levels
            usage,            // Usage
            D3DFMT_A8R8G8B8,  // Format
            pool,             // Pool
            &_texture,        // ppTexture
            NULL));           // pSharedHandle
    }

    Device* _device;
    COMPointer<IDirect3DTexture9> _texture;

    friend class CPUTextureLock;
    friend class CPUTexture;
};


// A texture in GPU memory. Used as render sources and render targets.
class GPUTexture : public Texture, public LinkedListMember<GPUTexture>
{
public:
    void create(Device* device, Vector size)
    {
        _device = device;
        _size = size;
        postReset();
        plotBlock(Vector(0, 0), 256, 0x808080);
    }

    void plotBlock(Vector texel, int size, DWORD colour)
    {
        RECT rect;
        //if (size > 1) {
        //    rect.left = texel.x;
        //    rect.top = texel.y;
        //    rect.right = texel.x + size;
        //    rect.bottom = texel.y + size;
        //    IF_ERROR_THROW_DX(
        //         _device->_device->ColorFill(_surface, &rect, 0xffffff));
        //
        //    rect.left = texel.x;
        //    rect.top = texel.y;
        //    rect.right = texel.x + size - 1;
        //    rect.bottom = texel.y + size - 1;
        //    IF_ERROR_THROW_DX(
        //         _device->_device->ColorFill(_surface, &rect, colour));
        //    return;
        //}
        rect.left = texel.x;
        rect.top = texel.y;
        rect.right = texel.x + size;
        rect.bottom = texel.y + size;
        IF_ERROR_THROW_DX(
             _device->_device->ColorFill(_surface, &rect, colour));
    }

    void source()
    {
        IF_ERROR_THROW_DX(_device->_device->SetTexture(0, _texture));
    }

    void destination()
    {
        IF_ERROR_THROW_DX(_device->_device->SetRenderTarget(0, _surface));
    }

    void preReset() { _surface = 0; _texture = 0; }

    void postReset()
    {
        Texture::create(_device, _size, D3DUSAGE_RENDERTARGET,
            D3DPOOL_DEFAULT);
        IF_ERROR_THROW_DX(_texture->GetSurfaceLevel(0, &_surface));
    }

private:
    Device* _device;
    Vector _size;
    COMPointer<IDirect3DSurface9> _surface;

    friend class CPUTexture;
};


// A texture in CPU memory. When a device is lost it is generally faster
// (especially when there is a lot of detail) to redraw the texture in CPU
// memory and then blit it to the GPU.
class CPUTexture : public Texture
{
public:
    void create(Device* device, Vector2<int> size)
    {
        Texture::create(device, size, 0, D3DPOOL_SYSTEMMEM);
    }

    void update(GPUTexture* target)
    {
        IF_ERROR_THROW_DX(
            _device->_device->UpdateTexture(_texture, target->_texture));
    }
};


// A lock on a CPU texture which can be used to plot blocks.
class CPUTextureLock : Uncopyable
{
public:
    CPUTextureLock(CPUTexture* texture) : _texture(texture)
    {
        // Lock the entire texture - this is only used when we're plotting a
        // new tile or when the device is lost. In both of these cases we need
        // to update the entire texture anyway.
        IF_ERROR_THROW_DX(_texture->_texture->LockRect(
            0,           // Level
            &_lockData,  // pLockedRect
            NULL,        // pRect
            0));         // Flags
    }

    ~CPUTextureLock() { _texture->_texture->UnlockRect(0); }

    void plotBlock(Vector texel, int size, DWORD colour)
    {
        int pitch = _lockData.Pitch;
        Byte* p = reinterpret_cast<Byte*>(_lockData.pBits) + texel.y*pitch +
            texel.x*4;
        for (int y = 0; y < size; ++y) {
            DWord* l = reinterpret_cast<DWord*>(p);
            for (int x = 0; x < size; ++x)
                *l++ = colour;
            p += pitch;
        }
    }

private:
    CPUTexture* _texture;
    D3DLOCKED_RECT _lockData;
};


template<class TexelSource> class TowerGrid;
template<class TexelSource> class GridRenderer;


template<class TexelSource> class Tower
  : public LinkedListMember<Tower<TexelSource> >
{
    typedef GridRenderer<TexelSource> GridRenderer;
    typedef TowerGrid<TexelSource> TowerGrid;
public:
    Tower(GridRenderer* renderer)
      : _renderer(renderer),
        _dirty(false),
        _logTexelsPerTile(_renderer->logTexelsPerTile())
    {
        int s = 1 << _logTexelsPerTile;
        _texture.create(_renderer->device(), Vector(s, s));
        for (int i = 0; i < 4; ++i)
            _subTowers[i] = 0;
    }

    void initialize(int logTilesPerTower, TowerGrid* towerGrid)
    {
        _dirty = true;
        _towerGrid = towerGrid;
        _logTilesPerTower = logTilesPerTower;
        _shift = logTilesPerTower + _renderer->logTexelsPerTile() - 1;
        _mask = (1 << _shift) - 1;
    }

    void disassemble()
    {
        if (_logTilesPerTower != 0)
            disassembleSubTowers();
        _renderer->release(this);
    }

    void paint(Vector position)
    {
        if (_dirty) {
            if (_logTilesPerTower > 0)
                for (int i = 0; i < 4; ++i) {
                    Tower* tower = _subTowers[i];
                    if (tower == 0) {
                        tower = _renderer->aquire();
                        _subTowers[i] = tower;
                        tower->initialize(_logTilesPerTower - 1, _towerGrid);
                    }
                    Vector quadrant = quadrantFromIndex(i);
                    tower->paint((position << 1) | quadrant);
                    _renderer->quarter(quadrant);
                    _texture.destination();
                    _renderer->clean();
                }
            else {
                CPUTexture* cpuTexture = _renderer->cpuTexture();
                {
                    CPUTextureLock lock(cpuTexture);
                    _lock = &lock;
                    _towerGrid->_texelSource->plotTile(position);
                }
                cpuTexture->update(&_texture);
            }
            _dirty = false;
        }
        _texture.source();
        if (_logTilesPerTower == _renderer->logTilesPerTower()) {
            {
                GeometryLock lock(_renderer->quad());
                for (int i = 0; i < 4; ++i) {
                    Vector xy = quadrantFromIndex(i);
                    lock.setUV(i, Vector2Cast<float>(xy));
                    lock.setXY(i, _towerGrid->pixelForTower(position + xy)*
                        _renderer->device()->transform());
                }
            }
            _renderer->paint();
        }
    }

    void plot(Vector texel, int size, DWORD colour)
    {
        if (_logTilesPerTower == 0) {
            _texture.plotBlock(texel, size, colour);
            return;
        }
        _dirty = true;
        Vector quadrant = texel >> _shift;
        Tower* tower = _subTowers[(quadrant.y << 1) | quadrant.x];
        if (tower != 0)
            tower->plot(texel & _mask, size, colour);
    }

    void tilePlot(Vector texel, int size, DWORD colour)
    {
        if (_logTilesPerTower == 0) {
            _lock->plotBlock(texel, size, colour);
            return;
        }
        Vector quadrant = texel >> _shift;
        _subTowers[(quadrant.y << 1) | quadrant.x]->
            tilePlot(texel & _mask, size, colour);
    }

    void augment()
    {
        ++_logTilesPerTower;
        ++_shift;
        _mask = (1 << _shift) - 1;
        if (_logTilesPerTower > 1) {
            for (int i = 0; i < 4; ++i)
                _subTowers[i]->augment();
            return;
        }
        _texture.source();
        for (int i = 0; i < 4; ++i) {
            Tower* tower = _renderer->aquire();
            tower->initialize(0, _towerGrid);
            _subTowers[i] = tower;
            tower->destination();
            _renderer->expandQuarter(quadrantFromIndex(i));
            tower->_dirty = false;
        }
    }

    void diminish()
    {
        if (_logTilesPerTower > 1) {
            for (int i = 0; i < 4; ++i)
                _subTowers[i]->diminish();
            return;
        }
        --_logTilesPerTower;
        --_shift;
        _mask = (1 << _shift) - 1;
        _texture.destination();
        for (int i = 0; i < 4; ++i) {
            Tower* tower = _subTowers[i];
            if (tower != 0) {
                tower->source();
                _renderer->quarter(quadrantFromIndex(i));
                _renderer->scale();
            }
        }
        disassembleSubTowers();
    }

    Tower* split(int i)
    {
        Tower* tower = _subTowers[i];
        _subTowers[i] = 0;
        if (tower != 0)
            tower->augment();
        return tower;
    }

    void combine(int i, Tower* tower)
    {
        tower->diminish();
        _subTowers[i] = tower;
    }

private:
    void disassembleSubTowers()
    {
        for (int i = 0; i < 4; ++i) {
            Tower* tower = _subTowers[i];
            if (tower != 0) {
                tower->disassemble();
                _subTowers[i] = 0;
            }
        }
    }

    void source() { _texture.source(); }
    void destination() { _texture.destination(); }

    static Vector quadrantFromIndex(int i) { return Vector(i&1, i>>1); }

    GridRenderer* _renderer;
    bool _dirty;
    GPUTexture _texture;
    int _logTilesPerTower;
    int _logTexelsPerTile;
    int _shift;
    int _mask;
    Tower* _subTowers[4];
    TowerGrid* _towerGrid;
    CPUTextureLock* _lock;
};


template<class TexelSource> class TowerGrid : Uncopyable
{
    typedef Tower<TexelSource> Tower;
    typedef GridRenderer<TexelSource> GridRenderer;
public:
    TowerGrid(GridRenderer* renderer, TexelSource* texelSource)
      : _renderer(renderer),
        _texelSource(texelSource),
        _logTexelsPerTile(renderer->logTexelsPerTile()),
        _logTilesPerTower(renderer->logTilesPerTower()),
        _logTexelsPerTower(_logTexelsPerTile + _logTilesPerTower),
        _texelsPerTower(1 << _logTexelsPerTower),
        _texelMask(_texelsPerTower - 1),
        _bufferSize(1, 1),
        _size(1, 1),
        _towers(1, 0)
    {
        renderer->addGrid(this);
    }
    ~TowerGrid() { _renderer->removeGrid(this); }

    // Methods used by Screen

    void split()
    {
        enlargeBuffer(_size << 1);
        for (int y = _size.y - 1; y >= 0; --y)
            for (int x = _size.x - 1; x >= 0; --x) {
                Vector xy(x, y);
                Vector xy2 = xy << 1;
                Tower* tower = towerAt(xy);
                for (int i = 0; i < 4; ++i) {
                    Vector position = xy2 + quadrantFromIndex(i);
                    if (tower != 0)
                        setTower(position, tower->split(i));
                    else
                        setTower(position, 0);
                }
                if (tower != 0)
                    _renderer->release(tower);
            }
        _size <<= 1;
    }

    void combine(Vector offset)
    {
        for (int y = -offset.y; y < _size.y; y += 2)
            for (int x = -offset.x; x < _size.x; x += 2) {
                Vector xy2(x, y);
                Vector xy = (xy2 + offset) >> 1;
                for (int i = 0; i < 4; ++i) {
                    Vector position = xy2 + quadrantFromIndex(i);
                    if (position.inside(_size)) {
                        Tower* tower = towerAt(position);
                        if (tower != 0)
                            _renderer->combine(tower, i, this);
                        setTower(position, 0);
                    }
                }
                Tower* tower = _renderer->combined();
                setTower(xy, tower);
            }
        _size = (_size + offset + 1) >> 1;
    }

    void move(Vector topLeftTower, Vector bottomRightTower)
    {
        Vector newSize = bottomRightTower - topLeftTower;
        enlargeBuffer(newSize);

        // Release towers that we no longer need
        for (int y = 0; y < _size.y; ++y) {
            for (int x = 0; x < topLeftTower.x; ++x)
                release(Vector(x, y));
            for (int x = bottomRightTower.x; x < _size.x; ++x)
                release(Vector(x, y));
        }
        for (int x = 0; x < _size.x; ++x) {
            for (int y = 0; y < topLeftTower.y; ++y)
                release(Vector(x, y));
            for (int y = bottomRightTower.y; y < _size.y; ++y)
                release(Vector(x, y));
        }

        // Move tiles
        Vector start(0, 0);
        Vector end = newSize;
        Vector delta(1, 1);
        if (topLeftTower.x < 0) {
            start.x = newSize.x - 1;
            end.x = -1;
            delta.x = -1;
        }
        if (topLeftTower.y < 0) {
            start.y = newSize.y - 1;
            end.y = -1;
            delta.y = -1;
        }
        for (int y = start.y; y != end.y; y += delta.y)
            for (int x = start.x; x != end.x; x += delta.x) {
                Vector xy(x, y);
                Vector xy2 = xy + topLeftTower;
                if (xy2.inside(_size))
                    setTower(xy, towerAt(xy2));
                else
                    setTower(xy, 0);
            }
        _size = newSize;
    }

    void plot(Vector texel, int size, DWORD colour)
    {
        Tower* tower = towerAt(texel >> _logTexelsPerTower);
        if (tower != 0)
            tower->plot(texel & _texelMask, size, colour);
    }

    void update()
    {
        for (int y = 0; y < _size.y; ++y)
            for (int x = 0; x < _size.x; ++x) {
                Vector xy(x, y);
                Tower* tower = towerAt(xy);
                if (_texelSource->texelAreaVisible(xy << _logTexelsPerTower,
                    _texelsPerTower)) {
                    if (tower == 0) {
                        tower = _renderer->aquire();
                        tower->initialize(_logTilesPerTower, this);
                        setTower(xy, tower);
                    }
                }
                else
                    if (tower != 0) {
                        tower->disassemble();
                        setTower(xy, 0);
                    }
            }
    }

    void tilePlot(Vector texel, int size, DWORD colour)
    {
//        printf("Plotting tile at %08x,%08x size %i colour %i\n",texel.x,texel.y,size,colour);
        towerAt(texel >> _logTexelsPerTower)->
            tilePlot(texel & _texelMask, size, colour);
    }

    Vector size() const { return _size; }
    int logTexelsPerTile() { return _logTexelsPerTile; }
    int logTilesPerTower() { return _logTilesPerTower; }

    // Methods called by ImageWindow

    void paint(const PaintHandle& paint)
    {
        for (int y = 0; y < _size.y; ++y)
            for (int x = 0; x < _size.x; ++x) {
                Vector position(x, y);
                Tower* tower = towerAt(position);
                if (tower != 0)
                    tower->paint(position);
            }
    }

    void resize(Vector newSize)
    {
        _texelSource->resize(newSize);
        _renderer->resize(newSize);
    }

    void doneResize() { _renderer->deleteReserve(); }


    // Methods called by ZoomingRotatingWindow

    void beginScene() { _renderer->beginScene(); }

    void endScene() { _renderer->endScene(); }

    void zoomRotate(Fix16p16 zoomLevel, Fix16p16 angle,
        Vector zoomPosition, bool panning)
    {
        _texelSource->
            zoomRotate(zoomLevel, angle, zoomPosition, panning, false);
    }

    void setInterrupt() { _texelSource->setInterrupt(); }
    void clearInterrupt() { _texelSource->clearInterrupt(); }

    void resume() { _texelSource->resume(); }

    void noRegionChange() { _texelSource->noRegionChange(); }


    // Methods called by GridRenderer

    void preReset()
    {
        for (int y = 0; y < _size.y; ++y)
            for (int x = 0; x < _size.x; ++x) {
                Vector xy(x, y);
                Tower* tower = towerAt(xy);
                if (tower != 0) {
                    tower->disassemble();
                    setTower(xy, 0);
                }
            }
    }


    // Methods called by Tower

    Vector2<float> pixelForTower(Vector towerPosition)
    {
        return _texelSource->
            pixelFromTexel(towerPosition << _logTexelsPerTower);
    }

    void destroy() { }

private:
    Tower* towerAt(Vector towerPosition)
    {
        if (!towerPosition.inside(_size))
            return 0;
        return _towers[towerPosition.y*_bufferSize.x + towerPosition.x];
    }

    void setTower(Vector towerPosition, Tower* tower)
    {
        _towers[towerPosition.y*_bufferSize.x + towerPosition.x] = tower;
    }

    void enlargeBuffer(Vector newSize)
    {
        newSize = Vector(max(newSize.x, _bufferSize.x),
            max(newSize.y, _bufferSize.y));
        if (_bufferSize == newSize)
            return;
        _towers.resize(newSize.x * newSize.y, 0);
        for (int y = newSize.y - 1; y >= 0; --y)
            for (int x = newSize.x - 1; x >= 0; --x)
                _towers[y*newSize.x + x] = towerAt(Vector(x, y));
        _bufferSize = newSize;
    }

    void release(Vector towerPosition)
    {
        Tower* tower = towerAt(towerPosition);
        if (tower != 0)
            tower->disassemble();
        if (towerPosition.inside(_size))  // TODO: Needed?
            setTower(towerPosition, 0);
    }

    static Vector quadrantFromIndex(int i) { return Vector(i&1, i>>1); }

    Vector _size;
    Vector _bufferSize;
    int _logTexelsPerTile;
    int _logTilesPerTower;
    int _logTexelsPerTower;
    int _texelsPerTower;
    int _texelMask;
    std::vector<Tower*> _towers;
    TexelSource* _texelSource;
    GridRenderer* _renderer;

    friend class Tower;
};


template<class TexelSource> class GridRenderer : Uncopyable
{
    typedef Tower<TexelSource> Tower;
    typedef TowerGrid<TexelSource> TowerGrid;
public:
    GridRenderer(int logTilesPerTower, int logTexelsPerTile)
      : _logTilesPerTower(logTilesPerTower),
        _logTexelsPerTile(logTexelsPerTile),
        _combined(0)
    {
        _pool.create(this);
    }

    void setDevice(Device* device)
    {
        _device = device;
        _geometry.create(device);
        int s = 1 <<  _logTexelsPerTile;
        _cpuTexture.create(device, Vector(s, s));
    }

    Tower* create() { return new Tower(this); }
    Tower* aquire() { return _pool.aquire(); }
    void release(Tower* tower) { _pool.release(tower); }
    CPUTexture* cpuTexture() { return &_cpuTexture; }
    void uploadTexture(GPUTexture* texture) { _cpuTexture.update(texture); }

    void beginScene() { _device->_device->BeginScene(); }

    void endScene()
    {
        _device->_device->EndScene();
        HRESULT hr = _device->_device->Present(
            NULL,    // pSourceRect
            NULL,    // pDestRect
            NULL,    // hDestWindowOverride
            NULL);   // pDirtyRegion
        if (hr == D3DERR_DEVICELOST) {
            preReset();
            _device->reset();
            postReset();
        }
        else
            IF_ERROR_THROW_DX(hr);
    }

    void resize(Vector size)
    {
        bool reset = false;
        if (_device->resetNeeded(size))  {
            preReset();
            reset = true;
        }
        _device->setSize(size);
        if (reset)
            postReset();
    }

    void deleteReserve() { _pool.deleteReserve(); }

    void preReset()
    {
        _geometry.preReset();
        for (std::vector<TowerGrid*>::size_type i = 0; i < _grids.size(); ++i)
            _grids[i]->preReset();
        deleteReserve();
    }

    void postReset()
    {
        _geometry.postReset();
        for (std::vector<TowerGrid*>::size_type i = 0; i < _grids.size(); ++i)
            _grids[i]->update();
    }

    int logTilesPerTower() const { return _logTilesPerTower; }
    int logTexelsPerTile() const { return _logTexelsPerTile; }

    Geometry* quad() { return &_geometry; }

    void paint()
    {
        _device->destination();
        _device->setFilter(D3DTEXF_LINEAR);
        _device->setGammaCorrection(true);
        _geometry.draw();
    }

    void quarter(Vector quadrant)
    {
        GeometryLock lock(&_geometry);
        for (int i = 0; i < 4; ++i) {
            Vector xy = quadrantFromIndex(i);
            lock.setUV(i, Vector2Cast<float>(xy));
            lock.setXY(i, Vector2Cast<float>(
                (xy + quadrant) << (_logTexelsPerTile - 1)));
        }
    }

    void clean()
    {
        _device->setFilter(D3DTEXF_LINEAR);
        _device->setGammaCorrection(false);
        _geometry.draw();
    }

    void combine(Tower* subTower, int i, TowerGrid* towerGrid)
    {
        if (_combined == 0) {
            _combined = aquire();
            _combined->initialize(_logTilesPerTower, towerGrid);
        }
        _combined->combine(i, subTower);
    }

    Tower* combined()
    {
        Tower* tower = _combined;
        if (tower == 0)
            return 0;
        _combined = 0;
        return tower;
    }

    void expandQuarter(Vector quarter)
    {
        {
            GeometryLock lock(&_geometry);
            for (int i = 0; i < 4; ++i) {
                Vector xy = quadrantFromIndex(i);
                lock.setUV(i, Vector2Cast<float>(xy + quarter)*0.5f);
                lock.setXY(i, Vector2Cast<float>(xy << _logTexelsPerTile));
            }
        }
        scale();
    }

    void scale()
    {
        _device->setFilter(D3DTEXF_POINT);
        _device->setGammaCorrection(false);
        _geometry.draw();
    }

    Device* device() { return _device; }

    void addGrid(TowerGrid* grid)
    {
        _grids.push_back(grid);
    }

    void removeGrid(TowerGrid* grid)
    {
        bool found = false;
        for (std::vector<TowerGrid*>::size_type i = 0; i < _grids.size() - 1;
            ++i) {
            if (_grids[i] == grid)
                found = true;
            if (found)
                _grids[i] = _grids[i + 1];
        }
        _grids.pop_back();
    }
private:
    static Vector quadrantFromIndex(int i) { return Vector(i&1, i>>1); }

    int _logTilesPerTower;
    int _logTexelsPerTile;
    CPUTexture _cpuTexture;
    Device* _device;
    Pool<Tower, GridRenderer> _pool;
    std::vector<TowerGrid*> _grids;
    Quad _geometry;
    Tower* _combined;
};


#endif // INCLUDED_DIRECTX_H
