// This file implements the Matrix. This stores information about complete and
// incomplete points using as little time and space overhead as possible.
//
// In order to store large amounts of information in as little space as
// possible, we avoid using C++ polymorphism which would store type information
// with every leaf - when we have a square grid 2^n by 2^n points of all the
// same type, we store them in a grid, which has bits describing the type at
// the start followed by the actual data.
//
// A block is either a grid block or a leaf block. A grid block holds a grid
// which contains other blocks, all of the same type. A leaf block is either
// complete or incomplete.

#include <intrin.h>

enum BlockType
{
    gridBlockType,
    incompleteLeafType,
    completeLeafType,
    lastBlockType  // Keep this last (not actually used in Grid).
};


// Forward declarations
template<class FractalProcessor> class Grid;
template<class FractalProcessor> class BlockPointer;

// A block is a member of a grid and is either a point or a pointer to a
// sub-grid.
class Block
{
    // Block contains no member data directly - we must cast the this pointer
    // to extract the data.
};


// A block representing a sub-grid
// sizeof(GridBlock) == 4
template<class FractalProcessor> class GridBlock : public Block
{
public:
    Grid<FractalProcessor>* _grid;
};


// A leaf represents a particular complex number - an input to the fractal
// algorithm which produces a colour as an output.
class Leaf : public Block
{
public:
    // Colour to plot. This may be different from _iterations, as blocks that
    // have been subdivided should be plotted in the same colour as the block
    // that they were subdivided from until they've iterated by more than that.
    unsigned int _colour;
};


// A leaf that has escaped or is periodic.
class CompleteLeaf : public Leaf
{ };


// A leaf for which the decision about whether or not it is in the set has not
// been made.
template<class FractalProcessor> class IncompleteLeaf
  : public Leaf, public LinkedListMember<IncompleteLeaf<FractalProcessor> >
{
    typedef Grid<FractalProcessor> Grid;
    typedef BlockPointer<FractalProcessor> BlockPointer;
public:
    int precision() const { return _parent->precision(); }

    // BlockPointer for this leaf.
    BlockPointer blockPointer() const
    {
        return BlockPointer(_parent, _parent->_gridType.positionForIndex(
            (reinterpret_cast<const Byte*>(this) -
                (reinterpret_cast<const Byte*>(_parent) + sizeof(Grid))) /
                    bytes()));
    }

    void init(IncompleteLeaf* from)
    {
        _colour = from->_colour;
        _iterations = from->_iterations;
        int op = from->precision() + 1;
        int p = precision() + 1;
        for (int i = 0; i < numbers(); ++i)
            copy(number(i), p, from->number(i), op);
    }

    Digit* number(int offset)
    {
        return reinterpret_cast<Digit*>(
            reinterpret_cast<Byte*>(this) + sizeof(IncompleteLeaf) +
            offset*(precision() + 1)*sizeof(Digit));
    }

    IncompleteLeaf* next()
    {
        return reinterpret_cast<IncompleteLeaf*>(number(numbers()));
    }

    void init(int colour)
    {
        _colour = colour;
        _iterations = 0;
        for (int i = 0; i < numbers(); ++i)
            zero(number(i), precision() + 1);
    }

    int bytes() const
    {
        return sizeof(IncompleteLeaf) +
            numbers()*sizeof(Digit)*(precision() + 1);
    }

    unsigned int _iterations;

    // Which grid is this leaf in?
    Grid* _parent;

    static int numbers() { return 4; }

    // Data for _z and _zS follows in "struck hack" fashion: _z.x, _z.y, _zS.x
    // _zS.y.
};


template<class FractalProcessor> class GridType
{
    typedef IncompleteLeaf<FractalProcessor> IncompleteLeaf;
    typedef Grid<FractalProcessor> Grid;
    typedef GridBlock<FractalProcessor> GridBlock;
public:
    GridType(int logBlocks, BlockType blockType, int precision,
        int logPointsPerBlockOffset)
      : _logBlocks(logBlocks),
        _blockType(blockType),
        _precision(precision),
        _logPointsPerBlockOffset(logPointsPerBlockOffset)
    { }

    Vector positionForIndex(int index)
    {
        return Vector(index&((1 << _logBlocks) - 1), index >> _logBlocks);
    }

    int indexForPosition(Vector position)
    {
        return (position.y << _logBlocks) | position.x;
    }

    int bytesPerBlock()
    {
        if (_blockType == completeLeafType)
            return sizeof(CompleteLeaf);
        if (_blockType == incompleteLeafType)
            return sizeof(IncompleteLeaf) +
                IncompleteLeaf::numbers()*sizeof(Digit)*(_precision + 1);
        return sizeof(GridBlock);
    }

    int bytes() { return itemOffset(count()); }
    int count() const { return 1 << (_logBlocks << 1); }
    int itemOffset(int index) { return sizeof(Grid) + index*bytesPerBlock(); }

    bool operator==(GridType other)
    {
        return _logBlocks == other._logBlocks &&
            _blockType == other._blockType && _precision == other._precision;
    }

    bool operator!=(GridType other) { return !operator==(other); }

    // 0 for 1x1, 1 for 2x2, 2 for 4x4 etc... up to 15 for 32768x32768
    int _logBlocks : 5;

    // Type of items in grid. Technically only 2 are needed so we can reclaim a
    // bit here if necessary, but the VS debugger gets confused about enums
    // using the most significant bit in a bitfield.
    BlockType _blockType : 3;

    // 10 bits means we can have precisions of 32 to 32768 bits, which is also
    // the range of the zoom level.
    int _precision : 10;

    // Given a grid, we need to be able to find the point size of its blocks
    // for reseating and final deletion purposes.
    int _logPointsPerBlockOffset : 6;

    // 8 bits left for future expansion.
};


