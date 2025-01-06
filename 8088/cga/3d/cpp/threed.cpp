#include "alfe/main.h"
#include "alfe/bitmap_png.h"
#include "alfe/colour_space.h"
#include "alfe/user.h"
#include "alfe/cga.h"
//#include "alfe/config_file.h"


template<class T> class LogTable
{
public:
    LogTable(double k, int p)
    {
        _entries.allocate(0x8000);
        for (int i = 0; i < 0x8000; ++i) {
            int ii = (i << 1) + 1;
            if (i >= 0x8000)
                ii = 0x10000 - i;
            double l = k*log(ii) + p;
            int ll = static_cast<int>(round(l/2))*2 + (ii < 0 ? 0x8000 : 0);
            entries[i] = ll;
        }
    }
    T operator[](int a)
    {
        if ((a & 1) != 0)
            throw Exception("Misaligned access");
        return _entries[(a & 0xfffe) >> 1];
    }
private:
    Array<T> _entries;
};

double logBase = 0x3fff/log(0x8001);

LogTable<SInt16> logTable(logBase, 0);

template<class T> class ExpTable
{
public:
    ExpTable(double k, double n, int maximum)
    {
        _entries.allocate(0x8000);
        for (int i = 0; i < 0x8000; ++i) {
            int ii = i << 1;
            if (i >= 0x8000)
                ii = i - 0x8000;
            double e = n*exp(ii/k);
            int ee = static_cast<int>(round(e));
            if (ee >= maximum)
                ee = 0x8000;
            if (i >= 0x8000)
                ee = -ee;
            entries[i] = ee;
        }
    }
    T operator[](int a)
    {
        if ((a & 1) != 0)
            throw Exception("Misaligned access");
        return _entries[(a & 0xfffe) >> 1];
    }
private:
    Array<T> _entries;
};

ExpTable<SInt16> expTable(0x3fff/log(0x8001), 640, 0x4000);

template<class T> class SquareTable
{
public:
    SquareTable(int end, double divisor, T nmask = 0) : _end(end)
    {
        _entries.allocate(end*2 - 1);
        for (int i = -end; i <= end; ++i)
            _entries[i + end] = static_cast<T>(i*i / divisor) & ~nmask;
    }
    T operator[](int a)
    {
        if ((a & 1) != 0)
            throw Exception("Misaligned access");
        if (a < -_end || a > _end)
            throw Exception("Table access out of range");
        return _entries[a + _end];
    }
private:
    Array<T> _entries;
    int _end;
};

SquareTable<Fix8p8> squareTable(0x101, 0x100);
SquareTable<Fix8p8> quarterSquareTable(0x101, 0x400, 1/256.0);

struct ViewParameters
{
public:
    Fix8p8 deltaX(Fix8p8 forward, Fix8p8 right, Fix8p8 down)
    {
        //return forward*_fx + right*_rx + down*_dx;
        return
            quarterSquareTable[forward._v + _fx._v]
            - quarterSquareTable[forward._v - _fx._v]
            + quarterSquareTable[right._v + _rx._v]
            - quarterSquareTable[right._v - _rx._v]
            + quarterSquareTable[down._v + _dx._v]
            - quarterSquareTable[down._v - _dx._v];
    }
    Fix8p8 deltaY(Fix8p8 forward, Fix8p8 right, Fix8p8 down)
    {
        //return forward*_fy + right*_ry + down*_dy;
        return
            quarterSquareTable[forward._v + _fy._v]
            - quarterSquareTable[forward._v - _fy._v]
            + quarterSquareTable[right._v + _ry._v]
            - quarterSquareTable[right._v - _ry._v]
            + quarterSquareTable[down._v + _dy._v]
            - quarterSquareTable[down._v - _dy._v];
    }
    Fix8p8 deltaZ(Fix8p8 forward, Fix8p8 right, Fix8p8 down)
    {
        //return forward*_fz + right*_rz + down*_dz;
        return
            quarterSquareTable[forward._v + _fy._v]
            - quarterSquareTable[forward._v - _fy._v]
            + quarterSquareTable[right._v + _ry._v]
            - quarterSquareTable[right._v - _ry._v]
            + quarterSquareTable[down._v + _dy._v]
            - quarterSquareTable[down._v - _dy._v];
    }

    Fix8p8 _fx;
    Fix8p8 _fy;
    Fix8p8 _fz;
    Fix8p8 _dx;
    Fix8p8 _dy;
    Fix8p8 _dz;
    Fix8p8 _rx;
    Fix8p8 _ry;
    Fix8p8 _rz;
    SInt16 _ex;
    SInt16 _ey;
    SInt16 _ez;
};

ViewParameters view;

struct Vertex
{
    SInt16 _x;
    SInt16 _y;
    SInt16 _z;
    SInt16 _sz;
    SInt16 _px;
    SInt16 _py;

    void transform()
    {
        SInt16 x = _x - view._ex;
        SInt16 y = _y - view._ey;
        SInt16 z = _z - view._ez;
        //_sx = x * view._rx + y * view._ry + z * view._rz;
        //_sy = x * view._dx + y * view._dy + z * view._dz;
        //_sz = x * view._fx + y * view._fy + z * view._fz;
        SInt16 sx =
              quarterSquareTable[x + view._rx._v]
            - quarterSquareTable[x - view._rx._v]
            + quarterSquareTable[y + view._ry._v]
            - quarterSquareTable[y - view._ry._v]
            + quarterSquareTable[z + view._rz._v]
            - quarterSquareTable[y - view._rz._v];
        SInt16 sy =
              quarterSquareTable[x + view._dx._v]
            - quarterSquareTable[x - view._dx._v]
            + quarterSquareTable[y + view._dy._v]
            - quarterSquareTable[y - view._dy._v]
            + quarterSquareTable[z + view._dz._v]
            - quarterSquareTable[y - view._dz._v];
        _sz =
              quarterSquareTable[x + view._fx._v]
            - quarterSquareTable[x - view._fx._v]
            + quarterSquareTable[y + view._fy._v]
            - quarterSquareTable[y - view._fy._v]
            + quarterSquareTable[z + view._fz._v]
            - quarterSquareTable[y - view._fz._v];
        static const int log0x0280 = logTable[0x0280];  // == 0x27c8
        static const int log0x3ffe = logTable[0x3ffe] - log0x0280;  // == 0x3bba - 0x27c8 == 0x13f2

        SInt16 logZ = logTable[_sz];
        _px = expTable[logTable[sx] - logZ];
        _py = expTable[logTable[sy] - logZ];
        if (_px == 0x4000 || _py == 0x4000) {
            // We had an overflow. This happens when _sz is small compared to
            // _sx and/or _sy, i.e. the point is off one of the sides of the
            // screen.

            static const int negate = 0x8000;
            if (sx >= 0) {
                if (sy >= 0) {
                    if (sx > sy) {   // 0 <= sy < sx
                        _px = 0x3ffe;
                        _py = expTable[log0x3ffe + logTable[sy] - logTable[sx]];
                    }
                    else {             // 0 <= sx <= sy
                        _py = 0x3ffe;
                        _px = expTable[log0x3ffe + logTable[sx] - logTable[sy]];
                    }
                }
                else { // sy < 0
                    sy = -sy;
                    if (sx > sy) {  // 0 < -sy < sx
                        _px = 0x3ffe;
                        _py = expTable[negate + log0x3ffe + logTable[sy] - logTable[sx]];
                    }
                    else {             // 0 < _sx <= -_sy
                        _py = -0x3ffe;
                        _px = expTable[log0x3ffe + logTable[sx] - logTable[sy]];
                    }
                }
            }
            else { // sx < 0
                sx = -sx;
                if (sy >= 0) {
                    if (sx > sy) {  // 0 <= sy < -sx
                        _px = -0x3ffe;
                        _py = expTable[log0x3ffe + logTable[sy] - logTable[sx]];
                    }
                    else {             // 0 < -sx <= sy
                        _py = 0x3ffe;
                        _px = expTable[negate + log0x3ffe + logTable[sx] - logTable[sy]];
                    }
                }
                else { // sy < 0
                    sy = -sy;
                    if (sx > sy) { // 0 < -sy < -sx
                        _px = -0x3ffe;
                        _py = expTable[negate + log0x3ffe + logTable[sy] - logTable[sx]];
                    }
                    else {             // 0 < -_sx <= -_sy
                        _py = -0x3ffe;
                        _px = expTable[negate + log0x3ffe + logTable[sx] - logTable[sy]];
                    }
                }
            }
            if (_sz < 0) {  // => interior/exterior code should treat Z=0 as positive
                _px = -_px;
                _py = -_py;
            }
        }
    }
};

