class BlockZoomData
{
public:
private:
    int _position;
    int _width;
};

class BlockSizeZoomData
{
    Array<BlockZoomData> _blocks;
};

class ZoomData
{
public:
private:
    Array<BlockSizeZoomData> _levels;
};


class BlockZoom
{
public:
private:
    int _position;
    int _width;
    BlockZoom* _left;
    BlockZoom* _right;
};


class BlockZoomX
{
public:
private:
    FunctionPointer _drawFunc;
    BlockZoom* _left;
    BlockZoom* _right;
};