// A grid is a square array of blocks whose width is a power of 2.
// sizeof(Grid) == 12
template<class FractalProcessor> class Grid
{
    typedef GridBlock<FractalProcessor> GridBlock;
    typedef IncompleteLeaf<FractalProcessor> IncompleteLeaf;
    typedef BlockPointer<FractalProcessor> BlockPointer;
    typedef GridType<FractalProcessor> GridType;
    typedef Matrix<FractalProcessor> Matrix;
public:
    // The address of the gridBlock containing this grid.
    BlockPointer blockPointer() const
    {
        if (_parent == 0)
            return BlockPointer();
        return BlockPointer(_parent,
            _parent->_gridType.positionForIndex(_index));
    }

    Block* blockAtPosition(Vector position)
    {
        return blockAtIndex(_gridType.indexForPosition(position));
    }

    void reparent(BlockPointer address)
    {
        _parent = address._grid;
        if (_parent != 0) {
            address.subGrid() = this;
            _index = _parent->_gridType.indexForPosition(address._position);
        }
    }

    Grid(GridType gridType) : _gridType(gridType) { }

    void moveFrom(FractalProcessor* processor, Grid* oldGrid)
    {
        switch (oldGrid->_gridType._blockType) {
            case gridBlockType:
                {
                    memcpy(this, oldGrid, oldGrid->_gridType.bytes());
                    GridBlock* gridBlock =
                        static_cast<GridBlock*>(blockAtIndex(0));
                    int n = _gridType.count();
                    for (int i = 0; i < n; ++i) {
                        Grid* grid = (gridBlock++)->_grid;
                        if (grid != 0)
                            grid->_parent = this;
                    }
                }
                break;
            case incompleteLeafType:
                {
                    *this = *oldGrid;
                    int logPointsPerLeaf =
                        logPointsPerBlock(processor->matrix());

                    // Move the leaves. We do this one by one instead of with
                    // a memcpy() in order to get the LinkedList pointers
                    // correct.
                    IncompleteLeaf* incompleteLeaf =
                        static_cast<IncompleteLeaf*>(blockAtIndex(0));
                    IncompleteLeaf* oldLeaf = static_cast<IncompleteLeaf*>(
                        oldGrid->blockAtIndex(0));
                    int n = _gridType.count();
                    for (int i = 0; i < n; ++i) {
                        incompleteLeaf->moveFrom(oldLeaf);
                        incompleteLeaf->init(oldLeaf);
                        processor->tracker()->reseatLeaf(oldLeaf,
                            incompleteLeaf, logPointsPerLeaf);
                        incompleteLeaf->_parent = this;
                        incompleteLeaf = incompleteLeaf->next();
                        oldLeaf = oldLeaf->next();
                    }
                }
                break;
            case completeLeafType:
                memcpy(this, oldGrid, oldGrid->_gridType.bytes());
                break;

        }
        processor->matrix()->reseatGrid(oldGrid, this);
        if (_parent != 0)
            reinterpret_cast<GridBlock*>(_parent->blockAtIndex(_index))->_grid
                = this;
    }

    int precision() const { return _gridType._precision; }

    int logPointsPerBlock(Matrix* matrix)
    {
        return (_gridType._logPointsPerBlockOffset +
            matrix->logPointsPerBlockBase())&0x3f;
    }

private:
    Block* blockAtIndex(int index)
    {
        return reinterpret_cast<Block*>(
            reinterpret_cast<Byte*>(this) + _gridType.itemOffset(index));
    }

public:
    GridType _gridType;

    // Which grid is this grid in? (0 if this is the matrix grid or is
    // disconnected).
    Grid* _parent;

    // Where in the parent grid is this grid?
    int _index;

    // Actual grid data (an array of the type described by _gridType follows
    // this object in "struct hack" fashion.
};


// This class encapsulates a point location and size. The lowest _logPoints
// bits of the components of _topLeft should always be 0.
class BlockLocation
{
public:
    BlockLocation()
      : _topLeft(0, 0),
        _logPoints(29)
    { }

    BlockLocation(Vector topLeft, int logPoints)
      : _topLeft(topLeft),
        _logPoints(logPoints)
    { }

    // Divide this square up into a 2^logPoints by 2^logPoints grid, and return
    // the location of the square at position "position" within that grid.
    BlockLocation child(Vector position, int logPoints)
    {
        int s = _logPoints - logPoints;
        return BlockLocation(_topLeft | (position << s), s);
    }

    // Is point contained within this location?
    bool contains(Vector point)
    {
        int s = 1 << _logPoints;
        return (point - _topLeft).inside(Vector(s, s));
    }

    // Is location contained entirely within this location?
    bool contains(BlockLocation location)
    {
        return contains(location._topLeft) &&
            contains(location._topLeft + (Vector(1, 1)<<location._logPoints));
    }

    Vector _topLeft;
    int _logPoints;
};


// The following class hierarchy exists just to break up the matrix
// functionality into chunks, each more high-level than the last.

// This class encapsulates a block within a grid.
template<class FractalProcessor> class BlockPointer
{
protected:
    typedef Grid<FractalProcessor> Grid;
    typedef GridBlock<FractalProcessor> GridBlock;
    typedef IncompleteLeaf<FractalProcessor> IncompleteLeaf;
    typedef GridType<FractalProcessor> GridType;

public:
    BlockPointer()
      : _grid(0),
        _position(0, 0)
    { }

    BlockPointer(Grid* grid, Vector position = Vector(0, 0))
      : _grid(grid),
        _position(position)
    { }

    IncompleteLeaf* incompleteLeaf() const
    {
        return static_cast<IncompleteLeaf*>(leaf());
    }

    bool isComplete() const { return blockType() == completeLeafType; }

    Grid*& subGrid() { return gridBlock()->_grid; }

    Grid* _grid;

    Vector _position;

protected:
    void ascend() { *this = _grid->blockPointer(); }

    // Assuming blockType() == gridBlockType, move to a block in the child
    // grid.
    void descend(Vector position = Vector(0, 0))
    {
        _grid = subGrid();
        _position = position;
    }

    CompleteLeaf* completeLeaf() const
    {
        return static_cast<CompleteLeaf*>(leaf());
    }

    unsigned int& colour() { return leaf()->_colour; }
    BlockType blockType() const { return _grid->_gridType._blockType; }
    int precision() const { return _grid->_gridType._precision; }
    int bytesPerBlock() const { return _grid->_gridType.bytesPerBlock(); }
    bool atTop() const { return _grid == 0; }
    Block* block() const { return _grid->blockAtPosition(_position); }
    Leaf* leaf() const { return static_cast<Leaf*>(block()); }
    int logBlocksPerGrid() const { return _grid->_gridType._logBlocks; }
    int blocksPerGrid() const { return 1 << logBlocksPerGrid(); }
    bool atEnd() const { return _position.y == blocksPerGrid(); }
    GridBlock* gridBlock() const { return static_cast<GridBlock*>(block()); }
    Grid* subGrid() const { return gridBlock()->_grid; }
};


// This class encapsulates a square group of blocks.
template<class FractalProcessor> class BlockGroup
  : public BlockPointer<FractalProcessor>
{
protected:
    BlockGroup(Grid* grid)
      : BlockPointer(grid),
        _logBlocksPerGroup(0)
    { }

    bool atLastQuadrant()
    {
        return logBlocksPerGrid() == 0 ||
            ((_position >> _logBlocksPerGroup) & 1) == Vector(1, 1);
    }

    void ascend() { BlockPointer::ascend(); _logBlocksPerGroup = 0; }

    void descend(Vector position = Vector(0, 0))
    {
        BlockPointer::descend(position);
        _logBlocksPerGroup = 0;
    }

    int blocksPerGroup() const { return 1 << _logBlocksPerGroup; }

    // Move the blocks from source to this group.
    void moveBlocks(BlockGroup source)
    {
        int destinationStride = blocksPerGrid();
        int sourceStride = source.blocksPerGrid();
        int width = blocksPerGroup();
        if (blockType() == incompleteLeafType) {
            int bytes = bytesPerBlock();
            destinationStride *= bytes;
            sourceStride *= bytes;
            Byte* sourceLine =
                reinterpret_cast<Byte*>(source.incompleteLeaf());
            Byte* destinationLine = reinterpret_cast<Byte*>(incompleteLeaf());
            for (int y = 0; y < width; ++y) {
                Byte* source = sourceLine;
                Byte* destination = destinationLine;
                for (int x = 0; x < width; ++x) {
                    reinterpret_cast<IncompleteLeaf*>(destination)->
                        init(reinterpret_cast<IncompleteLeaf*>(source));
                    source += bytes;
                    destination += bytes;
                }
                sourceLine += sourceStride;
                destinationLine += destinationStride;
            }
            return;
        }
        int bytes = _grid->_gridType.bytesPerBlock() << _logBlocksPerGroup;
        if (blockType() == gridBlockType) {
            GridBlock* sourceLine = source.gridBlock();
            GridBlock* destinationLine = gridBlock();
            for (int y = 0; y < width; ++y) {
                memcpy(destinationLine, sourceLine, bytes);
                GridBlock* destination = destinationLine;
                for (int x = 0; x < width; ++x) {
                    (destination++)->_grid->reparent(*this);
                    ++_position.x;
                }
                _position.x -= width;
                ++_position.y;
                sourceLine += sourceStride;
                destinationLine += destinationStride;
            }
            _position.y -= width;
            return;
        }
        CompleteLeaf* sourceLine = source.completeLeaf();
        CompleteLeaf* destinationLine = completeLeaf();
        for (int y = 0; y < width; ++y) {
            memcpy(destinationLine, sourceLine, bytes);
            sourceLine += sourceStride;
            destinationLine += destinationStride;
        }
    }

    int _logBlocksPerGroup;
};