Vertex* vertices;
int vertexCount;

static const int spanBits = 9;

struct Edge
{
    int _vertexIndexA;
    int _vertexIndexB;
    Vertex* _vertexA;
    Vertex* _vertexB;

    SInt16 _intercept;
    SInt16 _span;
    bool _horizontalish;
    bool _inverted;
    bool _degenerate;
    bool _processed;
    SInt16 _reversedIntercept;
    SInt16 _reversedSpan;

    Edge() : _degenerate(true) { }

    void init()
    {
        _vertexA = &vertices[_vertexIndexA];
        _vertexB = &vertices[_vertexIndexB];
    }

    // returns true if degenerate
    bool process()
    {
        if (_processed)
            return _degenerate;

        SInt16 ax = _vertexA->_px;
        SInt16 ay = _vertexA->_py;
        SInt16 bx = _vertexB->_px;
        SInt16 by = _vertexB->_py;
        SInt16 spanx = ax - bx;
        SInt16 spany = ay - by;

        // divide through by the largest of |spanx| and |spany|
        // no overflow can happen
        static const SInt16 spanLog = logTable[0x0200];
        SInt16 deltaLog;

        if (spanx < 0) {
            if (spany < 0) {
                if (-spanx > -spany) {
                    _horizontalish = true;
                    deltaLog = logTable[spany] - logTable[spanx];
                    _span = expTable[spanLog + deltaLog];
                    _intercept = by - expTable[logTable[bx] + deltaLog];
                    _inverted = true;
                }
                else {
                    _horizontalish = false;
                    deltaLog = logTable[spanx] - logTable[spany];
                    _span = expTable[spanLog + deltaLog];
                    _intercept = bx - expTable[logTable[by] + deltaLog];
                    _inverted = true;
                }
            }
            else { // spany >= 0
                if (-spanx > spany) {
                    _horizontalish = true;
                    deltaLog = logTable[spany] - logTable[spanx];
                    _span = expTable[spanLog + deltaLog];
                    _intercept = by - expTable[logTable[bx] + deltaLog];
                    _inverted = true;
                }
                else {
                    _horizontalish = false;
                    deltaLog = logTable[spanx] - logTable[spany];
                    _span = expTable[spanLog + deltaLog];
                    _intercept = bx - expTable[logTable[by] + deltaLog];
                    _inverted = false;
                }
            }
        }
        else { // spanx >= 0
            if (spany < 0) {
                if (spanx > -spany) {
                    _horizontalish = true;
                    deltaLog = logTable[spany] - logTable[spanx];
                    _span = expTable[spanLog + deltaLog];
                    _intercept = by - expTable[logTable[bx] + deltaLog];
                    _inverted = false;
                }
                else {
                    _horizontalish = false;
                    deltaLog = logTable[spanx] - logTable[spany];
                    _span = expTable[spanLog + deltaLog];
                    _intercept = bx - expTable[logTable[by] + deltaLog];
                    _inverted = true;
                }
            }
            else { // spany >= 0
                if (spanx > spany) {
                    _horizontalish = true;
                    deltaLog = logTable[spany] - logTable[spanx];
                    _span = expTable[spanLog + deltaLog];
                    _intercept = by - expTable[logTable[bx] + deltaLog];
                    _inverted = false;
                }
                else {
                    if (spany == 0) {  // Handle with interrupt 0
                        // We have a separate _degenerate flag for this case
                        // because both sides of the edge should be "outside"
                        // so that nothing is drawn.
                        _processed = true;
                        _degenerate = true;
                        return true;
                    }
                    else {
                        _horizontalish = false;
                        deltaLog = logTable[spanx] - logTable[spany];
                        _span = expTable[spanLog + deltaLog];
                        _intercept = bx - expTable[logTable[by] + deltaLog];
                        _inverted = false;
                    }
                }
            }
        }
        _processed = true;
        _degenerate = false;
        return false;
    }
};

Edge* edges;
int edgeCount;
Edge degenerateEdge;

struct Triangle
{
    int _vertexIndexA;
    int _vertexIndexB;
    int _vertexIndexC;
    int _edgeIndexAB;
    int _edgeIndexBC;
    int _edgeIndexCA;
    UInt16 _colour;
    Vertex* _vertexA;
    Vertex* _vertexB;
    Vertex* _vertexC;
    Edge* _edgeAB;
    Edge* _edgeBC;
    Edge* _edgeCA;
    bool _visible;
    SInt16 _z;

    void init()
    {
        _vertexA = &vertices[_vertexIndexA];
        _vertexB = &vertices[_vertexIndexB];
        _vertexC = &vertices[_vertexIndexC];
        _edgeAB = &edges[_edgeIndexAB];
        _edgeBC = &edges[_edgeIndexBC];
        _edgeCA = &edges[_edgeIndexCA];
    }

    void draw()
    {
        _visible = false;
        if (_vertexA->_sz <= 0) {
            if (_vertexB->_sz <= 0) {
                if (_vertexC->_sz <= 0) {
                    // Triangle is behind us - don't need to draw
                    return;
                }
                else {  //_vertexC->_sz > 0
                    if (_edgeBC->process())
                        return;
                    if (_edgeCA->process())
                        return;
                    renderTopLevel(_edgeBC, true, _edgeCA, true, &degenerateEdge, false, _colour);
                }
            }
            else {  //_vertexB->_sz > 0
                if (_vertexC->_sz <= 0) {
                    if (_edgeAB->process())
                        return;
                    if (_edgeBC->process())
                        return;
                    renderTopLevel(_edgeAB, true, _edgeBC, true, &degenerateEdge, false, _colour);
                }
                else {  //_vertexC->_sz > 0
                    if (_edgeAB->process())
                        return;
                    if (_edgeBC->process())
                        return;
                    if (_edgeCA->process())
                        return;
                    renderTopLevel(_edgeAB, false, _edgeBC, true, _edgeCA, false, _colour);
                }
            }
        }
        else {  // _vertexA->_sz > 0
            if (_vertexB->_sz <= 0) {
                if (_vertexC->_sz <= 0) {
                    if (_edgeAB->process())
                        return;
                    if (_edgeCA->process())
                        return;
                    renderTopLevel(_edgeAB, true, _edgeCA, true, &degenerateEdge, false, _colour);
                }
                else {  //_vertexC->_sz > 0
                    if (_edgeAB->process())
                        return;
                    if (_edgeBC->process())
                        return;
                    if (_edgeCA->process())
                        return;
                    renderTopLevel(_edgeAB, false, _edgeBC, false, _edgeCA, true, _colour);
                }
            }
            else {  //_vertexB->_sz > 0
                if (_vertexC->_sz <= 0) {
                    if (_edgeAB->process())
                        return;
                    if (_edgeBC->process())
                        return;
                    if (_edgeCA->process())
                        return;
                    renderTopLevel(_edgeAB, true, _edgeBC, false, _edgeCA, false, _colour);
                }
                else {  //_vertexC->_sz > 0
                    if (_edgeAB->process())
                        return;
                    if (_edgeBC->process())
                        return;
                    if (_edgeCA->process())
                        return;
                    renderTopLevel(_edgeAB, false, _edgeBC, false, _edgeCA, false, _colour);
                }
            }
        }
        _visible = true;
        _z = _vertexA->_z + _vertexB->_z + _vertexC->_z;
    }
};

struct Node
{
    Node* tl;
    Node* tr;
    Node* bl;
    Node* br;
    UInt16 colourT;
    UInt16 colourB;
    bool subdivided;
};

Node* lastPool;
Node* currentPool;
Node* allNodes;
int totalNodeCount;
int nodesInPool;
int nodesUsed;
bool firstPool;

void switchNodePool()
{
    lastPool = currentPool;
    if (firstPool)
        currentPool = allNodes + nodesInPool;
    else
        currentPool = allNodes;
    firstPool = !firstPool;
    nodesUsed = 1;
    currentPool[0].subdivided = false;
    currentPool[0].colourT = 0;
    currentPool[0].colourB = 0;
}

