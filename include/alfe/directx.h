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
        //_backBufferSize(0, 0),
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

    //bool resetNeeded(Vector size)
    //{
    //    /*return true;*/
    //    return size.x > static_cast<int>(_parameters.BackBufferWidth) ||
    //        size.y > static_cast<int>(_parameters.BackBufferHeight);
    //}

    void setSize(Vector size)
    {
        //printf("Resizing to %i,%i\n",size.x, size.y);
        //_parameters.BackBufferWidth = _backBufferSize.x = 800;
        //_parameters.BackBufferHeight = _backBufferSize.y = 600;
        //reset();

        //if (resetNeeded(size)) {
            //growBackBuffer(size);
            _parameters.BackBufferWidth = size.x;
            _parameters.BackBufferHeight = size.y;
            reset();
        //}
        D3DVIEWPORT9 viewport;
        viewport.X = 0;
        viewport.Y = 0;
        viewport.Width = size.x;
        viewport.Height = size.y;
        viewport.MinZ = 0.0f;
        viewport.MaxZ = 1.0f;
        IF_ERROR_THROW_DX(_device->SetViewport(&viewport));
        //_size = size;
    }

    void create(Direct3D* direct3D, Window* window)
    {
        _direct3D = direct3D;
        initBackBuffer();
        Vector size = window->getSize();
        _parameters.BackBufferWidth = size.x;
        _parameters.BackBufferHeight = size.y;

        //_size = _backBufferSize;
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
        //printf("Reset\n");
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
        //return Vector2Cast<float>(_backBufferSize + Vector(1,1))/Vector2Cast<float>(_size);
        return Vector2<float>(1.0f, 1.0f);
    }

private:
    //void growBackBuffer(Vector size)
    //{
    //    /*_parameters.BackBufferWidth = 0;
    //    _parameters.BackBufferHeight = 0; */

    //    _parameters.BackBufferWidth = _backBufferSize.x =
    //        max(static_cast<int>(_parameters.BackBufferWidth), size.x);
    //    _parameters.BackBufferHeight = _backBufferSize.y =
    //        max(static_cast<int>(_parameters.BackBufferHeight), size.y);

    //    //_parameters.BackBufferWidth = _backBufferSize.x = 800;
    //    //_parameters.BackBufferHeight = _backBufferSize.y = 600;
    //}

    void initBackBuffer()
    {
        D3DDISPLAYMODE displayMode;
        IF_ERROR_THROW_DX(_direct3D->_direct3D->
            GetAdapterDisplayMode(_adapter, &displayMode));

        _parameters.BackBufferFormat = displayMode.Format;
        //growBackBuffer(Vector(displayMode.Width, displayMode.Height));
    }

    void postReset()
    {
        IF_ERROR_THROW_DX(_device->GetRenderTarget(0, &_renderTarget));
        IF_ERROR_THROW_DX(_device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1));
        IF_ERROR_THROW_DX(_device->SetRenderState(D3DRS_LIGHTING, FALSE));
        if (_clampTextures)
            setTextureAddressingMode(D3DTADDRESS_CLAMP);
        D3DTEXTUREFILTERTYPE filterType = _filterType;
        _filterType = D3DTEXF_FORCE_DWORD;
        setFilter(filterType);
    }

    Direct3D* _direct3D;
    UINT _adapter;
    COMPointer<IDirect3DDevice9> _device;
    D3DPRESENT_PARAMETERS _parameters;
    COMPointer<IDirect3DSurface9> _renderTarget;
    bool _clampTextures;
    D3DTEXTUREFILTERTYPE _filterType;
    bool _gammaCorrection;
    //Vector _backBufferSize;
    //Vector _size;

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

    int stride() const { return _lockData.Pitch; }
    Byte* data() const { return reinterpret_cast<Byte*>(_lockData.pBits); }

    void plotBlock(Vector texel, int size, DWORD colour)
    {
        int pitch = stride();
        Byte* p = data() + texel.y*pitch + texel.x*4;
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


class Renderer
{
public:

};


#endif // INCLUDED_DIRECTX_H