// This class encapsulates a BlockGroup and a BlockLocation.
template<class FractalProcessor> class Site
  : public BlockGroup<FractalProcessor>
{
public:
    Vector& topLeft() { return _location._topLeft; }

    bool moveLeft()
    {
        Vector point = topLeft() - Vector(1, 0);
        if (point.x >= 0) {
            moveTo(point);
            return true;
        }
        return false;
    }

    bool moveRight()
    {
        Vector point = topLeft() + Vector(pointsPerGroup(), 0);
        if (point.x < 0x40000000) {
            moveTo(point);
            return true;
        }
        return false;
    }

    bool moveUp()
    {
        Vector point = topLeft() - Vector(0, 1);
        if (point.y >= 0) {
            moveTo(point);
            return true;
        }
        return false;
    }

    bool moveDown()
    {
        Vector point = topLeft() + Vector(0, pointsPerGroup());
        if (point.y < 0x40000000) {
            moveTo(point);
            return true;
        }
        return false;
    }

    int logPointsPerGroup() const { return _location._logPoints; }
    int& logPointsPerGroup() { return _location._logPoints; }
    int pointsPerGroup() const { return 1 << logPointsPerGroup(); }
protected:
    Site(Grid* grid = 0)
      : BlockGroup(grid)
    { }

    void moveNext()
    {
        int blocks = blocksPerGroup();
        int points = pointsPerGroup();
        _position.x += blocks;
        topLeft().x += points;
        if (_position.x == blocksPerGrid()) {
            _position.x = 0;
            _position.y += blocks;
            topLeft() += Vector(
                -(points << (logBlocksPerGrid() - _logBlocksPerGroup)),
                points);
        }
    }

    void moveNextQuadrant()
    {
        int blocks = blocksPerGroup();
        int points = pointsPerGroup();
        if (((_position.x >> _logBlocksPerGroup) & 1) == 0) {
            // Move right
            _position.x += blocks;
            topLeft().x += points;
        }
        else {
            // Move left and down
            _position += Vector(-blocks, blocks);
            topLeft() += Vector(-points, points);
        }
    }

    void ascend()
    {
        topLeft() -= _position << (logPointsPerGroup() - _logBlocksPerGroup);
        logPointsPerGroup() += logBlocksPerGrid() - _logBlocksPerGroup;
        BlockGroup::ascend();
    }

    void descend(Vector position = Vector(0, 0))
    {
        logPointsPerGroup() -=
            _logBlocksPerGroup + subGrid()->_gridType._logBlocks;
        topLeft() += position << logPointsPerGroup();
        BlockGroup::descend(position);
    }

    void ascendQuadrant()
    {
        if (_logBlocksPerGroup >= logBlocksPerGrid() - 1)
            ascend();
        else {
            ++_logBlocksPerGroup;
            ++logPointsPerGroup();
            _position &= -blocksPerGroup();
            topLeft() &= -pointsPerGroup();
        }
    }

    void descendQuadrant()
    {
        if (_logBlocksPerGroup == 0) {
            descend();
            _logBlocksPerGroup = logBlocksPerGrid() - 1;
            if (_logBlocksPerGroup < 0)
                _logBlocksPerGroup = 0;
            logPointsPerGroup() += _logBlocksPerGroup;
        }
        else {
            --_logBlocksPerGroup;
            --logPointsPerGroup();
        }
    }

    // Descend to the leaf block containing point "point".
    void descendToLeaf(Vector point)
    {
        while (blockType() == gridBlockType)
            descend((point - topLeft()) >>
                (logPointsPerGroup() - subGrid()->_gridType._logBlocks));
    }

    void moveTo(Vector point)
    {
        do {
            if (_grid->_parent == 0) {
                _position = point >> logPointsPerGroup();
                topLeft() = point & -pointsPerGroup();
                break;
            }
            else
                ascend();
        } while (!_location.contains(point));
        descendToLeaf(point);
    }

    void moveTo(BlockLocation location)
    {
        moveTo(location._topLeft);
        while (logPointsPerGroup() < location._logPoints)
            ascendQuadrant();
    }

    bool detail()
    {
        if (!isComplete())
            return false;
        unsigned int c = colour();
        Site r = *this;
        if (!r.moveRight() || (r.isComplete() && r.colour() != c))
            return true;
        Site d = *this;
        if (!d.moveDown() || (d.isComplete() && d.colour() != c))
            return true;
        return false;
    }

    void setLogBlocksPerGroup(int logBlocksPerGroup)
    {
        logPointsPerGroup() += logBlocksPerGroup - _logBlocksPerGroup;
        _logBlocksPerGroup = logBlocksPerGroup;
    }

    void setPosition(Vector position)
    {
        topLeft() += (position - _position) <<
            (logPointsPerGroup() - _logBlocksPerGroup);
        _position = position;
    }

    BlockLocation _location;
};