// level == 9 for 512x512 ldots
// level == 1 for 2x2 ldots
void render(int level, Node* node,
    SInt16 interceptAB, SInt16 interceptBC, SInt16 interceptCA,
    SInt16 spanAB, SInt16 spanBC, SInt16 spanCA,
    bool activeAB, bool activeBC, bool activeCA,
    bool invertedAB, bool invertedBC, bool invertedCA,
    bool horizontalishAB, bool horizontalishBC, bool horizontalishCA,
    int x, int y,
    bool insideABTL, bool insideABTR, bool insideABBL, bool insideABBR,
    bool insideBCTL, bool insideBCTR, bool insideBCBL, bool insideBCBR,
    bool insideCATL, bool insideCATR, bool insideCABL, bool insideCABR,
    UInt16 colour, bool highres)
{
    if (highres) {
        if (level == 1) {
            if ((y & 1) == 0) {
                if ((x & 1) == 0) {
                    if ((!activeAB || insideABTL) &&
                        (!activeBC || insideBCTL) &&
                        (!activeCA || insideCATL)) {
                        node->colourT = (node->colourT & 0xff3f) | (colour & 0x00c0);
                    }
                    if ((!activeAB || insideABTR) &&
                        (!activeBC || insideBCTR) &&
                        (!activeCA || insideCATR)) {
                        node->colourT = (node->colourT & 0xffcf) | (colour & 0x0030);
                    }
                    if ((!activeAB || insideABBL) &&
                        (!activeBC || insideBCBL) &&
                        (!activeCA || insideCABL)) {
                        node->colourT = (node->colourT & 0x3fff) | (colour & 0xc000);
                    }
                    if ((!activeAB || insideABBR) &&
                        (!activeBC || insideBCBR) &&
                        (!activeCA || insideCABR)) {
                        node->colourT = (node->colourT & 0xcfff) | (colour & 0x3000);
                    }
                    return;
                }
                else {  // (x & 1) == 1
                    if ((!activeAB || insideABTL) &&
                        (!activeBC || insideBCTL) &&
                        (!activeCA || insideCATL)) {
                        node->colourT = (node->colourT & 0xfff3) | (colour & 0x000c);
                    }
                    if ((!activeAB || insideABTR) &&
                        (!activeBC || insideBCTR) &&
                        (!activeCA || insideCATR)) {
                        node->colourT = (node->colourT & 0xfffc) | (colour & 0x0003);
                    }
                    if ((!activeAB || insideABBL) &&
                        (!activeBC || insideBCBL) &&
                        (!activeCA || insideCABL)) {
                        node->colourT = (node->colourT & 0xf3ff) | (colour & 0x0c00);
                    }
                    if ((!activeAB || insideABBR) &&
                        (!activeBC || insideBCBR) &&
                        (!activeCA || insideCABR)) {
                        node->colourT = (node->colourT & 0xfcff) | (colour & 0x0300);
                    }
                    return;
                }
            }
            else {  // (y & 1) == 1
                if ((x & 1) == 0) {
                    if ((!activeAB || insideABTL) &&
                        (!activeBC || insideBCTL) &&
                        (!activeCA || insideCATL)) {
                        node->colourB = (node->colourB & 0xff3f) | (colour & 0x00c0);
                    }
                    if ((!activeAB || insideABTR) &&
                        (!activeBC || insideBCTR) &&
                        (!activeCA || insideCATR)) {
                        node->colourB = (node->colourB & 0xffcf) | (colour & 0x0030);
                    }
                    if ((!activeAB || insideABBL) &&
                        (!activeBC || insideBCBL) &&
                        (!activeCA || insideCABL)) {
                        node->colourB = (node->colourB & 0x3fff) | (colour & 0xc000);
                    }
                    if ((!activeAB || insideABBR) &&
                        (!activeBC || insideBCBR) &&
                        (!activeCA || insideCABR)) {
                        node->colourB = (node->colourB & 0xcfff) | (colour & 0x3000);
                    }
                    return;
                }
                else {  // (x & 1) == 1
                    if ((!activeAB || insideABTL) &&
                        (!activeBC || insideBCTL) &&
                        (!activeCA || insideCATL)) {
                        node->colourB = (node->colourB & 0xfff3) | (colour & 0x000c);
                    }
                    if ((!activeAB || insideABTR) &&
                        (!activeBC || insideBCTR) &&
                        (!activeCA || insideCATR)) {
                        node->colourB = (node->colourB & 0xfffc) | (colour & 0x0003);
                    }
                    if ((!activeAB || insideABBL) &&
                        (!activeBC || insideBCBL) &&
                        (!activeCA || insideCABL)) {
                        node->colourB = (node->colourB & 0xf3ff) | (colour & 0x0c00);
                    }
                    if ((!activeAB || insideABBR) &&
                        (!activeBC || insideBCBR) &&
                        (!activeCA || insideCABR)) {
                        node->colourB = (node->colourB & 0xfcff) | (colour & 0x0300);
                    }
                    return;
                }
            }
        }
    }
    else {
        if (level == 2) {
            if ((!activeAB || insideABTL) &&
                (!activeBC || insideBCTL) &&
                (!activeCA || insideCATL)) {
                node->colourT = (node->colourT & 0xff0f) | (colour & 0x00f0);
            }
            if ((!activeAB || insideABTR) &&
                (!activeBC || insideBCTR) &&
                (!activeCA || insideCATR)) {
                node->colourT = (node->colourT & 0xfff0) | (colour & 0x000f);
            }
            if ((!activeAB || insideABBL) &&
                (!activeBC || insideBCBL) &&
                (!activeCA || insideCABL)) {
                node->colourT = (node->colourT & 0x0fff) | (colour & 0xf000);
            }
            if ((!activeAB || insideABBR) &&
                (!activeBC || insideBCBR) &&
                (!activeCA || insideCABR)) {
                node->colourT = (node->colourT & 0xf0ff) | (colour & 0x0f00);
            }
            return;
        }
    }
    if (insideABTL && insideABTR && insideABBL && insideABBR)
        activeAB = false;
    if (insideBCTL && insideBCTR && insideBCBL && insideBCBR)
        activeBC = false;
    if (insideCATL && insideCATR && insideCABL && insideCABR)
        activeCA = false;
    if (!activeAB && !activeBC && !activeCA) {
        // Node entirely inside triangle
        node->subdivided = false;
        node->colourT = colour;
        if (highres)
            node->colourB = colour;
        return;
    }
    if ((activeAB && !insideABTL && !insideABTR && !insideABBL && !insideABBR) ||
        (activeBC && !insideBCTL && !insideBCTR && !insideBCBL && !insideBCBR) ||
        (activeCA && !insideCATL && !insideCATR && !insideCABL && !insideCABR)) {
        // Node entirely outside triangle
        return;
    }
    if (!node->subdivided) {
        node->subdivided = true;
        Node* newNode = &currentPool[nodesUsed];
        nodesUsed += 4;
        if (nodesUsed >= nodesInPool)
            throw Exception("Out of nodes");
        node->tl = newNode;
        node->tr = newNode + 1;
        node->bl = newNode + 2;
        node->br = newNode + 3;
        newNode[0].colourT = node->colourT;
        newNode[1].colourT = node->colourT;
        newNode[2].colourT = node->colourT;
        newNode[3].colourT = node->colourT;
        newNode[0].subdivided = false;
        newNode[1].subdivided = false;
        newNode[2].subdivided = false;
        newNode[3].subdivided = false;
    }
    bool insideABTM, insideABML, insideABMM, insideABMR, insideABBM;
    bool insideBCTM, insideBCML, insideBCMM, insideBCMR, insideBCBM;
    bool insideCATM, insideCAML, insideCAMM, insideCAMR, insideCABM;
    // Initial intercept is at TL point
    // Compute intercept at TM point
    SInt16 interceptABTM, interceptABML, interceptABMM, interceptABMR, interceptABBM;
    SInt16 interceptBCTM, interceptBCML, interceptBCMM, interceptBCMR, interceptBCBM;
    SInt16 interceptCATM, interceptCAML, interceptCAMM, interceptCAMR, interceptCABM;
    SInt16 spanABH = spanAB >> 1;
    SInt16 spanBCH = spanBC >> 1;
    SInt16 spanCAH = spanCA >> 1;
    SInt16 spanABR = spanABH;
    SInt16 spanABB = spanABH;
    SInt16 spanBCR = spanBCH;
    SInt16 spanBCB = spanBCH;
    SInt16 spanCAR = spanCAH;
    SInt16 spanCAB = spanCAH;
    SInt16 half = (1 << (level + spanBits - 10));
    SInt16 full = (1 << (level + spanBits - 9));
    if (activeAB) {
        if (horizontalishAB) {
            interceptABTM = interceptAB + spanABH;
            insideABTM = ((interceptABTM > 0) != invertedAB);
            interceptABML = interceptAB + half;
            insideABML = ((interceptABML > 0) != invertedAB);
            interceptABMM = interceptABTM + half;
            insideABMM = ((interceptABMM > 0) != invertedAB);
            interceptABMR = interceptABML + spanAB;
            insideABMR = ((interceptABMR > 0) != invertedAB);
            interceptABBM = interceptABMM + half;
            insideABBM = ((interceptABBM > 0) != invertedAB);
            spanABR = spanAB - spanABR;
        }
        else {
            interceptABTM = interceptAB + half;
            insideABTM = ((interceptABTM > 0) != invertedAB);
            interceptABML = interceptAB + spanABH;
            insideABML = ((interceptABML > 0) != invertedAB);
            interceptABMM = interceptABML + half;
            insideABMM = ((interceptABMM > 0) != invertedAB);
            interceptABMR = interceptABML + full;
            insideABMR = ((interceptABMR > 0) != invertedAB);
            interceptABBM = interceptABTM + spanAB;
            insideABBM = ((interceptABBM > 0) != invertedAB);
            spanABB = spanAB - spanABB;
        }
    }
    if (activeBC) {
        if (horizontalishBC) {
            interceptBCTM = interceptBC + spanBCH;
            insideBCTM = ((interceptBCTM > 0) != invertedBC);
            interceptBCML = interceptBC + half;
            insideBCML = ((interceptBCML > 0) != invertedBC);
            interceptBCMM = interceptBCTM + half;
            insideBCMM = ((interceptBCMM > 0) != invertedBC);
            interceptBCMR = interceptBCML + spanBC;
            insideBCMR = ((interceptBCMR > 0) != invertedBC);
            interceptBCBM = interceptBCMM + half;
            insideBCBM = ((interceptBCBM > 0) != invertedBC);
            spanBCR = spanBC - spanBCR;
        }
        else {
            interceptBCTM = interceptBC + half;
            insideBCTM = ((interceptBCTM > 0) != invertedBC);
            interceptBCML = interceptBC + spanBCH;
            insideBCML = ((interceptBCML > 0) != invertedBC);
            interceptBCMM = interceptBCML + half;
            insideBCMM = ((interceptBCMM > 0) != invertedBC);
            interceptBCMR = interceptBCML + full;
            insideBCMR = ((interceptBCMR > 0) != invertedBC);
            interceptBCBM = interceptBCTM + spanBC;
            insideBCBM = ((interceptBCBM > 0) != invertedBC);
            spanBCB = spanBC - spanBCB;
        }
    }
    if (activeCA) {
        if (horizontalishCA) {
            interceptCATM = interceptCA + spanCAH;
            insideCATM = ((interceptCATM > 0) != invertedCA);
            interceptCAML = interceptCA + half;
            insideCAML = ((interceptCAML > 0) != invertedCA);
            interceptCAMM = interceptCATM + half;
            insideCAMM = ((interceptCAMM > 0) != invertedCA);
            interceptCAMR = interceptCAML + spanCA;
            insideCAMR = ((interceptCAMR > 0) != invertedCA);
            interceptCABM = interceptCAMM + half;
            insideCABM = ((interceptCABM > 0) != invertedCA);
            spanCAR = spanCA - spanCAR;
        }
        else {
            interceptCATM = interceptCA + half;
            insideCATM = ((interceptCATM > 0) != invertedCA);
            interceptCAML = interceptCA + spanCAH;
            insideCAML = ((interceptCAML > 0) != invertedCA);
            interceptCAMM = interceptCAML + half;
            insideCAMM = ((interceptCAMM > 0) != invertedCA);
            interceptCAMR = interceptCAML + full;
            insideCAMR = ((interceptCAMR > 0) != invertedCA);
            interceptCABM = interceptCATM + spanCA;
            insideCABM = ((interceptCABM > 0) != invertedCA);
            spanCAB = spanCA - spanCAB;
        }
    }
    // TODO: Replace some span >> 1 with span - (span >> 1) depending on horizontalish
    render(level - 1, node->tl,
        interceptAB, interceptBC, interceptCA,
        spanABH, spanBCH, spanCAH,
        activeAB, activeBC, activeCA,
        invertedAB, invertedBC, invertedCA,
        horizontalishAB, horizontalishBC, horizontalishCA,
        x, y,
        insideABTL, insideABTM, insideABML, insideABMM,
        insideBCTL, insideBCTM, insideBCML, insideBCMM,
        insideCATL, insideCATM, insideCAML, insideCAMM,
        colour, highres);
    SInt16 pixels = 1 << (level - 1);
    if (x + pixels < 320) {
        render(level - 1, node->tr,
            interceptABTM, interceptBCTM, interceptCATM,
            spanABR, spanBCR, spanCAR,
            activeAB, activeBC, activeCA,
            invertedAB, invertedBC, invertedCA,
            horizontalishAB, horizontalishBC, horizontalishCA,
            x + pixels, y,
            insideABTM, insideABTR, insideABMM, insideABMR,
            insideBCTM, insideBCTR, insideBCMM, insideBCMR,
            insideCATM, insideCATR, insideCAMM, insideCAMR,
            colour, highres);
    }
    if (y + pixels < 200) {
        render(level - 1, node->bl,
            interceptABML, interceptBCML, interceptCAML,
            spanABB, spanBCB, spanCAB,
            activeAB, activeBC, activeCA,
            invertedAB, invertedBC, invertedCA,
            horizontalishAB, horizontalishBC, horizontalishCA,
            x, y + pixels,
            insideABML, insideABMM, insideABBL, insideABBM,
            insideBCML, insideBCMM, insideBCBL, insideBCBM,
            insideCAML, insideCAMM, insideCABL, insideCABM,
            colour, highres);
        if (x + pixels < 320) {
            render(level - 1, node->br,
                interceptABMM, interceptBCMM, interceptCAMM,
                spanAB - spanABH, spanBC - spanBCH, spanCA - spanCAH,
                activeAB, activeBC, activeCA,
                invertedAB, invertedBC, invertedCA,
                horizontalishAB, horizontalishBC, horizontalishCA,
                x + pixels, y + pixels,
                insideABMM, insideABMR, insideABBM, insideABBR,
                insideBCMM, insideBCMR, insideBCBM, insideBCBR,
                insideCAMM, insideCAMR, insideCABM, insideCABR,
                colour, highres);
        }
    }
}

