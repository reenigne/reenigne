#include "alfe/main.h"
#include "alfe/bitmap_png.h"
#include "alfe/colour_space.h"
#include "alfe/user.h"
#include "alfe/cga.h"
//#include "alfe/config_file.h"

struct ViewParameters
{
	SInt16 _ex;
	SInt16 _ey;
	SInt16 _ez;
	SInt16 _xx;
	SInt16 _xy;
	SInt16 _xz;
	SInt16 _yx;
	SInt16 _yy;
	SInt16 _yz;
	SInt16 _zx;
	SInt16 _zy;
	SInt16 _zz;
};

ViewParameters view;

struct Vertex
{
	SInt16 _x;
	SInt16 _y;
	SInt16 _z;
	SInt32 _sx;
	SInt32 _sy;
	SInt16 _sz;
	SInt16 _px;
	SInt16 _py;

	void transform()
	{
		SInt16 _rx = _x - view._ex;
		SInt16 _ry = _y - view._ey;
		SInt16 _rz = _z - view._ez;
		_sx = _rx * view._xx + _ry * view._xy + _rz * view._xz;
		_sy = _rx * view._yx + _ry * view._yy + _rz * view._yz;
		_sz = _rx * view._zx + _ry * view._zy + _rz * view._zz;
		if (_sz == 0 || _sx / _sz <= -0x8000 || _sx / _sz >= 0x8000)  // Handle with interrupt 0
			handleOverflow();
		else {
			_px = _sx / _sz;
			if (_sy / _sz <= -0x8000 || _sy / _sz >= 0x8000)  // Handle with interrupt 0
				handleOverflow();
			else
				_py = _sy / _sz;
		}
	}
	SInt16 shift14div(SInt32 m, SInt32 d)
	{
		SInt32 h = d >> 16;
		if (h > 0) {               // 0x0001'0000 <= d < 0x8000'0000
			if (h >= 0x100) {        // 0x0100'0000 <= d < 0x8000'0000
				if (h >= 0x1000) {     // 0x1000'0000 <= d < 0x8000'0000
					if (h >= 0x2000) {   // 0x2000'0000 <= d < 0x8000'0000
						if (h >= 0x4000) { // 0x4000'0000 <= d < 0x8000'0000
							d >>= 15;
							m >>= 1;
						}
						else                    // 0x2000'0000 <= d < 0x4000'0000
							d >>= 14;
					}
					else {                    // 0x1000'0000 <= d < 0x2000'0000
						d >>= 13;
						m <<= 1;
					}
				}
				else {                      // 0x0100'0000 <= d < 0x1000'0000
					if (h >= 0x400) {    // 0x0400'0000 <= d < 0x1000'0000
						if (h >= 0x800) {  // 0x0800'0000 <= d < 0x1000'0000
							d >>= 12;
							m <<= 2;
						}
						else {                  // 0x0400'0000 <= d < 0x0800'0000
							d >>= 11;
							m <<= 3;
						}
					}
					else {                    // 0x0100'0000 <= d < 0x0400'0000
						if (h >= 0x200) {  // 0x0200'0000 <= d < 0x0400'0000
							d >>= 10;
							m <<= 4;
						}
						else {                  // 0x0100'0000 <= d < 0x0200'0000
							d >>= 9;
							m <<= 5;
						}
					}
				}
			}
			else {                        // 0x0001'0000 <= d < 0x0100'0000
				if (h >= 0x10) {       // 0x0010'0000 <= d < 0x0100'0000
					if (h >= 0x40) {     // 0x0040'0000 <= d < 0x0100'0000
						if (h >= 0x80) {   // 0x0080'0000 <= d < 0x0100'0000
							d >>= 8;
							m <<= 6;
						}
						else {                  // 0x0040'0000 <= d < 0x0080'0000
							d >>= 7;
							m <<= 7;
						}
					}
					else {                    // 0x0010'0000 <= d < 0x0040'0000
						if (h >= 0x20) {   // 0x0020'0000 <= d < 0x0040'0000
							d >>= 6;
							m <<= 8;
						}
						else {                  // 0x0010'0000 <= d < 0x0020'0000
							d >>= 5;
							m <<= 9;
						}
					}
				}
				else {                      // 0x0001'0000 <= d < 0x0010'0000
					if (h >= 0x4) {      // 0x0004'0000 <= d < 0x0010'0000
						if (h >= 0x8) {    // 0x0008'0000 <= d < 0x0010'0000
							d >>= 4;
							m <<= 10;
						}
						else {                  // 0x0004'0000 <= d < 0x0008'0000
							d >>= 3;
							m <<= 11;
						}
					}
					else {                    // 0x0001'0000 <= d < 0x0004'0000
						if (h >= 0x2) {    // 0x0002'0000 <= d < 0x0004'0000
							d >>= 2;
							m <<= 12;
						}
						else {                    // 0x0001'0000 <= d < 0x0002'0000
							d >>= 1;
							m <<= 13;
						}
					}
				}
			}
			return m / static_cast<SInt16>(d);
		}
		// 0x0000'0000 <= d < 0x0001'0000
		if ((d & 0xffff) == 0)  // Handle with interrupt 0? (Probably quicker to just test)
			return 0x4000;
		else
			return (m << 14) / static_cast<SInt16>(d); // No point shifting both d and m left - it won't change the result!
	}
	void handleOverflow()
	{
		if (_sx >= 0) {
			if (_sy >= 0) {
				if (_sx > _sy) {   // 0 <= _sy < _sx
					_px = 0x4000;
					_py = shift14div(_sy, _sx);
				}
				else {             // 0 <= _sx <= _sy
					_py = 0x4000;
					_px = shift14div(_sx, _sy);
				}
			}
			else { // _sy < 0
				if (_sx > -_sy) {  // 0 < -_sy < _sx
					_px = 0x4000;
					_py = shift14div(_sy, _sx);
				}
				else {             // 0 < _sx <= -_sy
					_py = -0x4000;
					_px = -shift14div(_sx, _sy);
				}
			}
		}
		else { // _sx < 0
			if (_sy >= 0) {
				if (-_sx > _sy) {  // 0 <= _sy < -_sx
					_px = -0x4000;
					_py = -shift14div(_sy, _sx);
				}
				else {             // 0 < -_sx <= _sy
					_py = 0x4000;
					_px = shift14div(_sx, _sy);
				}
			}
			else { // _sy < 0
				if (-_sx > -_sy) { // 0 < -_sy < -_sx
					_px = -0x4000;
					_py = -shift14div(_sy, _sx);
				}
				else {             // 0 < -_sx <= -_sy
					_py = -0x4000;
					_px = -shift14div(_sx, _sy);
				}
			}
		}
		if (_sz < 0) {  // => interior/exterior code should treat Z=0 as positive
			_px = -_px;
			_py = -_py;
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

	SInt16 _intersect;
	SInt16 _span;
	bool _horizontalish;
	bool _inverted;
	bool _degenerate;
	bool _processed;
	SInt16 _reversedIntersect;
	SInt16 _reversedSpan;

	void init()
	{
		_vertexA = &vertices[_vertexIndexA];
		_vertexB = &vertices[_vertexIndexB];
	}

	SInt16 safeDivide(SInt32 intersect, SInt16 span)  // Handle with interrupt 0
	{
		SInt16 intersectHigh = intersect >> 16;
		if (intersectHigh >= 0) {
			if (span >= 0) {  // Note: span can never be 0
				if (intersectHigh >= span)
					return 0x4000;
			}
			else { // span < 0
				if (intersectHigh >= -span)
					return -0x4000;
			}
		}
		else {
			if (span >= 0) {
				if (-intersectHigh >= span)
					return -0x4000;
			}
			else { // span < 0
				if (-intersectHigh >= -span)
					return 0x4000;
			}
		}
		return intersect / span;

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
		SInt32 intersect = ax * by - bx * ay;

		// divide through by the largest of |spanx| and |spany|

		if (spanx < 0) {
			if (spany < 0) {
				if (-spanx > -spany) {
					_horizontalish = true;
					_span = /*-*/(spany << spanBits) / spanx;
					_intersect = /*-*/safeDivide(intersect, spanx);
					_inverted = true;
				}
				else {
					_horizontalish = false;
					_span = /*-*/(spanx << spanBits) / spany;
					_intersect = /*-*/safeDivide(intersect, spany);
					_inverted = true;
				}
			}
			else { // spany >= 0
				if (-spanx > spany) {
					_horizontalish = true;
					_span = /*-*/(spany << spanBits) / spanx;
					_intersect = /*-*/safeDivide(intersect, spanx);
					_inverted = true;
				}
				else {
					_horizontalish = false;
					_span = (spanx << spanBits) / spany;
					_intersect = safeDivide(intersect, spany);
					_inverted = false;
				}
			}
		}
		else { // spanx >= 0
			if (spany < 0) {
				if (spanx > -spany) {
					_horizontalish = true;
					_span = (spany << spanBits) / spanx;
					_intersect = safeDivide(intersect, spanx);
					_inverted = false;
				}
				else {
					_horizontalish = false;
					_span = /*-*/(spanx << spanBits) / spany;
					_intersect = /*-*/safeDivide(intersect, spany);
					_inverted = true;
				}
			}
			else { // spany >= 0
				if (spanx > spany) {
					_horizontalish = true;
					_span = (spany << spanBits) / spanx;
					_intersect = safeDivide(intersect, spanx);
					_inverted = false;
				}
				else {
					if (spany == 0) {  // Handle with interrupt 0
						// We have a separate _degenerate flag for this case
						// because both sides of the edge should be "outside"
						// so that nothing is drawn. There is a case to be made
						// for drawing something if the edge intersects the eye
						// (i.e. the two points project to the same place on
						// the screen but have opposite z signs) but we should
						// never let the eye get that close to any triangles.
						_processed = true;
						_degenerate = true;
						return true;
					}
					else {
						_horizontalish = false;
						_span = (spanx << spanBits) / spany;
						_intersect = safeDivide(intersect, spany);
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
					// TODO: draw edges CB and AC
					if (_edgeBC->process())
						return;
					if (_edgeCA->process())
						return;
				}
			}
			else {  //_vertexB->_sz > 0
				if (_vertexC->_sz <= 0) {
					// TODO: draw edges BA and CB
					if (_edgeAB->process())
						return;
					if (_edgeBC->process())
						return;
				}
				else {  //_vertexC->_sz > 0
					// TODO: draw edges AB, CB and CA
					if (_edgeAB->process())
						return;
					if (_edgeBC->process())
						return;
					if (_edgeCA->process())
						return;
				}
			}
		}
		else {  // _vertexA->_sz > 0
			if (_vertexB->_sz <= 0) {
				if (_vertexC->_sz <= 0) {
					// TODO: draw edges AC and BA
					if (_edgeAB->process())
						return;
					if (_edgeCA->process())
						return;
				}
				else {  //_vertexC->_sz > 0
					// TODO: draw edges AB, BC and AC
					if (_edgeAB->process())
						return;
					if (_edgeBC->process())
						return;
					if (_edgeCA->process())
						return;
				}
			}
			else {  //_vertexB->_sz > 0
				if (_vertexC->_sz <= 0) {
					// TODO: draw edges BA, BC and CA
					if (_edgeAB->process())
						return;
					if (_edgeBC->process())
						return;
					if (_edgeCA->process())
						return;
				}
				else {  //_vertexC->_sz > 0
					// TODO: draw edges AB, BC and CA
					if (_edgeAB->process())
						return;
					if (_edgeBC->process())
						return;
					if (_edgeCA->process())
						return;
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

Node* nodes;
int nodeCount;

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
		Node* newNode = &nodes[nodeCount];
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
		nodeCount += 4;
	}
	bool insideABTM, insideABML, insideABMM, insideABMR, insideABBM;
	bool insideBCTM, insideBCML, insideBCMM, insideBCMR, insideBCBM;
	bool insideCATM, insideCAML, insideCAMM, insideCAMR, insideCABM;
	// Initial intersect is at TL point
	// Compute intersect at TM point
	SInt16 interceptABTM, interceptABML, interceptABMM, interceptABMR, interceptABBM;
	SInt16 interceptBCTM, interceptBCML, interceptBCMM, interceptBCMR, interceptBCBM;
	SInt16 interceptCATM, interceptCAML, interceptCAMM, interceptCAMR, interceptCABM;
	SInt16 spanABR = spanAB >> 1;
	SInt16 spanABB = spanAB >> 1;
	SInt16 spanBCR = spanBC >> 1;
	SInt16 spanBCB = spanBC >> 1;
	SInt16 spanCAR = spanCA >> 1;
	SInt16 spanCAB = spanCA >> 1;
	if (activeAB) {
		if (horizontalishAB) {
			interceptABTM = interceptAB + (spanAB >> 1);
			insideABTM = ((interceptABTM > 0) != invertedAB);
			interceptABML = interceptAB + (1 << (level + spanBits - 10));
			insideABML = ((interceptABML > 0) != invertedAB);
			interceptABMM = interceptABTM + (1 << (level + spanBits - 10));
			insideABMM = ((interceptABMM > 0) != invertedAB);
			interceptABMR = interceptABML + spanAB;
			insideABMR = ((interceptABMR > 0) != invertedAB);
			interceptABBM = interceptABMM + (1 << (level + spanBits - 10));
			insideABBM = ((interceptABBM > 0) != invertedAB);
			spanABR = spanAB - (spanAB >> 1);
		}
		else {
			interceptABTM = interceptAB + (1 << (level + spanBits - 10));
			insideABTM = ((interceptABTM > 0) != invertedAB);
			interceptABML = interceptAB + (spanAB >> 1);
			insideABML = ((interceptABML > 0) != invertedAB);
			interceptABMM = interceptABML + (1 << (level + spanBits - 10));
			insideABMM = ((interceptABMM > 0) != invertedAB);
			interceptABMR = interceptABML + (1 << (level + spanBits - 9));
			insideABMR = ((interceptABMR > 0) != invertedAB);
			interceptABBM = interceptABTM + spanAB;
			insideABBM = ((interceptABBM > 0) != invertedAB);
			spanABB = spanAB - (spanAB >> 1);
		}
	}
	if (activeBC) {
		if (horizontalishBC) {
			interceptBCTM = interceptBC + (spanBC >> 1);
			insideBCTM = ((interceptBCTM > 0) != invertedBC);
			interceptBCML = interceptBC + (1 << (level + spanBits - 10));
			insideBCML = ((interceptBCML > 0) != invertedBC);
			interceptBCMM = interceptBCTM + (1 << (level + spanBits - 10));
			insideBCMM = ((interceptBCMM > 0) != invertedBC);
			interceptBCMR = interceptBCML + spanBC;
			insideBCMR = ((interceptBCMR > 0) != invertedBC);
			interceptBCBM = interceptBCMM + (1 << (level + spanBits - 10));
			insideBCBM = ((interceptBCBM > 0) != invertedBC);
			spanBCR = spanBC - (spanBC >> 1);
		}
		else {
			interceptBCTM = interceptBC + (1 << (level + spanBits - 10));
			insideBCTM = ((interceptBCTM > 0) != invertedBC);
			interceptBCML = interceptBC + (spanBC >> 1);
			insideBCML = ((interceptBCML > 0) != invertedBC);
			interceptBCMM = interceptBCML + (1 << (level + spanBits - 10));
			insideBCMM = ((interceptBCMM > 0) != invertedBC);
			interceptBCMR = interceptBCML + (1 << (level + spanBits - 9));
			insideBCMR = ((interceptBCMR > 0) != invertedBC);
			interceptBCBM = interceptBCTM + spanBC;
			insideBCBM = ((interceptBCBM > 0) != invertedBC);
			spanBCB = spanBC - (spanBC >> 1);
		}
	}
	if (activeCA) {
		if (horizontalishCA) {
			interceptCATM = interceptCA + (spanCA >> 1);
			insideCATM = ((interceptCATM > 0) != invertedCA);
			interceptCAML = interceptCA + (1 << (level + spanBits - 10));
			insideCAML = ((interceptCAML > 0) != invertedCA);
			interceptCAMM = interceptCATM + (1 << (level + spanBits - 10));
			insideCAMM = ((interceptCAMM > 0) != invertedCA);
			interceptCAMR = interceptCAML + spanCA;
			insideCAMR = ((interceptCAMR > 0) != invertedCA);
			interceptCABM = interceptCAMM + (1 << (level + spanBits - 10));
			insideCABM = ((interceptCABM > 0) != invertedCA);
			spanCAR = spanCA - (spanCA >> 1);
		}
		else {
			interceptCATM = interceptCA + (1 << (level + spanBits - 10));
			insideCATM = ((interceptCATM > 0) != invertedCA);
			interceptCAML = interceptCA + (spanCA >> 1);
			insideCAML = ((interceptCAML > 0) != invertedCA);
			interceptCAMM = interceptCAML + (1 << (level + spanBits - 10));
			insideCAMM = ((interceptCAMM > 0) != invertedCA);
			interceptCAMR = interceptCAML + (1 << (level + spanBits - 9));
			insideCAMR = ((interceptCAMR > 0) != invertedCA);
			interceptCABM = interceptCATM + spanCA;
			insideCABM = ((interceptCABM > 0) != invertedCA);
			spanCAB = spanCA - (spanCA >> 1);
		}
	}
	// TODO: Replace some span >> 1 with span - (span >> 1) depending on horizontalish
	render(level - 1, node->tl,
		interceptAB, interceptBC, interceptCA,
		spanAB >> 1, spanBC >> 1, spanCA >> 1,
		activeAB, activeBC, activeCA,
		invertedAB, invertedBC, invertedCA,
		horizontalishAB, horizontalishBC, horizontalishCA,
		x, y,
		insideABTL, insideABTM, insideABML, insideABMM,
		insideBCTL, insideBCTM, insideBCML, insideBCMM,
		insideCATL, insideCATM, insideCAML, insideCAMM,
		colour, highres);
	if (x + (1 << (level - 1)) < 320) {
		render(level - 1, node->tr,
			interceptABTM, interceptBCTM, interceptCATM,
			spanABR, spanBCR, spanCAR,
			activeAB, activeBC, activeCA,
			invertedAB, invertedBC, invertedCA,
			horizontalishAB, horizontalishBC, horizontalishCA,
			x + (1 << (level - 1)), y,
			insideABTM, insideABTR, insideABMM, insideABMR,
			insideBCTM, insideBCTR, insideBCMM, insideBCMR,
			insideCATM, insideCATR, insideCAMM, insideCAMR,
			colour, highres);
	}
	if (y + (1 << (level - 1)) < 200) {
		render(level - 1, node->bl,
			interceptABML, interceptBCML, interceptCAML,
			spanABB, spanBCB, spanCAB,
			activeAB, activeBC, activeCA,
			invertedAB, invertedBC, invertedCA,
			horizontalishAB, horizontalishBC, horizontalishCA,
			x, y + (1 << (level - 1)),
			insideABML, insideABMM, insideABBL, insideABBM,
			insideBCML, insideBCMM, insideBCBL, insideBCBM,
			insideCAML, insideCAMM, insideCABL, insideCABM,
			colour, highres);
		if (x + (1 << (level - 1)) < 320) {
			render(level - 1, node->br,
				interceptABMM, interceptBCMM, interceptCAMM,
				spanAB - (spanAB >> 1), spanBC - (spanBC >> 1), spanCA - (spanCA >> 1),
				activeAB, activeBC, activeCA,
				invertedAB, invertedBC, invertedCA,
				horizontalishAB, horizontalishBC, horizontalishCA,
				x + (1 << (level - 1)), y + (1 << (level - 1)),
				insideABMM, insideABMR, insideABBM, insideABBR,
				insideBCMM, insideBCMR, insideBCBM, insideBCBR,
				insideCAMM, insideCAMR, insideCABM, insideCABR,
				colour, highres);
		}
	}
}


Triangle* triangles;
int triangleCount;
Triangle** sortedTriangles;
int sortedTriangleCount;
Triangle** sortBuffer;
int nextFreeSortPosition;

struct Shape
{
	void process()
	{
		for (int i = 0; i < vertexCount; ++i)
			vertices[i].transform();
		for (int i = 0; i < edgeCount; ++i)
			edges[i]._processed = false;
		
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
};

class ThreeDWindow : public RootWindow
{
public:
	void setOutput(CGAOutput* output) { _output = output; }
	void setConfig() //ConfigFile* configFile, File configPath)
	{
		//_configFile = configFile;
		//_sequencer.setROM(
		//	File(configFile->get<String>("cgaROM"), configPath.parent()));

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

		_rollAngularVelocity = 0;
		_pitchAngularVelocity = 0;
		_yawAngularVelocity = 0;
		_forwardVelocity = 0;
		_rightVelocity = 0;
		_downVelocity = 0;
		_x = 0;
		_y = 0;
		_z = 0;
		_roll = 0;
		_pitch = 0;
		_yaw = 0;

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
		_rollAngularVelocity = (_zPressed ? -1 : 0) + (_xPressed ? 1 : 0);
		_pitchAngularVelocity = (_upPressed ? -1 : 0) + (_downPressed ? 1 : 0);
		_yawAngularVelocity =
			(_leftPressed ? -1 : 0) + (_rightPressed ? 1 : 0);
		_forwardVelocity = (_wPressed ? 1 : 0) + (_sPressed ? -1 : 0);
		_rightVelocity = (_aPressed ? -1 : 0) + (_dPressed ? 1 : 0);
		_downVelocity = (_qPressed ? 1 : 0) + (_ePressed ? -1 : 0);

		_roll += _rollAngularVelocity;
		_pitch += _pitchAngularVelocity;
		_yaw += _yawAngularVelocity;
		_x += _forwardVelocity;  // TODO: these should be relative to direction
		_y += _rightVelocity;
		_z += _downVelocity;


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
			//	_spacePressed = !up;
			//	return true;
		}
		return false;
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
	float _rollAngularVelocity;   // around back/forward axis
	float _pitchAngularVelocity;  // around left/right axis
	float _yawAngularVelocity;    // around up/down axis
	float _forwardVelocity;
	float _rightVelocity;
	float _downVelocity;
	float _x;
	float _y;
	float _z;
	float _roll;
	float _pitch;
	float _yaw;

	int _frame;
};

class Program : public WindowProgram<ThreeDWindow>
{
public:
	void run()
	{
		//ConfigFile configFile;
		////configFile.addDefaultOption("cgaROM", String("5788005.u33"));
		//configFile.addDefaultOption("fftWisdom", String("wisdom"));

		//String configName = "default.config";
		//if (_arguments.count() >= 2)
		//	configName = _arguments[1];
		//File configPath(configName, true);
		//configFile.load(configPath);
		//FFTWWisdom<float> wisdom(File(configFile.get<String>("fftWisdom"),
		//	configPath.parent()));

		CGAOutput output(_window.getData(), _window.getSequencer(),
			_window.outputWindow());
		_window.setOutput(&output);

		_window.setConfig(); // &configFile, configPath);

		WindowProgram::run();
		_window.join();
	}
};