// This class encapsulates local transformations on the matrix and other
// operations requiring the FractalProcessor.
template<class FractalProcessor> class Manipulator
  : public Site<FractalProcessor>
{
public:
    void plot(unsigned int newColour)
    {
        if (newColour == colour())
            return;
        colour() = newColour;
        _processor->screen()->checkedPlot(_location, newColour);
    }

    void complete(unsigned int newColour)
    {
        plot(newColour);

        // In order to change the type of a block, it needs to be in a 1x1
        // grid.
        while (logBlocksPerGrid() > 0)
            split();

        // Now we have a 1x1 grid, the change is easy - just create a new grid
        // for the new block.
        ascend();
        deleteChildGrid();
        createGrid(0, completeLeafType, precision());
        descend();

        colour() = newColour;
        tryConsolidateGrid();

        int logPointsPerTexel = _processor->screen()->logPointsPerTexel();
        if (visible() && detail() && logPointsPerGroup() > logPointsPerTexel) {
            splitLeaf();

            // See if the up and left leaves can be split.
            Vector point = topLeft();
            if (moveLeft()) {
                if (logPointsPerGroup() > logPointsPerTexel && visible() &&
                    detail())
                    splitLeaf();
                moveTo(point);
            }
            if (moveUp()) {
                if (logPointsPerGroup() > logPointsPerTexel && visible() &&
                    detail())
                    splitLeaf();
            }
        }
    }

    bool visible() const
    {
        return _processor->screen()->blockVisible(_location);
    }

    // Called by Screen to plot a tile, and recursively to plot part of a tile.
    // Always acts on the entire grid, but assumes _logBlocksPerGroup == 0.
    void plot(BlockLocation location)
    {
        int l = location._logPoints - logPointsPerGroup();

        // Move to the top-left of the blocks of interest.
        Vector position =
            (location._topLeft - topLeft()) >> logPointsPerGroup();

        if (l <= 0) {
            setPosition(position);
            // We're plotting a single block or part of one.
            if (blockType() == gridBlockType)
                child().plot(location);
            else
                _processor->screen()->plotBlock(location, colour());
            return;
        }
        // We're plotting multiple subblocks.
        int width = 1 << l;
        for (int y = 0; y < width; ++y)
            for (int x = 0; x < width; ++x) {
                setPosition(position | Vector(x, y));
                if (blockType() == gridBlockType)
                    child().plot(_location);
                else
                    _processor->screen()->plotBlock(_location, colour());
            }
    }

    // Create a new grid in this gridblock.
    Grid* createGrid(int logBlocks, BlockType blockType, int precision)
    {
        GridType gridType(logBlocks, blockType, precision, logPointsPerGroup()
            - logBlocks - _processor->matrix()->logPointsPerBlockBase());
        Grid* grid = _processor->tracker()->allocateGrid(gridType);
        *grid = Grid(gridType);
        grid->reparent(*this);
        if (gridType._blockType == incompleteLeafType) {
            descend();
            while (!atEnd()) {
                IncompleteLeaf* leaf = incompleteLeaf();
                leaf->_parent = grid;
                _processor->queue()->add(leaf, logPointsPerGroup());
                moveNext();
            }
            ascend();
        }
        return grid;
    }

    Manipulator child(Vector position = Vector(0, 0)) const
    {
        Manipulator child = *this;
        child.descend(position);
        return child;
    }

    // Attempt to consolidate grid with its siblings. Updates grid and position
    // so that they continue to refer to the same block.
    void tryConsolidateGrid()
    {
        Manipulator other = parent();
        if (other.atTop()) {
            // Can't consolidate the matrix itself.
            return;
        }
        if (other._grid->_parent == 0) {
            // Can't consolidate the subgrids of the matrix.
            return;
        }

        // In order to consolidate, this block's grid must be the same size and
        // type as its sibling grids.

        GridType gridType = _grid->_gridType;
        other.ascendQuadrant();
        other.descendQuadrant();
        for (int i = 0; i < 4; ++i) {
            Grid* quarter = other.subGrid();
            if (quarter->_gridType != gridType)
                return;
            other.moveNextQuadrant();
        }
        int logBlocks = logBlocksPerGrid();
        BlockType type = blockType();
        int p = precision();
        Vector newPosition = _position |
            ((parent()._position & 1) << logBlocks);
        int oldLogBlocks = _logBlocksPerGroup;
        ascend();
        ascendQuadrant();
        descendQuadrant();

        // Consolidation will happen. Make the parent grid 2x2.
        while (logBlocksPerGrid() > 1)
            split();

        // Create new grid and update position
        other = *this;
        other._grid = parent().createGrid(logBlocks + 1, type, p);
        // Zero the old grid's parent pointer, so the new grid pointer won't
        // get overwritten if the old grid moves during the following deletes.
        _grid->_parent = 0;
        _grid->_index = logPointsPerGroup();
        other._position = Vector(0, 0);
        other._logBlocksPerGroup = logBlocks;
        other.topLeft() = topLeft();
        other.logPointsPerGroup() = logPointsPerGroup();

        // Move the data and delete the old grids
        for (int i = 0; i < 4; ++i) {
            descend();
            setLogBlocksPerGroup(logBlocks);
            other.moveBlocks(*this);
            ascend();
            other.moveNext();
            deleteChildGrid();
            moveNext();
        }

        // Swap this with the other and then delete the other to ensure our
        // grid pointer is updated.
        Manipulator other2 = *this;
        *this = other;
        other2.deleteGrid();

        setLogBlocksPerGroup(oldLogBlocks);
        setPosition(newPosition);

        tryConsolidateGrid();
    }

    // Deletes the grid in this gridBlock, and all sub-grids of that grid.
    void recursiveDeleteChildGrid()
    {
        int i = 0;
        do {
            if (subGrid()->_gridType._blockType == gridBlockType)
                descend();
            else
                do {
                    deleteChildGrid();
                    if (i == 0)
                        return;
                    --i;
                    moveNext();
                    if (!atEnd())
                        break;
                    ascend();
                } while (true);
            ++i;
        } while (true);
    }

    void deleteChildGrid()
    {
        // We need to clear the pointer in the grid before we delete, or we
        // might move this block while it's in a non-valid state. But we need
        // to save the _grid pointer first so we know what to delete. Hence
        // this slightly roundabout way of doing things.
        Manipulator c = child();
        subGrid() = 0;
        c.deleteGrid();
    }

    // Deletes the entire grid that this is the top-left block of.
    void deleteGrid()
    {
        if (blockType() == incompleteLeafType)
            while (!atEnd()) {
                _processor->queue()->remove(incompleteLeaf(),
                    _grid->logPointsPerBlock(_processor->matrix())/* logPointsPerGroup()*/);
                moveNext();
            }
        _processor->tracker()->deallocateGrid(_grid);
    }

    // Make sure none of the adjacent blocks are too small to consolidate.
    bool canConsolidateLeaf()
    {
        int logPointsPerLeaf = logPointsPerGroup() - 1;
        int pointsPerTwoLeaves = pointsPerGroup();
        int pointsPerLeaf = pointsPerTwoLeaves >> 1;
        Vector point = topLeft();
        if (point.x > 0) {
            moveTo(point + Vector(-1, 0));
            if (logPointsPerGroup() < logPointsPerLeaf)
                return false;
            moveTo(point + Vector(-1, pointsPerLeaf));
            if (logPointsPerGroup() < logPointsPerLeaf)
                return false;
        }
        if (point.y > 0) {
            moveTo(point + Vector(0, -1));
            if (logPointsPerGroup() < logPointsPerLeaf)
                return false;
            moveTo(point + Vector(pointsPerLeaf, -1));
            if (logPointsPerGroup() < logPointsPerLeaf)
                return false;
        }
        if (point.x < 0x40000000 - pointsPerTwoLeaves) {
            moveTo(point + Vector(pointsPerTwoLeaves, 0));
            if (logPointsPerGroup() < logPointsPerLeaf)
                return false;
            moveTo(point + Vector(pointsPerTwoLeaves, pointsPerLeaf));
            if (logPointsPerGroup() < logPointsPerLeaf)
                return false;
        }
        if (point.y < 0x40000000 - pointsPerTwoLeaves) {
            moveTo(point + Vector(0, pointsPerTwoLeaves));
            if (logPointsPerGroup() < logPointsPerLeaf)
                return false;
            moveTo(point + Vector(pointsPerLeaf, pointsPerTwoLeaves));
            if (logPointsPerGroup() < logPointsPerLeaf)
                return false;
        }
        return true;
    }

    bool isDeleted() const { return _isDeleted; }

protected:
    Manipulator()
      : _processor(0)
    { }

    void moveToLeaf(IncompleteLeaf* leaf)
    {
        BlockPointer pointer = leaf->blockPointer();
        *this =
            Manipulator(_processor, pointer._grid).child(pointer._position);
    }

    Manipulator parent() const
    {
        Manipulator parent = *this;
        parent.ascend();
        return parent;
    }

    // Splits this leaf block into 4. The top left new leaf will be the same
    // as the original leaf and the other 3 will be incomplete leaves. Leaves
    // the address pointing to the same (now smaller) leaf. Splits adjacent
    // leaves as necessary to maintain the adjacency invariant.
    void splitLeaf()
    {
        Manipulator l = *this;
        Vector point = topLeft();
        if (l.moveLeft() && l.logPointsPerGroup() > logPointsPerGroup()) {
            *this = l;
            splitLeaf();
            moveTo(point);
        }
        Manipulator r = *this;
        if (r.moveRight() && r.logPointsPerGroup() > logPointsPerGroup()) {
            *this = r;
            splitLeaf();
            moveTo(point);
        }
        Manipulator u = *this;
        if (u.moveUp() && u.logPointsPerGroup() > logPointsPerGroup()) {
            *this = u;
            splitLeaf();
            moveTo(point);
        }
        Manipulator d = *this;
        if (d.moveDown() && d.logPointsPerGroup() > logPointsPerGroup()) {
            *this = d;
            splitLeaf();
            moveTo(point);
        }
        uncheckedSplitLeaf();
    }

    int screenPrecision(int logLeavesPerGroup)
    {
        return _processor->screen()->
            precisionForLeaf(logPointsPerGroup() - logLeavesPerGroup) - 1;
    }

    void uncheckedSplitLeaf()
    {
        // In order to split a block, it needs to be in a 1x1
        // grid.
        while (logBlocksPerGrid() > 0)
            split();

        Manipulator dest;
        unsigned int colour = leaf()->_colour;

        if (blockType() == incompleteLeafType) {
            Grid* quad = parent().createGrid(
                1, incompleteLeafType, screenPrecision(1));
            dest = parent().child();
            dest.moveBlocks(*this);
            dest.moveNext();
            for (int i = 1; i < 4; ++i) {
                dest.incompleteLeaf()->init(colour);
                dest.moveNext();
            }
        }
        else {
            int newPrecision = screenPrecision(1);
            if (newPrecision > precision()) {
                Grid* quad = parent().createGrid(
                    1, incompleteLeafType, screenPrecision(1));
                dest = parent().child();
                for (int i = 0; i < 4; ++i) {
                    dest.incompleteLeaf()->init(colour);
                    dest.moveNext();
                }
            }
            else {
                Grid* quad = parent().createGrid(1, gridBlockType, 0);
                dest = parent().child();
                dest.createGrid(0, completeLeafType, newPrecision);
                dest.descend();
                dest.completeLeaf()->_colour = colour;
                dest.ascend();
                dest.moveNext();
                for (int i = 1; i < 4; ++i)
                    dest.createLeaf(colour);
            }
        }
        dest.setPosition(Vector(0, 0));
        Manipulator other = *this;
        *this = dest;
        other.deleteGrid();

        tryConsolidateGrid();

        _processor->leavesInQueue();
    }

    // Consolidates this leaf block with its siblings.
    void consolidateLeaf()
    {
        // A 1x1 grid is handled by consolidating its parent, which will be
        // at least 2x2.
        if (logBlocksPerGrid() == 0)
            ascend();

        // In order to consolidate, need to reduce the grid to 2x2.
        while (logBlocksPerGrid() > 1)
            split();

        BlockType type = blockType();
        setPosition(Vector(0, 0));

        Grid* newGrid;
        if (type == gridBlockType) {
            // We are a grid of (1x1) grids - we can just reparent.
            for (int i = 1; i < 4; ++i) {
                moveNext();
                recursiveDeleteChildGrid();
            }
            setPosition(Vector(0, 0));
            newGrid = child()._grid;
            newGrid->reparent(parent());
            if (newGrid->_gridType._blockType == incompleteLeafType) {
                // We're changing the size of this leaf, so we need to move it
                // into a different queue.
                IncompleteLeaf* leaf = static_cast<IncompleteLeaf*>(
                    newGrid->blockAtPosition(Vector(0, 0)));
                WorkQueueList<FractalProcessor>* queue = _processor->queue();
                queue->remove(leaf, logPointsPerGroup());
                queue->add(leaf, logPointsPerGroup() + 1);
                ++newGrid->_gridType._logPointsPerBlockOffset;
            }
        }
        else {
            newGrid = parent().createGrid(0, type, screenPrecision(-1));
            parent().child().moveBlocks(*this);
        }
        // This should not invalidate newGrid, because newGrid should be 1x1
        // and we're deleting a 2x2 grid.
        deleteGrid();
        _grid = newGrid;
        setPosition(Vector(0, 0));
        ++logPointsPerGroup();
        tryConsolidateGrid();
    }

    // Creates and initializes a 1x1 incompleteLeaf grid. Used by grow(),
    // initMatrix() and splitLeaf().
    void createLeaf(int colour = 0)
    {
        createGrid(0, incompleteLeafType, screenPrecision(0));
        child().incompleteLeaf()->init(colour);
        moveNext();
    }

    FractalProcessor* _processor;

    // Split grid into 4 and updates grid and position so that they continue to
    // refer to the same block.
    void split()
    {
        Vector originalTopLeft = topLeft();
        Vector oldPosition = _position;
        BlockLocation oldLocation = _location;
        int oldLogBlocks = _logBlocksPerGroup;

        // Create new grid and move data into it.
        int logBlocks = logBlocksPerGrid() - 1;
        int p = precision();
        Grid* quad = parent().createGrid(1, gridBlockType, p);

        Manipulator dest = parent().child();
        BlockType type = blockType();
        setPosition(Vector(0, 0));
        setLogBlocksPerGroup(logBlocks);
        for (int i = 0; i < 4; ++i) {
            dest.createGrid(logBlocks, type, p);
            dest.descend();
            dest.setLogBlocksPerGroup(logBlocks);
            dest.moveBlocks(*this);
            dest.ascend();
            dest.moveNext();
            moveNext();
        }

        // Save this grid for later deletion.
        Manipulator other = *this;

        // Adjust grid and position.
        _grid = BlockPointer(quad, oldPosition >> logBlocks).subGrid();
        oldPosition &= ((1 << logBlocks) - 1);
        _position = oldPosition;
        _logBlocksPerGroup = oldLogBlocks;
        _location = oldLocation;

        // Delete the previous grid
        other.setPosition(Vector(0, 0));
        other.setLogBlocksPerGroup(0);
        other.deleteGrid();

        ascend();
        tryConsolidateGrid();
        descend();

        setPosition(oldPosition);
        setLogBlocksPerGroup(oldLogBlocks);
    }

private:
    // Manipulator for the GridBlock that holds Grid g.
    Manipulator(FractalProcessor* processor, Grid* g)
      : _processor(processor)
    {
        if (g->_parent == 0) {
            _location = BlockLocation(Vector(0, 0), 30);
            _isDeleted = (g == processor->matrix()->deleted());
        }
        else {
            BlockPointer blockPointer = g->blockPointer();
            _grid = blockPointer._grid;
            _position = blockPointer._position;
            if (((_grid->_gridType._logPointsPerBlockOffset -
                g->_gridType._logPointsPerBlockOffset)&0x3f) ==
                g->_gridType._logBlocks) {
                Manipulator parent = Manipulator(processor, _grid);
                _location =
                    parent._location.child(_position, logBlocksPerGrid());
                _isDeleted = parent._isDeleted;
            }
            else
                _isDeleted = true;
        }
    }

    bool _isDeleted;
};