void renderTopLevel(Edge* edgeAB, bool invertedAB, Edge* edgeBC, bool invertedBC, Edge* edgeCA, bool invertedCA, UInt16 colour)
{
    bool activeCA = (edgeCA != 0);
    SInt16 interceptAB = edgeAB->_intercept;
    SInt16 interceptBC = edgeBC->_intercept;
    SInt16 interceptCA = edgeCA->_intercept;
    SInt16 spanAB = edgeAB->_span;
    SInt16 spanBC = edgeBC->_span;
    SInt16 spanCA = edgeCA->_span;
    bool horizontalishAB = edgeAB->_horizontalish;
    bool horizontalishBC = edgeBC->_horizontalish;
    bool horizontalishCA = edgeCA->_horizontalish;
    invertedAB = (invertedAB != edgeAB->_inverted);
    invertedBC = (invertedBC != edgeBC->_inverted);
    invertedCA = (invertedCA != edgeCA->_inverted);
    bool insideABTL;
    bool insideABTR;
    bool insideABBL;
    bool insideABBR;
    bool insideBCTL;
    bool insideBCTR;
    bool insideBCBL;
    bool insideBCBR;
    bool insideCATL;
    bool insideCATR;
    bool insideCABL;
    bool insideCABR;
    SInt16 interceptABTR;
    SInt16 interceptABBL;
    SInt16 interceptABBR;
    SInt16 interceptBCTR;
    SInt16 interceptBCBL;
    SInt16 interceptBCBR;
    SInt16 interceptCATR;
    SInt16 interceptCABL;
    SInt16 interceptCABR;
    SInt16 full = 1 << spanBits;

    if (horizontalishAB) {
        insideABTL = ((interceptAB > 0) != invertedAB);
        interceptABTR = interceptAB + spanAB;
        insideABTR = ((interceptABTR > 0) != invertedAB);
        interceptABBR = interceptABTR + full;
        insideABBR = ((interceptABBR > 0) != invertedAB);
        interceptABBL = interceptABBR - spanAB;
        insideABBL = ((interceptABBL > 0) != invertedAB);
    }
    else {
        insideABTL = ((interceptAB > 0) != invertedAB);
        interceptABTR = interceptAB + full;
        insideABTR = ((interceptABTR > 0) != invertedAB);
        interceptABBR = interceptABTR + spanAB;
        insideABBR = ((interceptABBR > 0) != invertedAB);
        interceptABBL = interceptABBR - full;
        insideABBL = ((interceptABBL > 0) != invertedAB);
    }
    if (horizontalishBC) {
        insideBCTL = ((interceptBC > 0) != invertedBC);
        interceptBCTR = interceptBC + spanBC;
        insideBCTR = ((interceptBCTR > 0) != invertedBC);
        interceptBCBR = interceptBCTR + full;
        insideBCBR = ((interceptBCBR > 0) != invertedBC);
        interceptBCBL = interceptBCBR - spanBC;
        insideBCBL = ((interceptBCBL > 0) != invertedBC);
    }
    else {
        insideBCTL = ((interceptBC > 0) != invertedBC);
        interceptBCTR = interceptBC + full;
        insideBCTR = ((interceptBCTR > 0) != invertedBC);
        interceptBCBR = interceptBCTR + spanBC;
        insideBCBR = ((interceptBCBR > 0) != invertedBC);
        interceptBCBL = interceptBCBR - full;
        insideBCBL = ((interceptBCBL > 0) != invertedBC);
    }
    if (activeCA) {
        if (horizontalishCA) {
            insideCATL = ((interceptCA > 0) != invertedCA);
            interceptCATR = interceptCA + spanCA;
            insideCATR = ((interceptCATR > 0) != invertedCA);
            interceptCABR = interceptCATR + full;
            insideCABR = ((interceptCABR > 0) != invertedCA);
            interceptCABL = interceptCABR - spanCA;
            insideCABL = ((interceptCABL > 0) != invertedCA);
        }
        else {
            insideCATL = ((interceptCA > 0) != invertedCA);
            interceptCATR = interceptCA + full;
            insideCATR = ((interceptCATR > 0) != invertedCA);
            interceptCABR = interceptCATR + spanCA;
            insideCABR = ((interceptCABR > 0) != invertedCA);
            interceptCABL = interceptCABR - full;
            insideCABL = ((interceptCABL > 0) != invertedCA);
        }
    }

    render(9, currentPool, interceptAB, interceptBC, interceptCA,
        spanAB, spanBC, spanCA,
        true, true, activeCA,
        invertedAB, invertedBC, invertedCA,
        horizontalishAB, horizontalishBC, horizontalishCA,
        0, 0,
        insideABTL, insideABTR, insideABBL, insideABBR,
        insideBCTL, insideBCTR, insideBCBL, insideBCBR,
        insideCATL, insideCATR, insideCABL, insideCABR,
        colour, true /* highres */);
}

Triangle* triangles;
int triangleCount;
Triangle** sortedTriangles;
int sortedTriangleCount;
Triangle** sortBuffer;
int nextFreeSortPosition;

typedef Vector3<float> InitVertex;

struct InitTriangle
{
    InitTriangle(int colour, std::initializer_list<int> vertices)
    {
        _colour = colour;
        _v1 = vertices.begin()[0];
        _v2 = vertices.begin()[1];
        _v3 = vertices.begin()[2];
    }
    int _colour;
    int _v1, _v2, _v3;
};

struct Shape
{
    Shape(std::initializer_list<InitVertex> initVertices, float scale,
        std::initializer_list<InitTriangle> initTriangles)
    {
        _vertices.allocate(initVertices.size());
        _triangles.allocate(initTriangles.size());
        triangles = &_triangles[0];
        triangleCount = _triangles.count();
        vertices = &_vertices[0];
        vertexCount = _vertices.count();
        for (int i = 0; i < vertexCount; ++i) {
            vertices[i].init(
        }

        _nVertices = vertices.size();
        _vertices.allocate(_nVertices);
        _vertex0 = &_vertices[0];
        float distance = (256.0 / 200.0) * (5.0 / 6.0);
        scale = distance / (scale * sqrt(1 + distance * distance));
        for (int i = 0; i < _nVertices; ++i) {
            _vertex0[i] = vertices.begin()[i] * scale;
            _vertex0[i].x = Fix8p8::fromRepresentation(adjust(_vertex0[i].x.representation()));
            _vertex0[i].y = Fix8p8::fromRepresentation(adjust(_vertex0[i].y.representation()));
            _vertex0[i].z = Fix8p8::fromRepresentation(adjust(_vertex0[i].z.representation()));
        }

        _nFaces = faces.size();
        _faces.allocate(_nFaces);
        _face0 = &_faces[0];
        for (int i = 0; i < _nFaces; ++i)
            _face0[i] = faces.begin()[i];


    }

    void process()
    {
        for (int i = 0; i < vertexCount; ++i)
            vertices[i].transform();
        for (int i = 0; i < edgeCount; ++i)
            edges[i]._processed = false;
        

        sortedTriangles = naturalMergeSort(&triangles, triangleCount);
        // TODO: depth sort triangles, furthest away first.
        for (int i = 0; i < triangleCount; ++i)
            triangles[i].draw();
    }
    Triangle** merge(Triangle** left, int countLeft, Triangle** right, int countRight)
    {
        if (countLeft == 0)
            return right;
        if (countRight == 0)
            return left;
        Triangle** destination = &sortBuffer[nextFreeSortPosition];
        while (countLeft > 0 && countRight > 0) {
            if (left[0]->_z <= right[0]->_z) {
                sortBuffer[nextFreeSortPosition] = *left;
                ++left;
                --countLeft;
            }
            else {
                sortBuffer[nextFreeSortPosition] = *right;
                ++right;
                --countRight;
            }
            ++nextFreeSortPosition;
        }
        if (countLeft == 0) {
            while (countRight > 0) {
                sortBuffer[nextFreeSortPosition] = *right;
                ++right;
                --countRight;
                ++nextFreeSortPosition;
            }
        }
        else {
            while (countLeft > 0) {
                sortBuffer[nextFreeSortPosition] = *left;
                ++left;
                --countLeft;
                ++nextFreeSortPosition;
            }
        }
        return destination;
    }
    int leftRun(Triangle** array, int count)
    {
        for (int i = 0; i < count - 1; ++i) {
            if (array[i + 1]->_z < array[i]->_z) {
                return i + 1;
            }
        }
        return 1;
    }
    Triangle** naturalMergeSort(Triangle** array, int count)
    {
        if (count < 2)
            return array;
        return naturalMergeSortLeftRunKnown(array, count, leftRun(array, count));
    }
    Triangle** naturalMergeSortLeftRunKnown(Triangle** array, int count, int leftRunLength)
    {
        if (leftRunLength * 2 >= count) {
            int remaining = count - leftRunLength;
            Triangle** rightArray = naturalMergeSort(array + leftRunLength, remaining);
            return merge(array, leftRunLength, rightArray, remaining);
        }
        // Find run at end
        int rightRunLength = 1;
        for (int i = count - 1; i >= leftRunLength; --i) {
            if (array[i + 1]->_z < array[i]->_z) {
                rightRunLength = count - i;
                break;
            }
        }
        return naturalMergeSortBothRunsKnown(array, count, leftRunLength, rightRunLength);
    }
    Triangle** naturalMergeSortRightRunKnown(Triangle** array, int count, int rightRunLength)
    {
        if (rightRunLength * 2 >= count) {
            int remaining = count - rightRunLength;
            Triangle** leftArray = naturalMergeSort(array, remaining);
            return merge(leftArray, remaining, array + count - rightRunLength, rightRunLength);
        }
        // Find run at start
        int leftRunLength = leftRun(array, count);
        if (leftRunLength * 2 >= count) {
            int remaining = count - leftRunLength;
            Triangle** rightArray = naturalMergeSort(array + leftRunLength, remaining);
            return merge(array, leftRunLength, rightArray, remaining);
        }
        return naturalMergeSortBothRunsKnown(array, count, leftRunLength, rightRunLength);
    }
    Triangle** naturalMergeSortBothRunsKnown(Triangle** array, int count, int leftRunLength, int rightRunLength)
    {
        if (rightRunLength * 2 >= count) {
            int remaining = count - rightRunLength;
            Triangle** leftArray = naturalMergeSortLeftRunKnown(array, remaining, leftRunLength);
            return merge(leftArray, remaining, array + count - rightRunLength, rightRunLength);
        }
        return naturalMergeSortMiddle(array, count, leftRunLength, rightRunLength);
    }
    Triangle** naturalMergeSortMiddle(Triangle** array, int count, int leftRunLength, int rightRunLength)
    {
        // Find run in middle
        int middle = count / 2;
        int middleRightRunLength = 1;
        // Find right extent of middle run
        for (int i = middle; i < count - (rightRunLength + 1); ++i) {
            if (array[i + 1]->_z < array[i]->_z) {
                middleRightRunLength = i + 1 - middle;
                break;
            }
        }
        // Find left extent of middle run
        int middleLeftRunLength = 0;
        for (int i = middle - 1; i >= leftRunLength; --i) {
            if (array[i + 1]->_z < array[i]->_z) {
                middleLeftRunLength = count - i;
                break;
            }
        }
        int middleRunLength = middleLeftRunLength + middleRightRunLength;
        if (middleLeftRunLength > middleRightRunLength) {
            // Middle moves to right edge of middle run
            int leftCount = middle + middleRightRunLength;
            Triangle** leftArray = naturalMergeSortBothRunsKnown(array, leftCount, leftRunLength, middleRunLength);
            int rightCount = count - leftCount;
            Triangle** rightArray = naturalMergeSortRightRunKnown(array + leftCount, rightCount, rightRunLength);
            return merge(leftArray, leftCount, rightArray, rightCount);
        }
        // Middle moves to left edge of middle run
        int leftCount = middle - middleLeftRunLength;
        Triangle** leftArray = naturalMergeSortLeftRunKnown(array, leftCount, leftRunLength);
        int rightCount = count - leftCount;
        Triangle** rightArray = naturalMergeSortBothRunsKnown(array + leftCount, rightCount, middleRunLength, rightRunLength);
        return merge(leftArray, leftCount, rightArray, rightCount);
    }

    Array<Vertex> _vertices;
    Array<Edge> _edges;
    Array<Triangle> _triangles;
};

static const float phi = (sqrt(5.0f) + 1) / 2;

Shape shape
{ {{  phi,     1,     0},  // Icosahedron
  { -phi,     1,     0},
  {  phi,    -1,     0},
  { -phi,    -1,     0},
  {    1,     0,   phi},
  {    1,     0,  -phi},
  {   -1,     0,   phi},
  {   -1,     0,  -phi},
  {    0,   phi,     1},
  {    0,  -phi,     1},
  {    0,   phi,    -1},
  {    0,  -phi,    -1}},
  8192/phi,
 {{1, { 4,  8,  0}},
  {2, {10,  5,  0}},
  {3, { 9,  4,  2}},
  {4, { 5, 11,  2}},
  {5, { 8,  6,  1}},
  {6, { 7, 10,  1}},
  {7, { 6,  9,  3}},
  {6, {11,  7,  3}},
  {9, { 8, 10,  0}},
  {10, {10,  8,  1}},
  {11, {11,  9,  2}},
  {12, { 9, 11,  3}},
  {13, { 0,  2,  4}},
  {14, { 2,  0,  5}},
  {15, { 3,  1,  6}},
  {1, { 1,  3,  7}},
  {2, { 4,  6,  8}},
  {3, { 6,  4,  9}},
  {4, { 7,  5, 10}},
  {5, { 5,  7, 11}}} };


class Fix8p8
{
public:
    Fix8p8() { }
    Fix8p8(double v) : _v(static_cast<SInt16>(round(v * 256.0))) { }
    //Fix8p8(SInt8 v) : _v(v << 8) { }
    Fix8p8 operator*(Fix8p8& other)
    {
        Fix8p8 r;
        r._v = (*this)*other._v;
        return r;
    }
    SInt16 operator*(SInt16 other) const
    {
        return static_cast<SInt16>(
            ((static_cast<SInt32>(_v)) * (static_cast<SInt32>(other))) >> 8);
    }
    Fix8p8& operator+=(const Fix8p8& other)
    {
        _v += other._v;
        return *this;
    }
    Fix8p8 operator+(const Fix8p8& other) const
    {
        Fix8p8 r = *this;
        r += other;
        return r;
    }
    Fix8p8& operator-=(const Fix8p8& other)
    {
        _v -= other._v;
        return *this;
    }
    Fix8p8 operator-(const Fix8p8& other) const
    {
        Fix8p8 r = *this;
        r -= other;
        return r;
    }
    Fix8p8 operator-() const
    {
        Fix8p8 r;
        r._v = -_v;
        return r;
    }
    Fix8p8 operator~() const
    {
        Fix8p8 r;
        r._v = ~_v;
        return r;
    }
    Fix8p8 operator&=(const Fix8p8& other)
    {
        _v &= other._v;
        return *this;
    }
    Fix8p8 operator&(const Fix8p8 other) const
    {
        Fix8p8 r = *this;
        r &= other;
        return r;
    }
    SInt16 _v;
};

template<class T> class SinCosTable
{
public:
    SinCosTable(int tau, T nmask = 0, T scale = 1)
    {
        _tau4 = tau/4;
        int tau54 = tau + _tau4;
        _entries.allocate(tau54);
        for (int i = 0; i < tau54; ++i)
            _entries[i] = static_cast<T>(sin(i*M_PI*2/tau)*scale) & ~nmask;
    }
    T sin(int a) { return _entries[a]; }
    T cos(int a) { return _entires[a + _tau4]; }
private:
    Array<T> _entries;
    int _tau4;
};

SinCosTable<Fix8p8> sinCosTable(0x400, 1/256.0);

template<class T> class ScaleTable
{
public:
    ScaleTable(int first, int last, double scale, T nmask = 0) : _first(first)
    {
        _entries.allocate(1 + last - first);
        for (int i = first; i <= last; ++i)
            _entries[i-first] = static_cast<T>(i * scale) & ~nmask;
    }
    T operator[](int a)
    {
        if ((a & 1) != 0)
            throw Exception("Misaligned access");
        if (a < _first || a >= _entries.count() + _first)
            throw Exception("Table access out of range");
        return _entries[a-_first];
    }
private:
    Array<T> _entries;
    int _first;
    int _last;
};

//ScaleTable<SInt16> scaleTableX(-0x100, 0x100, 1);  // TODO: figure out scale
//ScaleTable<SInt16> scaleTableY(-0x100, 0x100, 1);  // TODO: figure out scale
//ScaleTable<SInt16> scaleTableZ(-0x100, 0x100, 1);  // TODO: figure out scale

template<class T> class NormTable
{
public:
    NormTable(int first, int last) : _first(first)
    {
        _entries.allocate(1 + last - first);
        for (int i = first; i <= last; ++i) {
            _entries[i-first] =
                new ScaleTable(-0x100, 0x100, 256.0/sqrt(i/256.0), 1/256.0);
        }
    }
    ScaleTable<T>* operator[](int a)
    {
        if (a < _first || a >= _entries.count() + _first)
            throw Exception("Table access out of range");
        return _entries[a - _first];
    }
private:
    Array<ScaleTable<T>*> _entries;
    int _first;
};

NormTable<Fix8p8> normTable(0xf0, 0x110);

MotionMatrix motionMatrix;

class Rotor3D
{
public:
    Rotor3D() : _s(1), _xy(0), _yz(0), _zx(0) { }
    //Rotor3D operator*(const Rotor3D& r)
    //{
    //  return Rotor3D(
    //      _s*r._s  - _xy*r._xy - _yz*r._yz - _zx*r._zx,
    //      _s*r._xy + _xy*r._s  - _yz*r._zx + _zx*r._yz,
    //      _s*r._yz + _xy*r._zx + _yz*r._s  - _zx*r._xy,
    //      _s*r._zx - _xy*r._yz + _yz*r._xy + _zx*r._s);
    //}
    //const Rotor3D& operator*=(const Rotor3D& other)
    //{
    //  *this = (*this) * other;
    //}
    Rotor3D rotate(SInt16 x, SInt16 y)
    {
        // Transform rotor according to mouse movement


        //static const float scale = 0.01;
        //*this *= Rotor3D(cos(x * scale), 0, 0, sin(x * scale));
        //*this *= Rotor3D(cos(y * scale), 0, sin(y * scale), 0);

        Fix8p8 xc = sinCosTable.cos(x);
        Fix8p8 yc = sinCosTable.cos(y);
        Fix8p8 xs = sinCosTable.sin(x);
        Fix8p8 ys = sinCosTable.sin(y);

        //Fix8p8 xcyc = xc*yc;
        //Fix8p8 xsyc = xs*yc;
        //Fix8p8 xsys = xs*ys;
        //Fix8p8 xcys = xc*ys;

        //_s = _s * xcyc - _zx * xsyc - _xy * xsys - _yz * xcys;
        //_xy = _xy * xcyc - _yz * xsyc + _s * xsys + _zx * xcys;
        //_yz = _s * xcys - _zx * xsys + _xy * xsyc + _yz * xcyc;
        //_zx = -_xy * xcys + _yz * xsys + _s * xsyc + _zx * xcyc;


        //Fix8p8 zx = _zx * xc +  _s * xs;
        //Fix8p8 s  =  _s * xc - _zx * xs;
        //Fix8p8 yz = _yz * xc + _xy * xs;
        //Fix8p8 xy = _xy * xc - _yz * xs;

        //_s  =  s * yc - yz * ys;
        //_xy = xy * yc + zx * ys;
        //_yz = yz * yc +  s * ys;
        //_zx = zx * yc - xy * ys;

        Fix8p8 zx =
            quarterSquareTable[(xc + _zx)._v]
          - quarterSquareTable[(xc - _zx)._v]
          + quarterSquareTable[(xs +  _s)._v]
          - quarterSquareTable[(xs -  _s)._v];
        Fix8p8 s  =
            quarterSquareTable[(xs + _zx)._v]
          - quarterSquareTable[(xs - _zx)._v]
          + quarterSquareTable[(xc +  _s)._v]
          - quarterSquareTable[(xc -  _s)._v];
        Fix8p8 yz =
            quarterSquareTable[(xs + _xy)._v]
          - quarterSquareTable[(xs - _xy)._v]
          + quarterSquareTable[(xc + _yz)._v]
          - quarterSquareTable[(xc - _yz)._v];
        Fix8p8 xy =
            quarterSquareTable[(xs - _yz)._v]
          - quarterSquareTable[(xs + _yz)._v]
          - quarterSquareTable[(xc - _xy)._v]
          + quarterSquareTable[(xc + _xy)._v];

        _xy =
            quarterSquareTable[(yc +  xy)._v]
          - quarterSquareTable[(yc -  xy)._v]
          + quarterSquareTable[(ys +  zx)._v]
          - quarterSquareTable[(ys -  zx)._v];
        _zx =
            quarterSquareTable[(ys -  xy)._v]
          - quarterSquareTable[(ys +  xy)._v]
          - quarterSquareTable[(yc -  zx)._v]
          + quarterSquareTable[(yc +  zx)._v];
        _yz =
            quarterSquareTable[(yc +  yz)._v]
          - quarterSquareTable[(yc -  yz)._v]
          + quarterSquareTable[(ys +   s)._v]
          - quarterSquareTable[(ys -   s)._v];
        _s =
            quarterSquareTable[(ys -  yz)._v]
          - quarterSquareTable[(ys +  yz)._v]
          - quarterSquareTable[(yc -   s)._v]
          + quarterSquareTable[(yc +   s)._v];


        // Normalise rotor

        Fix8p8 ss = squareTable[_s._v];  // _s*_s
        Fix8p8 zz = squareTable[_xy._v]; // _xy*_xy
        Fix8p8 xx = squareTable[_yz._v]; // _yz*_yz
        Fix8p8 yy = squareTable[_zx._v]; // _zx*_zx
        Fix8p8 norm2 = ss + xx + yy + zz;
        ScaleTable<Fix8p8>* activeNormTable = normTable[norm2._v];
        _s  = (*activeNormTable)[_s._v];
        _xy = (*activeNormTable)[_xy._v];
        _yz = (*activeNormTable)[_yz._v];
        _zx = (*activeNormTable)[_zx._v];


        // Init view matrix
        // Reverse transform to find forward, down and right vectors for motion

        //Fix8p8 sz = _xy*_s;
        //Fix8p8 xy = _zx*_yz;
        //Fix8p8 sy = _zx*_s;
        //Fix8p8 xz = _yz*_xy;
        //Fix8p8 sx = _yz*_s;
        //Fix8p8 yz = _zx*_xy;
        //view._xx = (ss + xx - yy - zz)._v;
        //view._xy = (2*(xy - sz))._v;
        //view._xz = (2*(xz + sy))._v;
        //view._yx = (2*(xy + sz))._v;
        //view._yy = (ss - xx + yy - zz)._v;
        //view._yz = (2*(yz - sx))._v;
        //view._zx = (2*(xz - sy))._v;
        //view._zy = (2*(yz + sx))._v;
        //view._zz = (ss - xx - yy + zz)._v;

        Fix8p8 sspxx = ss + xx;
        Fix8p8 yypzz = yy + zz;
        Fix8p8 xy2 = squareTable[_yz._v + _zx._v] - xx - yy;
        Fix8p8 xz2 = squareTable[_yz._v + _xy._v] - xx - zz;
        Fix8p8 xs2 = squareTable[_yz._v + _s._v] - sspxx;
        Fix8p8 yz2 = squareTable[_zx._v + _xy._v] - yypzz;
        Fix8p8 ys2 = squareTable[_zx._v + _s._v] - yy - ss;
        Fix8p8 zs2 = squareTable[_xy._v + _s._v] - zz - ss;
        Fix8p8 ssmxx = ss - xx;
        Fix8p8 yymzz = yy - zz;
        motionMatrix._rx = sspxx - yypzz;
        view._xx = scaleTableX[motionMatrix._rx._v];
        motionMatrix._ry = xy2 - zs2;
        view._xy = scaleTableX[motionMatrix._ry._v];
        motionMatrix._rz = xz2 + ys2;
        view._xz = scaleTableX[motionMatrix._rz._v];
        motionMatrix._dx = xy2 + zs2;
        view._yx = scaleTableY[motionMatrix._dx._v];
        motionMatrix._dy = ssmxx + yymzz;
        view._yy = scaleTableY[motionMatrix._dy._v];
        motionMatrix._dz = yz2 - xs2;
        view._yz = scaleTableY[motionMatrix._dz._v];
        motionMatrix._fx = xz2 - ys2;
        view._zx = scaleTableZ[motionMatrix._fx._v];
        motionMatrix._fy = yz2 + xs2;
        view._zy = scaleTableZ[motionMatrix._fy._v];
        motionMatrix._fz = ssmxx - yymzz;
        view._zz = scaleTableZ[motionMatrix._fz._v];
    }
private:
    Rotor3D(Fix8p8 s, Fix8p8 xy, Fix8p8 yz, Fix8p8 zx)
      : _s(s), _xy(xy), _yz(yz), _zx(zx) { }