// This class holds the root grid pointer and encapsulates global matrix
// operations.
template<class FractalProcessor> class Matrix : Manipulator<FractalProcessor>
{
public:
    Matrix()
      : _root(0),
        _interrupt(false),
        _splitLargeLeavesLocation(Vector(0, 0), 29),
        _splitLargeLeavesPending(false),
        _consolidateSmallLeavesPending(false),
        _consolidateOffScreenLeavesPending(false)
    { }

    ~Matrix()
    {
        reset();
        if (_root != 0) {
            while (!atEnd()) {
                recursiveDeleteChildGrid();
                moveNext();
            }
            deleteGrid();
        }
    }

    // Manipulator for a particular IncompleteLeaf.
    Manipulator* manipulator(IncompleteLeaf* leaf)
    {
        reset();
        moveToLeaf(leaf);
        return this;
    }

    // Manipulator for a particular point.
    Manipulator* manipulator(Vector topLeft)
    {
        reset();
        moveTo(topLeft);
        return this;
    }

    // Called by Screen to plot a tile.
    void plot(BlockLocation location)
    {
        reset();
        Manipulator::plot(location);
    }

    // Splits leaves larger than a tile.
    void splitMultiTileLeaves()
    {
        reset();
        int maxLogPoints = _processor->screen()->logPointsPerTile();
        do {
            if (blockType() == gridBlockType && _logBlocksPerGroup == 0) {
                descendQuadrant();
                if (logPointsPerGroup() - _logBlocksPerGroup > maxLogPoints)
                    continue;
                // Ascend
            }
            else
                if (_logBlocksPerGroup > 0) {
                    descendQuadrant();
                    continue;
                }
                else {
                    uncheckedSplitLeaf();  // Don't need to split adjacents.
                    if (logPointsPerGroup() > maxLogPoints)
                        continue;
                    ascendQuadrant();
                    if (!atLastQuadrant()) {
                        moveNextQuadrant();
                        continue;
                    }
                    // Ascend
                }
            do {
                ascendQuadrant();
                if (atTop())
                    return;
                if (atLastQuadrant())
                    continue;
                moveNextQuadrant();
                break;
            } while (true);
        } while (true);
    }

    // Creates the matrix and returns its 2x2 grid.
    void setProcessor(FractalProcessor* processor)
    {
        _deleted = 0;
        _logPointsPerBlockBase = 0;
        _processor = processor;
        _position = Vector(0, 0);
        _logBlocksPerGroup = 0;
        topLeft() = Vector(0, 0);
        logPointsPerGroup() = 29;
        _root = createGrid(1, gridBlockType, 1);
        _grid = _root;
        while (!atEnd())
            createLeaf();
        splitMultiTileLeaves();
    }

    // Enlarge the matrix when zooming out. The original matrix will be in the
    // "quadrant" quadrant of the new matrix.
    void grow(Vector quadrant)
    {
        --_logPointsPerBlockBase;
        reset();
        _root = parent().createGrid(1, gridBlockType, 0);
        _grid->reparent(BlockPointer(_root, quadrant));
        reset();

        // Create some new 1x1 IncompleteLeaf blocks for the other 3
        // quadrants.
        for (int i = 0; i < 4; ++i)
            if (_position != quadrant)
                createLeaf();
            else
                moveNext();
        setPosition(quadrant);

        child().child().tryConsolidateGrid();

        splitMultiTileLeaves();
        reset();

        // Split up the new leaves to maintain the matrix invariant that says
        // no more than 2 leaves can be adjacent to another leaf-side.
        switch (quadrant.x | (quadrant.y << 1)) {
            case 0:
                moveTo(Vector(0x20000000, 0));
                do {
                    Manipulator l = *this;
                    l.moveLeft();
                    int lLogPointsPerGroup = l.logPointsPerGroup();
                    while (lLogPointsPerGroup < logPointsPerGroup() - 1)
                        splitLeaf();
                    moveDown();
                } while (topLeft().y < 0x20000000);
                moveTo(Vector(0, 0x20000000));
                do {
                    Manipulator u = *this;
                    u.moveUp();
                    int uLogPointsPerGroup = u.logPointsPerGroup();
                    while (uLogPointsPerGroup < logPointsPerGroup() - 1)
                        splitLeaf();
                    moveRight();
                } while (topLeft().x < 0x20000000);
                break;
            case 1:
                moveTo(Vector(0x1fffffff, 0));
                do {
                    Manipulator r = *this;
                    r.moveRight();
                    int rLogPointsPerGroup = r.logPointsPerGroup();
                    while (rLogPointsPerGroup < logPointsPerGroup() - 1) {
                        splitLeaf();
                        moveRight();
                    }
                    moveDown();
                } while (topLeft().y < 0x20000000);
                moveTo(Vector(0x1fffffff, 0x20000000));
                do {
                    moveRight();
                    Manipulator u = *this;
                    u.moveUp();
                    int uLogPointsPerGroup = u.logPointsPerGroup();
                    while (uLogPointsPerGroup < logPointsPerGroup() - 1)
                        splitLeaf();
                } while (topLeft().x + pointsPerGroup() < 0x40000000);
                break;
            case 2:
                moveTo(Vector(0x20000000, 0x1fffffff));
                do {
                    moveDown();
                    Manipulator l = *this;
                    l.moveLeft();
                    int lLogPointsPerGroup = l.logPointsPerGroup();
                    while (lLogPointsPerGroup < logPointsPerGroup() - 1)
                        splitLeaf();
                } while (topLeft().y + pointsPerGroup() < 0x40000000);
                moveTo(Vector(0, 0x1fffffff));
                do {
                    Manipulator d = *this;
                    d.moveDown();
                    int dLogPointsPerGroup = d.logPointsPerGroup();
                    while (dLogPointsPerGroup < logPointsPerGroup() - 1) {
                        splitLeaf();
                        moveDown();
                    }
                    moveRight();
                } while (topLeft().x < 0x20000000);
                break;
            case 3:
                moveTo(Vector(0x1fffffff, 0x1fffffff));
                do {
                    moveDown();
                    Manipulator r = *this;
                    r.moveRight();
                    int rLogPointsPerGroup = r.logPointsPerGroup();
                    while (rLogPointsPerGroup < logPointsPerGroup() - 1) {
                        splitLeaf();
                        moveRight();
                    }
                } while (topLeft().y + pointsPerGroup() < 0x40000000);
                moveTo(Vector(0x1fffffff, 0x1fffffff));
                do {
                    moveRight();
                    Manipulator d = *this;
                    d.moveDown();
                    int dLogPointsPerGroup = d.logPointsPerGroup();
                    while (dLogPointsPerGroup < logPointsPerGroup() - 1) {
                        splitLeaf();
                        moveDown();
                    }
                } while (topLeft().x + pointsPerGroup() < 0x40000000);
                break;
        }

        // Adjust the interrupted locations.
        quadrant <<= 29;
        --_splitLargeLeavesLocation._logPoints;
        _splitLargeLeavesLocation._topLeft =
            (_splitLargeLeavesLocation._topLeft >> 1) | quadrant;
        --_consolidateSmallLeavesLocation._logPoints;
        _consolidateSmallLeavesLocation._topLeft =
            (_consolidateSmallLeavesLocation._topLeft >> 1) | quadrant;
        --_consolidateOffScreenLeavesLocation._logPoints;
        _consolidateOffScreenLeavesLocation._topLeft =
            (_consolidateOffScreenLeavesLocation._topLeft >> 1) | quadrant;
    }

    // Shrink the matrix when zooming in. semiQuadrant is the top left of the
    // new matrix in units of 1/4 of the original matrix width. The new matrix
    // will have 1/4 of the area of the original matrix.
    void shrink(Vector semiQuadrant)
    {
        reset();
        // First make sure each subgrid of the grid is a 2x2 gridBlock grid.
        for (int i = 0; i < 4; ++i) {
            descend();
            // If it's a 1x1 leaf grid, split it up.
            if (logBlocksPerGrid() == 0)
                splitLeaf();
            // If it's not a 2x2 grid of grids, split it up.
            if (blockType() != gridBlockType || logBlocksPerGrid() != 1) {
                split();
                ascend();
            }
            ascend();
            moveNext();
        }

        // Pick the 1/16th grids we need.
        Grid** newQuadrants[4];
        setPosition(Vector(0, 0));
        for (int i = 0; i < 4; ++i) {
            descend();
            for (int j = 0; j < 4; ++j) {
                Vector p((j&1) | ((i&1)<<1), (j>>1) | (i&2));
                p -= semiQuadrant;
                if (p.inside(Vector(2, 2)))
                    newQuadrants[p.x | (p.y<<1)] = &subGrid();
                moveNext();
            }
            ascend();
            moveNext();
        }
        // Swap these grids with the ones in the top-left quadrant
        setPosition(Vector(0, 0));
        descend();
        for (int i = 0; i < 4; ++i) {
            // Swap the grids
            Grid* g1 = subGrid();
            Grid* g2 = *newQuadrants[i];
            subGrid() = g2;
            *newQuadrants[i] = g1;

            // Swap their parents
            Grid* parent = g1->_parent;
            g1->_parent = g2->_parent;
            g2->_parent = parent;

            // Swap their indexes
            int index = g1->_index;
            g1->_index = g2->_index;
            g2->_index = index;

            moveNext();
        }
        ascend();

        // Delete the other 3 quadrants
        for (int i = 1; i < 4; ++i) {
            moveNext();
            moveToDeleted();
        }

        // Make the top-left quadrant the root
        setPosition(Vector(0, 0));
        descend();
        _root = _grid;

        Manipulator p = parent();

        // Fix up the new root's parent
        _root->_parent = 0;

        // Delete the previous root
        p.deleteGrid();

        Vector oldMatrixTopLeft = semiQuadrant << 28;
        BlockLocation oldMatrix(oldMatrixTopLeft, 29);
        if (oldMatrix.contains(_splitLargeLeavesLocation)) {
            ++_splitLargeLeavesLocation._logPoints;
            _splitLargeLeavesLocation._topLeft =
                (_splitLargeLeavesLocation._topLeft - oldMatrixTopLeft) << 1;
        }
        else
            _splitLargeLeavesLocation = BlockLocation(Vector(0, 0), 29);

        if (oldMatrix.contains(_consolidateSmallLeavesLocation)) {
            ++_consolidateSmallLeavesLocation._logPoints;
            _consolidateSmallLeavesLocation._topLeft =
                (_consolidateSmallLeavesLocation._topLeft - oldMatrixTopLeft)
                << 1;
        }
        else
            _consolidateSmallLeavesLocation = BlockLocation(Vector(0, 0), 29);

        if (oldMatrix.contains(_consolidateOffScreenLeavesLocation)) {
            ++_consolidateOffScreenLeavesLocation._logPoints;
            _consolidateOffScreenLeavesLocation._topLeft =
                (_consolidateOffScreenLeavesLocation._topLeft -
                oldMatrixTopLeft) << 1;
        }
        else
            _consolidateOffScreenLeavesLocation =
                BlockLocation(Vector(0, 0), 29);

        ++_logPointsPerBlockBase;
    }

    void reseatGrid(Grid* oldGrid, Grid* newGrid)
    {
        if (_root == oldGrid)
            _root = newGrid;
        if (_grid == oldGrid)
            _grid = newGrid;
        if (_deleted == oldGrid)
            _deleted = newGrid;
    }

    bool resume()
    {
        if (_consolidateSmallLeavesPending) {
            _consolidateSmallLeavesPending = false;
            consolidateSmallLeaves();
            if (_consolidateSmallLeavesPending)
                return false;
        }
        if (_deleted != 0) {
            finalDelete();
            if (_deleted != 0)
                return false;
        }
        if (_consolidateOffScreenLeavesPending) {
            _consolidateOffScreenLeavesPending = false;
            consolidateOffScreenLeaves();
            if (_consolidateOffScreenLeavesPending)
                return false;
        }
        if (_splitLargeLeavesPending) {
            _splitLargeLeavesPending = false;
            splitLargeLeaves();
            if (_splitLargeLeavesPending)
                return false;
        }
        return true;
    }

    void setInterrupt() { _interrupt = true; }
    void clearInterrupt() { _interrupt = false; }

    void scheduleConsolidateSmallLeaves()
    {
        _consolidateSmallLeavesPending = true;
        _consolidateSmallLeavesLocation = BlockLocation(Vector(0, 0), 29);
        _consolidatedSmallLeaves = false;

    }
    void scheduleConsolidateOffScreenLeaves()
    {
        _consolidateOffScreenLeavesPending = true;
        _consolidateOffScreenLeavesLocation = BlockLocation(Vector(0, 0), 29);
        _consolidatedOffScreenLeaves = false;
    }

    void scheduleSplitLargeLeaves() { _splitLargeLeavesPending = true; }

    int logPointsPerBlockBase() { return _logPointsPerBlockBase; }

    Grid* deleted() const { return _deleted; }

private:
    void splitLargeLeaves()
    {
        reset();
        moveTo(_splitLargeLeavesLocation);
        int maxLogPoints = _processor->screen()->logPointsPerTexel();
        do {
            if (_interrupt) {
                _splitLargeLeavesPending = true;
                _splitLargeLeavesLocation = _location;
                return;
            }
            if (!visible()) {
                if (!atLastQuadrant()) {
                    moveNextQuadrant();
                    continue;
                }
                // Ascend
            }
            else
                if (blockType() == gridBlockType && _logBlocksPerGroup == 0) {
                    descendQuadrant();
                    if (blockType() != incompleteLeafType &&
                        logPointsPerGroup() - _logBlocksPerGroup >
                            maxLogPoints)
                        continue;
                    // Ascend
                }
                else
                    if (_logBlocksPerGroup > 0) {
                        descendQuadrant();
                        continue;
                    }
                    else {
                        if (detail()) {
                            if (!_processor->tracker()->canAllocate()) {
                                // We're low on memory - do some more iterating
                                // before splitting any more.
                                _splitLargeLeavesPending = true;
                                _splitLargeLeavesLocation = _location;
                                return;
                            }
                            if (logPointsPerGroup() - 1 > maxLogPoints)
                                maxLogPoints = logPointsPerGroup() - 1;
                            splitLeaf();
                            continue;
                        }
                        else {
                            if (!atLastQuadrant()) {
                                moveNextQuadrant();
                                continue;
                            }
                            // Ascend
                        }
                    }
            do {
                ascendQuadrant();
                if (atTop()) {
                    _splitLargeLeavesLocation =
                        BlockLocation(Vector(0, 0), 29);
                    return;
                }
                if (atLastQuadrant())
                    continue;
                moveNextQuadrant();
                break;
            } while (true);
        } while (true);
    }

    // Consolidate leaves smaller than a texel.
    void consolidateSmallLeaves()
    {
        reset();
        moveTo(_consolidateSmallLeavesLocation);
        int logPointsPerTexel = _processor->screen()->logPointsPerTexel();
start:
        if (_interrupt) {
            _consolidateSmallLeavesPending = true;
            _consolidateSmallLeavesLocation = _location;
            return;
        }
        if (_logBlocksPerGroup > 1)
            goto descend;
        if (_logBlocksPerGroup == 0) {
            if (blockType() != gridBlockType)
                goto next;
            Grid* sub = subGrid();
            if (sub->_gridType._logBlocks > 1)
                goto descend;
            if (sub->_gridType._logBlocks == 0)
                goto next;
            if (sub->_gridType._blockType == gridBlockType)
                for (int i = 0; i < 4; ++i) {
                    GridType type = static_cast<GridBlock*>(
                        sub->blockAtPosition(Vector(i&1, i>>1)))->_grid
                        ->_gridType;
                    if (type._logBlocks > 0)
                        goto descend;
                }
            goto tryConsolidate;
        }
        if (blockType() == gridBlockType)
            for (int i = 0; i < 4; ++i) {
                GridType type = static_cast<GridBlock*>(_grid->blockAtPosition(
                    _position | Vector(i&1, i>>1)))->_grid->_gridType;
                if (type._logBlocks > 0)
                    goto descend;
            }
tryConsolidate:
        if (logPointsPerGroup() >= logPointsPerTexel)
            goto next;
        {
            Manipulator probe = *this;
            if (!probe.canConsolidateLeaf())
                goto next;
        }
        descendQuadrant();
        if (blockType() == gridBlockType)
            descend();
        consolidateLeaf();
        ascendQuadrant();
        goto start;
next:
        if (atLastQuadrant())
            goto ascend;
        moveNextQuadrant();
        goto start;

descend:
        descendQuadrant();
        goto start;
ascend:
        ascendQuadrant();
        if (!atTop())
            goto next;
        if (_consolidatedSmallLeaves) {
            // Consolidating may have unblocked another consolidation. Keep
            // going until there's no more to do.
            _consolidatedSmallLeaves = false;
            reset();
            moveTo(BlockLocation(Vector(0, 0), 29));
            goto start;
        }
    }

    void consolidateOffScreenLeaves()
    {
        reset();
        moveTo(_consolidateOffScreenLeavesLocation);
        int logPointsPerTile = _processor->screen()->logPointsPerTile();
start:
        if (_interrupt) {
            _consolidateOffScreenLeavesPending = true;
            _consolidateOffScreenLeavesLocation = _location;
            return;
        }
        if (_processor->screen()->blockEntirelyVisible(_location))
            goto next;
        if (_logBlocksPerGroup > 1)
            goto descend;
        if (_logBlocksPerGroup == 0) {
            if (blockType() != gridBlockType)
                goto next;
            Grid* sub = subGrid();
            if (sub->_gridType._logBlocks > 1)
                goto descend;
            if (sub->_gridType._logBlocks == 0)
                goto next;
            if (sub->_gridType._blockType == gridBlockType)
                for (int i = 0; i < 4; ++i) {
                    GridType type = static_cast<GridBlock*>(
                        sub->blockAtPosition(Vector(i&1, i>>1)))->_grid
                        ->_gridType;
                    if (type._logBlocks > 0)
                        goto descend;
                }
            goto tryConsolidate;
        }
        if (blockType() == gridBlockType)
            for (int i = 0; i < 4; ++i) {
                GridType type = static_cast<GridBlock*>(_grid->blockAtPosition(
                    _position | Vector(i&1, i>>1)))->_grid->_gridType;
                if (type._logBlocks > 0)
                    goto descend;
            }
tryConsolidate:
        if (visible())
            goto next;
        if (logPointsPerGroup() > logPointsPerTile)
            goto next;
        {
            Manipulator probe = *this;
            if (!probe.canConsolidateLeaf())
                goto next;
        }
        descendQuadrant();
        if (blockType() == gridBlockType)
            descend();
        consolidateLeaf();
        ascendQuadrant();
        _consolidatedOffScreenLeaves = true;
        goto start;
next:
        if (atLastQuadrant())
            goto ascend;
        moveNextQuadrant();
        goto start;

descend:
        descendQuadrant();
        goto start;
ascend:
        ascendQuadrant();
        if (!atTop())
            goto next;
        if (_consolidatedOffScreenLeaves) {
            // Consolidating may have unblocked another consolidation. Keep
            // going until there's no more to do.
            _consolidatedOffScreenLeaves = false;
            reset();
            moveTo(BlockLocation(Vector(0, 0), 29));
            goto start;
        }
    }

    // Move the child Grid to the deletion queue (or just delete it if it's not
    // a GridBlock grid).
    void moveToDeleted()
    {
        int logPointsPerBlock = logPointsPerGroup();
        BlockLocation location = _location;
        while (subGrid() != 0 &&
            subGrid()->_gridType._blockType == gridBlockType)
            descend();
        if (subGrid() != 0)
            deleteChildGrid();
        while (logPointsPerBlock != logPointsPerGroup()) {
            subGrid() = _deleted;
            if (_deleted != 0) {
                _deleted->_parent = _grid;
                _deleted->_index = _grid->_gridType.indexForPosition(_position);
            }
            _deleted = _grid;
            ascend();
            _deleted->_parent = 0;
        }
        subGrid() = 0;
    }

    void finalDelete()
    {
        while (_deleted != 0 && !_interrupt) {
            _grid = _deleted;
            _position = Vector(0, 0);
            _logBlocksPerGroup = 0;
            _deleted = subGrid();
            subGrid() = 0;
            if (_deleted != 0) {
                _deleted->_parent = 0;
                if (_deleted->_gridType._blockType != gridBlockType) {
                    deleteChildGrid();
                    _deleted = 0;
                }
            }
            while (true) {
                moveNext();
                if (atEnd())
                    break;
                moveToDeleted();
            }
            deleteGrid();
        }
    }

    // Resets the manipulator to the top-left quadrant of the matrix.
    void reset()
    {
        _grid = _root;
        _position = Vector(0, 0);
        _logBlocksPerGroup = 0;
        topLeft() = Vector(0, 0);
        logPointsPerGroup() = 29;
    }

    Grid* _root;
    Grid* _deleted;

    BlockLocation _splitLargeLeavesLocation;
    BlockLocation _consolidateSmallLeavesLocation;
    BlockLocation _consolidateOffScreenLeavesLocation;
    volatile bool _interrupt;
    bool _splitLargeLeavesPending;
    bool _consolidateSmallLeavesPending;
    bool _consolidateOffScreenLeavesPending;
    bool _consolidatedOffScreenLeaves;
    bool _consolidatedSmallLeaves;
    int _logPointsPerBlockBase;
};