    Fix8p8 _s;
    Fix8p8 _xy;
    Fix8p8 _yz;
    Fix8p8 _zx;
};

Rotor3D viewRotor;

class ThreeDWindow : public RootWindow
{
public:
    void setOutput(CGAOutput* output) { _output = output; }
    void setConfig() //ConfigFile* configFile, File configPath)
    {
        //_configFile = configFile;
        //_sequencer.setROM(
        //  File(configFile->get<String>("cgaROM"), configPath.parent()));

        _output->setConnector(1);          // old composite
        _output->setScanlineProfile(0);    // rectangle
        _output->setHorizontalProfile(0);  // rectangle
        _output->setScanlineWidth(1);
        _output->setScanlineBleeding(2);   // symmetrical
        _output->setHorizontalBleeding(2); // symmetrical
        _output->setZoom(2);
        _output->setHorizontalRollOff(0);
        _output->setHorizontalLobes(4);
        _output->setVerticalRollOff(0);
        _output->setVerticalLobes(4);
        _output->setSubPixelSeparation(1);
        _output->setPhosphor(0);           // colour
        _output->setMask(0);
        _output->setMaskSize(0);
        _output->setAspectRatio(5.0/6.0);
        _output->setOverscan(0.1);
        _output->setCombFilter(0);         // no filter
        _output->setHue(0);
        _output->setSaturation(100);
        _output->setContrast(100);
        _output->setBrightness(0);
        _output->setShowClipping(false);
        _output->setChromaBandwidth(1);
        _output->setLumaBandwidth(1);
        _output->setRollOff(0);
        _output->setLobes(1.5);
        _output->setPhase(1);

        _regs = -CGAData::registerLogCharactersPerBank;
        _cgaBytes.allocate(0x4000 + _regs);
        _vram = &_cgaBytes[_regs];
        _vram[CGAData::registerLogCharactersPerBank] = 12;
        _vram[CGAData::registerScanlinesRepeat] = 1;
        _vram[CGAData::registerHorizontalTotalHigh] = 0;
        _vram[CGAData::registerHorizontalDisplayedHigh] = 0;
        _vram[CGAData::registerHorizontalSyncPositionHigh] = 0;
        _vram[CGAData::registerVerticalTotalHigh] = 0;
        _vram[CGAData::registerVerticalDisplayedHigh] = 0;
        _vram[CGAData::registerVerticalSyncPositionHigh] = 0;
        _vram[CGAData::registerMode] = 0x1a;
        _vram[CGAData::registerPalette] = 0x0f;
        _vram[CGAData::registerHorizontalTotal] = 57 - 1;
        _vram[CGAData::registerHorizontalDisplayed] = 40;
        _vram[CGAData::registerHorizontalSyncPosition] = 45;
        _vram[CGAData::registerHorizontalSyncWidth] = 10; // 16;
        _vram[CGAData::registerVerticalTotal] = 128 - 1;
        _vram[CGAData::registerVerticalTotalAdjust] = 6;
        _vram[CGAData::registerVerticalDisplayed] = 100;
        _vram[CGAData::registerVerticalSyncPosition] = 112;
        _vram[CGAData::registerInterlaceMode] = 2;
        _vram[CGAData::registerMaximumScanline] = 1;
        _vram[CGAData::registerCursorStart] = 6;
        _vram[CGAData::registerCursorEnd] = 7;
        _vram[CGAData::registerStartAddressHigh] = 0;
        _vram[CGAData::registerStartAddressLow] = 0;
        _vram[CGAData::registerCursorAddressHigh] = 0;
        _vram[CGAData::registerCursorAddressLow] = 0;
        _data.setTotals(238944, 910, 238875);
        _data.change(0, -_regs, _regs + 0x4000, &_cgaBytes[0]);

        _outputSize = _output->requiredSize();

        add(&_bitmap);
        add(&_animated);

        _animated.setDrawWindow(this);
        _animated.setRate(60);

        _frame = 0;

        _wPressed = false;
        _aPressed = false;
        _sPressed = false;
        _dPressed = false;
        _qPressed = false;
        _ePressed = false;
        _zPressed = false;
        _xPressed = false;
        _leftPressed = false;
        _rightPressed = false;
        _upPressed = false;
        _downPressed = false;

        //_rollAngularVelocity = 0;
        //_pitchAngularVelocity = 0;
        //_yawAngularVelocity = 0;
        _forwardVelocity = 0;
        _rightVelocity = 0;
        _downVelocity = 0;
        _x = 0;
        _y = 0;
        _z = 0;
        //_roll = 0;
        //_pitch = 0;
        //_yaw = 0;

        _clipping = false;
    }
    ~ThreeDWindow() { join(); }
    void join() { _output->join(); }
    void create()
    {
        setText("CGA 3D");
        setInnerSize(_outputSize);
        _bitmap.setTopLeft(Vector(0, 0));
        _bitmap.setInnerSize(_outputSize);
        RootWindow::create();
        _animated.start();
    }
    virtual void draw()
    {
        _data.change(0, -_regs, _regs + 0x4000, &_cgaBytes[0]);
        _output->restart();
        _animated.restart();

        // Process movement
        //_rollAngularVelocity = (_zPressed ? -1 : 0) + (_xPressed ? 1 : 0);
        //_pitchAngularVelocity = (_upPressed ? -1 : 0) + (_downPressed ? 1 : 0);
        //_yawAngularVelocity =
        //  (_leftPressed ? -1 : 0) + (_rightPressed ? 1 : 0);
        _forwardVelocity = (_wPressed ? 1 : 0) + (_sPressed ? -1 : 0);
        _rightVelocity = (_aPressed ? -1 : 0) + (_dPressed ? 1 : 0);
        _downVelocity = (_qPressed ? 1 : 0) + (_ePressed ? -1 : 0);

        //_roll += _rollAngularVelocity;
        //_pitch += _pitchAngularVelocity;
        //_yaw += _yawAngularVelocity;
        _x += motionMatrix.deltaX(_forwardVelocity, _rightVelocity, _downVelocity);
        _y += motionMatrix.deltaY(_forwardVelocity, _rightVelocity, _downVelocity);
        _z += motionMatrix.deltaZ(_forwardVelocity, _rightVelocity, _downVelocity);
    }
    void centreMouse()
    {
        RECT rc;
        GetWindowRect(_hWnd, &rc);
        ClipCursor(&rc);
        _centre = Vector((rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2);
        SetCursorPos(_centre.x, _centre.y);
        _centre = screenToClient(_centre);
    }
    virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        int w = static_cast<int>(wParam);
        switch (uMsg) {
            case WM_KILLFOCUS:
                if (_clipping) {
                    ClipCursor(NULL);
                    _clipping = false;
                    ShowCursor(TRUE);
                }
                break;
            case WM_SETFOCUS:
                if (!_clipping) {
                    _clipping = true;
                    centreMouse();
                    ShowCursor(FALSE);
                }
                break;
        }
        return RootWindow::handleMessage(uMsg, wParam, lParam);
    }

    bool keyboardEvent(int key, bool up)
    {
        switch (key) {
            case VK_RIGHT:
                _rightPressed = !up;
                return true;
            case VK_LEFT:
                _leftPressed = !up;
                return true;
            case VK_UP:
                _upPressed = !up;
                return true;
            case VK_DOWN:
                _downPressed = !up;
                return true;
            case 'W':
                _wPressed = !up;
                return true;
            case 'A':
                _aPressed = !up;
                return true;
            case 'S':
                _sPressed = !up;
                return true;
            case 'D':
                _dPressed = !up;
                return true;
            case 'Q':
                _qPressed = !up;
                return true;
            case 'E':
                _ePressed = !up;
                return true;
            case 'Z':
                _zPressed = !up;
                return true;
            case 'X':
                _xPressed = !up;
                return true;
            case VK_ESCAPE:
                PostQuitMessage(0);
                return true;
            //case VK_SPACE:
            //  _spacePressed = !up;
            //  return true;
        }
        return false;
    }
    bool mouseInput(Vector position, int buttons, int wheel)
    {
        if (_clipping) {
            Vector delta = position - _centre;
            viewRotor.rotate(delta.x, delta.y);
            centreMouse();
        }
    }
    BitmapWindow* outputWindow() { return &_bitmap; }
    CGAData* getData() { return &_data; }
    CGASequencer* getSequencer() { return &_sequencer; }
private:
    CGAData _data;
    CGASequencer _sequencer;
    CGAOutput* _output;
    //ConfigFile* _configFile;
    AnimatedWindow _animated;
    BitmapWindow _bitmap;
    Vector _outputSize;
    Array<Byte> _cgaBytes;
    Byte* _vram;
    int _regs;
    bool _leftPressed;
    bool _rightPressed;
    bool _upPressed;
    bool _downPressed;
    bool _qPressed;
    bool _wPressed;
    bool _ePressed;
    bool _aPressed;
    bool _sPressed;
    bool _dPressed;
    bool _zPressed;
    bool _xPressed;
    //float _rollAngularVelocity;   // around back/forward axis
    //float _pitchAngularVelocity;  // around left/right axis
    //float _yawAngularVelocity;    // around up/down axis
    Fix8p8 _forwardVelocity;
    Fix8p8 _rightVelocity;
    Fix8p8 _downVelocity;
    Fix8p8 _x;
    Fix8p8 _y;
    Fix8p8 _z;
    //float _roll;
    //float _pitch;
    //float _yaw;
    bool _clipping;
    int _mouseButtons;

    int _frame;
    Vector _centre;
};

class Program : public WindowProgram<ThreeDWindow>
{
public:
    void run()
    {
        totalNodeCount = 0x1000;  // 64kB of nodes, 2048 per page
        Array<Node> nodesArray(totalNodeCount);
        allNodes = &nodesArray[0];
        nodesInPool = totalNodeCount / 2;
        currentPool = allNodes;
        lastPool = allNodes + nodesInPool;
        nodesUsed = 1;
        currentPool[0].subdivided = false;
        currentPool[0].colourT = 0;
        currentPool[0].colourB = 0;
        lastPool[0].subdivided = false;
        lastPool[0].colourT = 0;
        lastPool[0].colourB = 0;



        //ConfigFile configFile;
        ////configFile.addDefaultOption("cgaROM", String("5788005.u33"));
        //configFile.addDefaultOption("fftWisdom", String("wisdom"));

        //String configName = "default.config";
        //if (_arguments.count() >= 2)
        //  configName = _arguments[1];
        //File configPath(configName, true);
        //configFile.load(configPath);
        //FFTWWisdom<float> wisdom(File(configFile.get<String>("fftWisdom"),
        //  configPath.parent()));

        CGAOutput output(_window.getData(), _window.getSequencer(),
            _window.outputWindow());
        _window.setOutput(&output);

        _window.setConfig(); // &configFile, configPath);

        WindowProgram::run();
        _window.join();
    }
};
