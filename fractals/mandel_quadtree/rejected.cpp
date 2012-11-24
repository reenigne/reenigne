// No longer needed now that we take the lock in Screen::update()

    // Stop all the threads so that we can safely resize.
    void interrupt()
    {
        // Stop all the threads
        for (int i=0;i<_nThreads;++i)
            _threads[i]->interrupt();
        // Wait for them all to actually stop
        for (int i=0;i<_nThreads;++i)
            if ((_runningThreads & (1<<i)) != 0) {
                _threads[i]->wait();
                _runningThreads &= ~(1<<i);
            }
    }



    void dump()
    {
        printf("Grid 0x%08x Position %i,%i size %i = Point %08x,%08x size %i\n",_grid, _position.x, _position.y, _size, topLeft().x, topLeft().y, pointSize());
    }








    Site parent()
    {
        Site p = *this;
        p.ascend();
        return p;
    }
    Vector findTopLeft()
    {
        if (_grid == 0)
            return Vector(0, 0);
        return parent().findTopLeft() + (_position << (pointSize() - _size));
    }
    void checkTopLeft()
    {
        Vector tl = findTopLeft();
        if (tl != topLeft())
            printf("topLeft incorrect, should be %08x,%08x but is %08x,%08x\n",tl.x,tl.y,topLeft().x,topLeft().y);
    }



            if (_iterations != 0x80000000) {
                Complex<double> z = 0;
                Complex<double> c = _screen->cFromPoint(point);
                for (unsigned int i=0x80000000; i<_iterations; ++i) {
                    double zr2 = z.x*z.x;
                    double zi2 = z.y*z.y;
                    z = Complex<double>(zr2 - zi2 + c.x, 2*z.x*z.y + c.y);
                    if (zr2 + zi2 > 16.0)
                        printf("Error: Bailed out unexpectedly\n");
                }
                if (z.x != leaf->_z.x || z.y != leaf->_z.y) {
                    printf("After %i iterations, leaf 0x%08x, point %08x,%08x (c = %.17lf + %.17lf i)  z = %.17lf + %.17lf i  expected %.17lf + %.17lf i\n",
                        _iterations - 0x80000000, leaf, point.x, point.y, c.x, c.y, leaf->_z.x, leaf->_z.y, z.x, z.y);
                    exit(1);
                }
            }



int incompleteListCount = 0;

    void checkForLeaks(size_t memoryUsed) const
    {
        if (_memoryUsed != memoryUsed)
            assertTrue(false, L"Memory leak");
    }

    bool operator!=(const BlockLocation& other) const { return _topLeft != other._topLeft || _size != other._size; }


//* Tune maximum iterations at once - plot a graph of miao against ips.

        IF_ERROR_THROW_DX(_device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0, 0));  // TODO: get rid of

//* Can't plot a block while it is rendering. This currently works but it is
//  abusing the Direct3D API.
//  * Need a lock for each tile.
//  * Matrix is generally modified when we're plotting (completed leaf or
//    update() level change) but not vice versa (splitting). That suggests that
//    we should acquire the matrix lock before the tile lock when we need both,
//    otherwise the tile lock will be held for longer than necessary.
//
//    * We don't need to hold the matrix lock while we're painting. Update
//      operation goes like this:
//      * Acquire matrix lock
//        * Update matrix
//        * For each tile
//          * Acquire tile lock
//            * Update tile
//          * Release tile lock
//      * Acquire tile lock
//        * Present
//      * Release tile lock
//
//    * Post-iterate operation goes like this:
//      * Acquire matrix lock
//        * Update
//        * Acquire tile lock
//          * Plot
//        * Release tile lock
//      * Release matrix lock
//
//    * Split operation goes like this:
//      * Acquire matrix lock
//        * Split
//      * Release matrix lock


                if (x==0 || y==0)
                    *reinterpret_cast<DWord*>(l) = 0xffffff;
                else




        printf("originUnit = %.17lf + %.17lf i,  unitsPerTexel = %.17lf,  matrixLevel = %i,  matrixOrigin = %i, %i\n", _originUnit.x, _originUnit.y, _unitsPerTexel, _matrixLevel, _matrixOrigin.x, _matrixOrigin.y);
    bool detail()
    {
        if (_size > 0)
            printf("Trying to get detail for a group?\n");
        unsigned int c = colour();
        Site r = *this;
        if (!r.moveRight()) {
            _processor->screen()->checkedPlot(_location, 4);
            return true;
        }
        if (r.colour() != c) {
            if (r.topLeft().x - topLeft().x != pointWidth()) {
                printf("Bad right move\n");
            }
            _processor->screen()->checkedPlot(_location, 1);
            return true;
        }
        Site d = *this;
        if (!d.moveDown()) {
            _processor->screen()->checkedPlot(_location, 64);
            return true;
        }
        if (d.colour() != c) {
            if (d.topLeft().y - topLeft().y != pointWidth()) {
                printf("Bad down move\n");
            }
            _processor->screen()->checkedPlot(_location, 16);
            return true;
        }
        _processor->screen()->checkedPlot(_location, 0);
        return false;
    }



        // Use the low-fragmentation heap
        HANDLE heapHandle = reinterpret_cast<HANDLE>(_get_heap_handle());
        HMODULE kernel32 = LoadLibrary(L"kernel32.dll");
        typedef BOOL (*HSI)(HANDLE heapHandle,
            HEAP_INFORMATION_CLASS HeapInformationClass, PVOID HeapInformation,
            SIZE_T HeapInformationLength);
        HSI heapSetInformation = reinterpret_cast<HSI>(
            GetProcAddress(kernel32, "HeapSetInformation"));
        if (heapSetInformation != 0) {
            ULONG heapCompatibilityInformation = 2;
            IF_ZERO_CHECK_THROW_LAST_ERROR(heapSetInformation(heapHandle,
                HeapCompatibilityInformation,
                &heapCompatibilityInformation,
                sizeof(heapCompatibilityInformation)));
        }






        // v1 - ascend() after deleteGrid()
        descend();
        if (blockType() == gridBlockType)
            do {
                recursiveDeleteChildGrid();
                moveNext();
            } while (!atEnd());
        deleteGrid();
        ascend();


        // v2 - descends even for non-gridBlock grids
        descend();
        if (blockType() == gridBlockType)
            do {
                recursiveDeleteChildGrid();
                moveNext();
            } while (!atEnd());
        ascend();
        child().deleteGrid();


        // v3 - keeps Grid pointers on stack
        if (subGrid()->_blockType == gridBlockType) {
            descend();
            do {
                recursiveDeleteChildGrid();
                moveNext();
            } while (!atEnd());
            ascend();
        }
        child().deleteGrid();


        // v4 - has a loop
        int i = 0;
top:
        if (subGrid()->_blockType == gridBlockType) {
            descend();
            do {
                ++i;
                goto top;
returning:
                --i;
                moveNext();
            } while (!atEnd());
            ascend();
        }
        child().deleteGrid();
        if (i == 0)
            return;
        else
            goto returning;


        // v5




    void verify(int size=30)
    {
        int n = 1 << (_size << 1);
        for (int i=0; i<n; ++i) {
            Block* block = blockAtIndex(i);
            if (((unsigned int)(block)&0x80000003) != 0)
                assertTrue(false);
            if (_blockType == gridBlockType) {
                Grid* g = static_cast<GridBlock*>(block)->_grid;
                if (g->_parent != this)
                    printf("Grid 0x%08x has incorrect parent 0x%08x, should be 0x%08x\n",g,g->_parent,this);
                if (g->_index != i)
                    printf("Grid 0x%08x has incorrect parent index %i, should be %i\n",g,g->_index,i);
                g->verify(size-_size);
            }
            else {
                unsigned int col = static_cast<Leaf*>(block)->_colour;
                if (col == 0xcdcdcdcd)
                    col = 0xcdcdcdce;
                if (_blockType == incompleteLeafType) {
                    IncompleteLeaf* p = static_cast<IncompleteLeaf*>(block);
                    if (p->_parent != this)
                        printf("Point 0x%08x has incorrect parent 0x%08x, should be 0x%08x\n",p,p->_parent,this);
                }
            }
        }
    }



      : _base(0)
                Byte* address = _allocator->findAddress(_chunkSize);
                _allocator->setNextAddress(reinterpret_cast<Byte*>(oldChunk));
    void setNextAddress(Byte* base) { _base = base; }

    Byte* findAddress(size_t nBytes)
    {
        // Look for a suitable area of address space
        Byte* p = _base;
        Byte* newBase = _base;
        do {
            MEMORY_BASIC_INFORMATION information;
            printf("Looking for memory at 0x%08x\n",p);
            int size = VirtualQuery(p, &information, sizeof(information));
            if (size == 0) {
                // Gone into kernel memory - try again from the beginning.
                p = 0;
                continue;
            }
            if (information.State == MEM_FREE) {
                printf("Found 0x%08x bytes at 0x%08x\n",information.RegionSize,information.BaseAddress);
                if (p < newBase) {
                    // We wrapped around and found a free region lower down.
                    // Make this region the base.
                    newBase = p;
                }
                if (information.RegionSize >= nBytes) {
                    // Found a suitable space.
                    _base = newBase;
                    break;
                }
            }
            size_t regionSize = align(information.RegionSize);
            if (p == newBase && information.State != MEM_FREE) {
                // newBase doesn't point to a free page - update it so we avoid
                // checking these pages again.
                newBase += regionSize;
            }
            p += regionSize;
            if (p == _base) {
                // We've gone through the entire user-mode address space
                // without finding a suitable region. Fail the allocation
                // attempt.
                _base = newBase;
                return 0;
            }
        } while (true);
        if (p == _base) {
            // Found pages at the base. Move the base to the page following
            // the area we will allocate. Otherwise leave it where it is in
            // case we want to do a smaller allocation there later.
            _base += nBytes;
        }
        return p;
    }

    // First place to look for free address space.
    Byte* _base;



    void printZoomPoint()
    {
        printf("%.17lf + %.17lf i\n",_zoomUnit.x,_zoomUnit.y);
    }

class Scene : Uncopyable
{
public:
    Scene(Device* device) : _device(device) { device->_device->BeginScene(); }
    ~Scene() { _device->_device->EndScene(); }
private:
    Device* _device;
};




//* Take advantage of symmetry? When we obtain a new block, take a look to see if
//  it's partner is available in the grid and if so use that instead (or update
//  other block when we finish).
//  * In general, the symmetric point might be in the matrix but not be a Leaf.
//    Only calculate imag>=0 part of the matrix and plot twice?
//
//* The global matrix operations (splitLargeLeaves(), consolidateSmallLeaves()
//  and consolidateOffScreenBlocks()) and the tile operations (double, halve and
//  plot) are very slow and cause noticable pauses when they run during a zoom.
//  * Need to break up these operations into chunks that can be done in between
//    updates.
//    * Need to bear in mind that this change will cause the no-sub-texel-blocks
//      invariant to be violated - will have to check for this in getNextLeaf(),
//      complete() and plot().
//  * It would also help to be able to do multiple operations at once on the
//    worker threads.
//    * At the moment one would be hard-pushed to find a machine with so many
//      cores that splitting a tile between cores would be useful, but that might
//      not be true for very much longer. Maybe cross that bridge if and when we
//      come to it.
//  * Need to split large leaves when they go from offscreen to onscreen.
//    * Do this at the same time as consolidating offscreen blocks, to avoid
//      touching the memory twice.
//      * Have three methods - one for onscreen, one for offscreen and one for
//        straddling (which is called from Matrix).
//       * Add locationVisible and locationEntirelyVisible for previous frame.
//       * Once this is done, we only need splitLargeLeaves when we zoom in past
//         a zoom level, and for recovering after failing to split due to low
//         memory.
//  * Have a matrix per Tower.
//  * Lock only the matrices that are needed for an operation.
//    * Also need a global lock for use when we zoom in or out past a level.
//  * When doing adjacency work, use attempted locks. If one of these fails, save
//    the location and abandon the entire task. Don't restart the same task on
//    the same thread before trying something else.
//  * Have tasks in the work queue as well as leaves. A task can be:
//    * Screen changed - check a tile's matrix for splittable points that have
//      come on screen.
//      * This is very tricky because the screen can change again before we've
//        finished.
//        * For tasks not yet started, replace with an updated task.
//        * For tasks in progress or interrupted, add a subsequent task.
//          * Probably need to make sure these tasks are done in the right order.
//        * Need a representation for a transformation change.
//          * Parameters used to determine visibility of a BlockLocation
//            _matrixLevel
//            _matrixOrigin
//            _texelsPerPixel
//            _originPixel
//            _rotor
//            _innerTopLeft
//            _innerBottomRight
//
//    * Consolidate small leaves
//    * Split large leaves
//    * Double a tile
//    * Halve a tile
//    * Plot a tile







// * tryConsolidateGrid() never seems to be called with _logBlocksPerGroup>0 -
//   get rid of the  code that handles this.
//   * Can't seem to see this code any more - did I remove it already?


    void iterate()
    {
        Complex<float> z(static_cast<float>(_z.x),static_cast<float>(_z.y));
        Complex<float> zS(static_cast<float>(_zS.x),static_cast<float>(_zS.y));
        Complex<float> c(static_cast<float>(_c.x),static_cast<float>(_c.y));
        int maximumIterations = _maximumIterations;
        float delta2 = static_cast<float>(_delta2);
        float bailoutRadius2 = static_cast<float>(_bailoutRadius2);

        for (int i = 0; i < maximumIterations; i += 2) {
            float zr2 = z.x*z.x;
            float zi2 = z.y*z.y;
            z = Complex<float>(zr2 - zi2 + c.x, 2*z.x*z.y + c.y);
            if (zr2 + zi2 > bailoutRadius2) {
                _result = i + 1;
                return;
            }

            zr2 = z.x*z.x;
            zi2 = z.y*z.y;
            z = Complex<float>(zr2 - zi2 + c.x, 2*z.x*z.y + c.y);
            if (zr2 + zi2 > bailoutRadius2) {
                _result = i + 2;
                return;
            }

            zr2 = zS.x*zS.x;
            zi2 = zS.y*zS.y;
            zS = Complex<float>(zr2 - zi2 + c.x, 2*zS.x*zS.y + c.y);
            Complex<float> d = z - zS;
            if (d.modulus2() < delta2) {
                _result = -(i + 2);
                return;
            }
        }

        _z = Complex<double>(z.x, z.y);
        _zS = Complex<double>(zS.x, zS.y);
        _result = -1;
    }


// Hand-optimize the SSE2 inner loop?
//   http://www.codeproject.com/KB/recipes/fractalssse.aspx




//* Move uncheckedSplitLeaf() and consolidateBlock() to Matrix so they can access
//  the LeafBuffer there (one LeafBuffer per Matrix).
//* Move complete() and splitLeaf() there too.
//* Avoid use of Manipulator in mandel_quadtree.cpp - use Matrix instead.
//  * Matrix inherits publicly to avoid having to move too much stuff?
//
//* Encapsulate the business of copying/moving blocks somehow to make the Matrix
//  classes more general.
//  * LeafBuffer
//    * uncheckedSplitLeaf()
//    * consolidateBlock()
//  * Grid::moveFrom()? (called from allocator.h when a grid is moved in memory)
//  * BlockGroup::moveBlocks()? (called when splitting and consolidating grids)


//* Separation of responsibilities: Matrix manipulation vs actual logic.
//  * What should the API look like?
//    * A FractalMatrix object with:
//      * plot()
//      * splitLargeLeaves()
//      * detail()
//      * visible()
//      * splitMultiTileLeaves()
//      * consolidateSmallLeaves()
//      * consolidateOffScreenBlocks() or its replacement
//      * everything to do with complete/incomplete
//    * A Matrix object which provides services to FractalMatrix and which has
//      shrink() and grow().
//      * Should not have to touch this to move to SOI.
//    * A LeafType object encapsulating BlockType and precision.
//    * LeafInfo/Leaf* pairs.
//    * Given a LeafInfo, how much space do we need?
//    * Move a Leaf from one location to another.
//      * Need to reseatLeaf() and moveFrom() only when moved by the memory
//        system.
//


template<class FractalProcessor> class LeafBuffer
{
public:
    void copyFrom(Block* block, BlockType blockType)
    {
        _blockType = blockType;
        switch(blockType) {
            case incompleteBlockType:
                {
                    IncompleteLeaf* incompleteLeaf =
                        static_cast<IncompleteLeaf*>(block);
                    _precision = incompleteLeaf->precision();
                    int bytes = (incompleteLeaf->bytes() + sizeof(int) - 1)
                        /sizeof(int);
                    ensureSpace(bytes);
                    IncompleteLeaf* buffer =
                        reinterpret_cast<IncompleteLeaf*>(_buffer);






                _buffer.resize(


            std::vector<int> incompleteLeafBuffer(
                IncompleteLeaf::bytes()/sizeof(int));
            IncompleteLeaf* incomplete = reinterpret_cast<IncompleteLeaf*>(
                &incompleteLeafBuffer[0]);
    }

    void copyTo(Block* block)
    {
    }

    BlockType blockType() const { return _blockType; }
    int precision() const { return _precision; }
private:
    int ensureSpace(int bytes)
    {
        int size = (bytes + sizeof(int) - 1)/sizeof(int);
        if (size > _buffer.size())
            _buffer.resize(size);
    }

    BlockType _blockType;
    std::vector<int> _buffer;
    int _precision;
};


    void test()
    {
        _cx.ensureLength(2);
        _cy.ensureLength(2);
        _zx.ensureLength(2);
        _zy.ensureLength(2);
        SignedDigit bailoutRadius2 = static_cast<SignedDigit>(
            ldexp(16.0, bitsPerDigit - intBits));
        _maximumIterations = 0x4000;

        zero(_zx, 1);
        zero(_zy, 1);
        _cx[0] = 0xffa615ce;
        _cy[0] = 2;
        printf("Precision 1\n");
        int p = 1;
        ensureLengths();
        fixedFromDouble(_t[3], 1.0, _logDelta, p);
        int maximumIterations = _maximumIterations;
        for (int i = 0; i < maximumIterations; ++i) {
            multiply(_t[0], _zx, _zx, _t[4], p);
            multiply(_t[1], _zy, _zy, _t[4], p);
            printf("_zx^2 = "); print(_t[0], p); printf("\n");
            printf("_zy^2 = "); print(_t[1], p); printf("\n");
            add(_t[2], _t[0], _t[1], p);
            printf("|z|^2 = "); print(_t[2], p); printf("\n");
            if (static_cast<SignedDigit>(_t[2][p - 1]) > bailoutRadius2) {
                printf("Bailed out after %i iterations\n", i + 1);
                break;
            }
            multiply(_t[2], _zx, _zy, _t[4], p, intBits/* + 1*/);
            printf("_zx*_zy = "); print(_t[2], p); printf("\n");
            shiftLeft(_t[2], _t[2], p, 1);
            printf("2*_zx*_zy = "); print(_t[2], p); printf("\n");
            add(_zx, _t[0], _cx, p);
            sub(_zx, _zx, _t[1], p);
            printf("New _zx = "); print(_zx, p); printf("\n");
            add(_zy, _t[2], _cy, p);
            printf("New _zy = "); print(_zy, p); printf("\n");
        }

        zero(_zx, 2);
        zero(_zy, 2);
        _cx[0] = 0;
        _cx[1] = 0xffa615ce;
        _cy[0] = 0;
        _cy[1] = 2;
        printf("Precision 2\n");
        p = 2;
        ensureLengths();
        fixedFromDouble(_t[3], 1.0, _logDelta, p);
        for (int i = 0; i < maximumIterations; ++i) {
            multiply(_t[0], _zx, _zx, _t[4], p);
            multiply(_t[1], _zy, _zy, _t[4], p);
            printf("_zx^2 = "); print(_t[0], p); printf("\n");
            printf("_zy^2 = "); print(_t[1], p); printf("\n");
            add(_t[2], _t[0], _t[1], p);
            printf("|z|^2 = "); print(_t[2], p); printf("\n");
            if (static_cast<SignedDigit>(_t[2][p - 1]) > bailoutRadius2) {
                printf("Bailed out after %i iterations\n", i + 1);
                break;
            }
            multiply(_t[2], _zx, _zy, _t[4], p, intBits/* + 1*/);
            printf("_zx*_zy = "); print(_t[2], p); printf("\n");
            shiftLeft(_t[2], _t[2], p, 1);
            printf("2*_zx*_zy = "); print(_t[2], p); printf("\n");
            add(_zx, _t[0], _cx, p);
            sub(_zx, _zx, _t[1], p);
            printf("New _zx = "); print(_zx, p); printf("\n");
            add(_zy, _t[2], _cy, p);
            printf("New _zy = "); print(_zy, p); printf("\n");
        }

    }


                MandelbrotEvaluator<FractalProcessor> evaluator(0);
    evaluator.test();



//* Artifacts at precision boundaries.
//  * When completing a block (add code to complete()) that isn't the TL in its
//    quadrant:
//    * Find the sibling blocks.
//      * Just look in the grid (or parent grid if we're 1x1) - low bits of
//        position.
//      * If any of the siblings are grids, descend to the TL of each.
//    * If the TL is lower precision than all the other 3, de-complete it.
//      * Can the TL be a gridblock? Why would we have smaller, lower-precision
//        blocks?
//    * At what point in the completion sequence? Before grid consolidation I
//      think (otherwise we may do redundant consolidation work).
//      * Before neighbour split.
//
//
//precision 2->precision 3 transition at x = fff00162 02aa0000, y = 0


    // Removes the entire block and replaces it with its top-left leaf.
    void consolidateBlock()
    {
        if (_logBlocksPerGroup > 0) {
            while (_logBlocksPerGroup < logBlocksPerGrid())
                split();
            ascend();
        }

        // Make a copy of the leaf.
        Manipulator corner = *this;
        corner.descendToLeaf(topLeft());
        BlockType type = corner.blockType();
        int logLeavesPerBlock =
            logPointsPerGroup() - corner.logPointsPerGroup();
        if (type == incompleteLeafType) {
            std::vector<int> incompleteLeafBuffer(
                IncompleteLeaf::bytes(precision())/sizeof(int));
            IncompleteLeaf* incomplete = reinterpret_cast<IncompleteLeaf*>(
                &incompleteLeafBuffer[0]);
            incomplete->init(corner.incompleteLeaf());

            // Replace the grid.
            recursiveDeleteChildGrid();
            createGrid(GridType(0, incompleteLeafType,
                screenPrecision(-logLeavesPerBlock)));
            descend();

            // Copy the leaf back.
            incompleteLeaf()->init(incomplete);
        }
        else {
            CompleteLeaf complete = *corner.completeLeaf();

            // Replace the grid.
            recursiveDeleteChildGrid();
            createGrid(GridType(0, incompleteLeafType,
                screenPrecision(-logLeavesPerBlock)));
            descend();

            // Copy the leaf back.
            *completeLeaf() = complete;
        }

        tryConsolidateGrid();
    }


    void consolidateOffScreenBlocks()
    {
//        printf("Consolidating offscreen blocks\n");
        reset();
        int logPointsPerTile = _processor->screen()->logPointsPerTile();
        do {
            if (!visible() && logPointsPerGroup() <= logPointsPerTile &&
                (_logBlocksPerGroup > 0 || blockType() == gridBlockType) &&
                canConsolidateBlock()) {
                consolidateBlock();
                if (!atLastQuadrant()) {
                    moveNextQuadrant();
                    continue;
                }
                // Ascend
            }
            else
                if (!_processor->screen()->blockEntirelyVisible(_location) &&
                    (blockType() == gridBlockType || _logBlocksPerGroup > 0)) {
                    descendQuadrant();
                    continue;
                }
                else {
                    if (!atLastQuadrant()) {
                        moveNextQuadrant();
                        continue;
                    }
                    // Ascend;
                }
            do {
                ascendQuadrant();
                if (atTop()) {
//                    _processor->matrix()->_root->dump(_processor);
                    return;
                }
                if (atLastQuadrant())
                    continue;
                moveNextQuadrant();
                break;
            } while (true);
        } while (true);
    }





    bool longFixedNeeded()
    {
        if (_precision < 2)
            return false;
        if (_precision > 2)
            return true;
        Digit* d = _cx;
        if ((d[1] & 3) != 0)
            return true;
        d = _cy;
        return ((d[1] & 3) != 0);
    }


        if (logPoints < _logPointsPerTexel)
            printf("Sub-texel!\n");

        Vector towerBottomRight = _towerGrid->size()<<_logTexelsPerTower;

            printf("Growing from quadrant %i,%i\n",quadrant.x,quadrant.y);
            printf("Matrix covers texel area %x,%x - %x,%x\n",
                _matrixOrigin.x,
                _matrixOrigin.y,
                _matrixOrigin.x + ((1<<30)>>_logPointsPerTexel),
                _matrixOrigin.y + ((1<<30)>>_logPointsPerTexel));

        printf("Delta.x = "); print(_temp, _precision);
        printf(" zoomTexel.x = %.17lf\n",zoomTexel.x);
        printf("Delta.y = "); print(_temp, _precision);
        printf(" zoomTexel.y = %.17lf\n",zoomTexel.y);
        printf("originPixel = %f,%f\n",_originPixel.x,_originPixel.y);

        printf("Matrix needs to cover texel area 0,0 - %x,%x\n",towerBottomRight.x,towerBottomRight.y);

        printf("Matrix covers texel area %x,%x - %x,%x\n",
            _matrixOrigin.x,
            _matrixOrigin.y,
            _matrixOrigin.x + ((1<<30)>>_logPointsPerTexel),
            _matrixOrigin.y + ((1<<30)>>_logPointsPerTexel));


            printf("_originUnitX = "); print(_originUnitX, _precision); /*print(doubleFromFixed(_originUnitX, 0, _precision));*/ printf("\n");
            printf("_originUnitY = "); print(_originUnitY, _precision); /*print(doubleFromFixed(_originUnitY, 0, _precision));*/ printf("\n");

            if (tl.x > br.x || tl.y > br.y) {
                printf("tl = %i,%i  br = %i,%i  towerBottomRight = 0x%x,0x%x  _matrixOrigin = 0x%08x,0x%08x  _logPointsPerTexel = %i\n",tl.x,tl.y,br.x,br.y,towerBottomRight.x,towerBottomRight.y,_matrixOrigin.x,_matrixOrigin.y,_logPointsPerTexel);
                printf("semiQuadrant wraparound!\n");
            }

                if (tl.x < 0 || tl.y < 0 || br.x > 4 || br.y > 4)
                    printf("Bad semiQuadrant!\n");

                printf("Shrinking matrix to semiQuadrant %i,%i\n",tl.x,tl.y);
                printf("Matrix covers texel area %x,%x - %x,%x\n",
                    _matrixOrigin.x,
                    _matrixOrigin.y,
                    _matrixOrigin.x + ((1<<30)>>_logPointsPerTexel),
                    _matrixOrigin.y + ((1<<30)>>_logPointsPerTexel));

            _matrix->consolidateOffScreenBlocks();


        printf("Moving grid at 0x%08x to 0x%08x\n",oldGrid,this);

                        printf("Reseating leaf from 0x%08x to 0x%08x\n",oldLeaf,incompleteLeaf);


        if (s < 0)
            printf("Bad location!\n");

        if (logPointsPerGroup() < 0)
            printf("Descended to a 0-point group!\n");

        if (logPointsPerGroup() < 0)
            printf("Descended to a 0-point group!\n");

        printf("Completing block\n");

        printf("Completing: deleting child grid\n");

        printf("Completing: creating new child grid\n");

        _processor->matrix()->_root->dump(_processor);

                printf("Adding 0x%08x\n",leaf);

        printf("Attempting to consolidate grid\n");

        printf("Consoldating grid\n");

                printf("Removing 0x%08x\n",incompleteLeaf());



        printf("Splitting leaf\n");

            printf("Splitting incomplete leaf\n");

            printf("Splitting complete leaf\n");

        checkVectors();

        printf("Split completed\n");
        _processor->matrix()->_root->dump(_processor);
        printf("Dumped\n");

    void dump(int indent = 0)
    {
        //char* type = "grids";
        //if (_grid->_gridType.blockType() == incompleteLeafType)
        //    type = "incompletes";
        //if (_grid->_gridType.blockType() == completeLeafType)
        //    type = "completes";
        printf("Grid 0x%08x Position %i,%i size %i of i s = Point %08x,%08x size %i\n",/*indent, "",*/ _grid, _position.x, _position.y, _logBlocksPerGroup, /*_grid->_gridType.logBlocks(), type,*/ topLeft().x, topLeft().y, logPointsPerGroup());
        _processor->matrix()->_root->dump(_processor, _grid);
        //if (parent()._grid != 0)
        //    parent().dump(indent + 2);
    }

    void dump2()
    {
        printf("Grid 0x%08x Position %i,%i size %i = Point %08x,%08x size %i\n",_grid, _position.x, _position.y, _logBlocksPerGroup, topLeft().x, topLeft().y, logPointsPerGroup());
    }

        printf("Consolidating leaf\n");
        _processor->matrix()->_root->dump(_processor);
        dump();
        checkVectors();
        printf("Before consolidation, logPointsPerGroup() == %i, logPointsPerBlock == %i, _logBlocksPerGroup == %i\n",logPointsPerGroup(), logPointsPerBlock(_grid), _logBlocksPerGroup);
        dump();

        printf("After ascension, logPointsPerGroup() == %i, logPointsPerBlock == %i, _logBlocksPerGroup == %i\n",logPointsPerGroup(), logPointsPerBlock(_grid), _logBlocksPerGroup);
        dump();

        printf("After split, logPointsPerGroup() == %i, logPointsPerBlock == %i, _logBlocksPerGroup == %i\n",logPointsPerGroup(), logPointsPerBlock(_grid), _logBlocksPerGroup);
        dump();

            if (subGrid()->_gridType.blockType() == gridBlockType || subGrid()->_gridType.logBlocks() != 0)
                printf("Bad leaf grid consolidation\n");

            printf("newGrid = 0x%08x\n",newGrid);

                printf("Changing size of incomplete leaf at 0x%08x\n",leaf);

             printf("After reparenting, logPointsPerGroup() == %i, logPointsPerBlock == %i, _logBlocksPerGroup == %i\n",logPointsPerGroup(), logPointsPerBlock(_grid), _logBlocksPerGroup);
            dump2();

            printf("After creation, logPointsPerGroup() == %i, logPointsPerBlock == %i, _logBlocksPerGroup == %i\n",logPointsPerGroup(), logPointsPerBlock(_grid), _logBlocksPerGroup);
            dump2();

        printf("newGrid = 0x%08x\n");

        printf("deleteGrid() complete\n");

        printf("After deletion, logPointsPerGroup() == %i, logPointsPerBlock == %i, _logBlocksPerGroup == %i\n",logPointsPerGroup(), logPointsPerBlock(_grid), _logBlocksPerGroup);
        dump();

        printf("After increment, logPointsPerGroup() == %i, logPointsPerBlock == %i, _logBlocksPerGroup == %i\n",logPointsPerGroup(), logPointsPerBlock(_grid), _logBlocksPerGroup);
        dump();
        printf("Consolidation complete\n");
        checkVectors();
        _processor->matrix()->_root->dump(_processor);

        printf("Consolidating offscreen leaf\n");
        checkVectors();

        checkVectors();

        printf("Splitting grid\n");

        printf("Attempting to find leaf 0x%08x\n",leaf);

        printf("Splitting multi-tile leaves\n");

                    _processor->matrix()->_root->dump(_processor);

        printf("Growing from quadrant %i,%i\n",quadrant.x,quadrant.y);

        _processor->matrix()->_root->dump(_processor);
        printf("Dumped\n");

        printf("Shrinking to semiQuadrant %i,%i\n",semiQuadrant.x, semiQuadrant.y);

        _processor->matrix()->_root->dump(_processor);

                    _processor->matrix()->_root->dump(_processor);

                    _processor->matrix()->_root->dump(_processor);

    bool _finalizeDeletionsPending;
    Grid* _deletionQueue;

    void finalizeDeletions()
    {
        _grid = _deletionQueue;
        _position = Vector(0, 0);
        _
        do {
            if (_interrupt( {
                _finalizeDeletionsPending = true;
                return;
            }
            if (_deletionQueue->_gridType
    }

        if (_finalizeDeletionsPending) {
            _finalizeDeletionsPending = false;
            finalizeDeletions();
            if (_finalizeDeletionsPending)
                return false;
        }

        if (_deletionQueue == oldGrid)
            _deletionQueue = newGrid;

        _finalizeDeletionsPending(false),
        _deletionQueue(0)

  //* Add a "Grid* _deletionQueue" to Matrix, initialized to 0.
  //* Fix this up in reseatGrid.
  //* shrink() always deletes 3 quadrants and their containing 2x2 grid. Put the
  //  existing _deletionQueue in the 4th quadrant and make the parent the new
  //  _deletionQueue.
  //* 0 in a grid means already deleted.
  //* Add a resume() function to actually do the deletion, priority 2.

        bool display = true;
        for (int i = 0; i < _precision - 1; ++i)
            if (_cx[i] != 0 || _cy[i] != 0)
                display = false;
        if (_cx[_precision - 1] != 0 || _cy[_precision - 1] != 0xfff00000)
            display = false;

                if (display)
                    printf("_precision=%i  _result=%i\n",_precision,i+1);
        if (display) {
            printf("_zx = "); print(_zx, _precision); printf("\n");
            printf("_zy = "); print(_zy, _precision); printf("\n");
            printf("_zSx = "); print(_zSx, _precision); printf("\n");
            printf("_zSy = "); print(_zSy, _precision); printf("\n");
        }

            if (display) {
                printf("diff.x = "); print(_t[0], _precision); printf("\n");
            }
            if (display) {
                printf("absdiff.x = "); print(_t[0], _precision); printf("\n");
                printf("delta = "); print(_t[3], _precision); printf("\n");
            }
                if (display)
                    printf("ltx\n");
                if (display) {
                    printf("diff.y = "); print(_t[0], _precision); printf("\n");
                }
                if (display) {
                    printf("absdiff.y = "); print(_t[0], _precision); printf("\n");
                }
                    printf("lty\n");

            if (logPointsPerGroup() < logPointsPerBlock)
                printf("Matrix invariant broken 0!\n");

        moveTo(point);
        if (logPointsPerGroup() < logPointsPerBlock)
            printf("Not consolidatable 0\n");
        moveTo(point + Vector(pointsPerBlock, 0));
        if (logPointsPerGroup() < logPointsPerBlock)
            printf("Not consolidatable 1\n");
        moveTo(point + Vector(pointsPerBlock, pointsPerBlock));
        if (logPointsPerGroup() < logPointsPerBlock)
            printf("Not consolidatable 2\n");
        moveTo(point + Vector(0, pointsPerBlock));
        if (logPointsPerGroup() < logPointsPerBlock)
            printf("Not consolidatable 3\n");

        if (_logBlocksPerGroup != 0)
            printf("consolidateLeaf() called on group!\n");
        Vector point = topLeft();
        int pointSize = logPointsPerGroup();

        if (topLeft() != point || pointSize != logPointsPerGroup())
            printf("ascend() changed point size!\n");
        if (logBlocksPerGrid() == 0)
            printf("Still 1x1 after ascend()!\n");

        if (topLeft() != point || pointSize != logPointsPerGroup())
            printf("split() changed point size!\n");
        if (logBlocksPerGrid() == 0)
            printf("Still 1x1 after split()!\n");
        if (_logBlocksPerGroup != 0)
            printf("split() left us in a group!\n");

        bool gbt = false;
        bool icb = false;
        Grid oldGrid = *_grid;
        Grid oldChild;
        Grid parentGrid = *parent()._grid;

                if (child()._grid->_gridType.logBlocks() != 0)
                    printf("Bad consolidation 3\n");
                if (child()._grid->_gridType.blockType() == gridBlockType)
                    printf("Bad consolidation 4\n");

            gbt = true;

            oldChild = *newGrid;
            if (newGrid->_gridType.logBlocks() != 0)
                printf("Bad consolidation\n");
            if (newGrid->_gridType.blockType() == gridBlockType)
                printf("Bad consolidation 2\n");

                icb = true;

        if (pointSize != logPointsPerGroup())
            printf("point size bad after consolidate!\n");

        if (_logBlocksPerGroup != 0)
            printf("_logBlocksPerGroup = %i after consolidate!\n");

        if (pointSize != logPointsPerGroup() - 1)
            printf("point size bad after tryConsolidateGrid()!\ngtb=%i\n",gbt);
        if (_logBlocksPerGroup != 0)
            printf("_logBlocksPerGroup = %i after tryConsolidateGrid()!\n");
        if ((point &~ ((1 << (pointSize/* + 1*/))-1))!=topLeft())
            printf("point moved from %08x,%08x to %08x,%08x - pointSize=%i\n",point.x,point.y,topLeft().x,topLeft().y,pointSize);
        if (logPointsPerGroup() != logPointsPerBlock(_grid)) {
            printf("point size incorrect - cached %i real %i\ngbt=%i\n",logPointsPerGroup(),logPointsPerBlock(_grid),gbt);
            printf("old point=%08x,%08x\n",point.x,point.y);
            printf("old pointSize=%i\n",pointSize);
            printf("current point=%08x,%08x\n",topLeft().x,topLeft().y);
            Vector rtl = realTopLeft();
            printf("current point should be %08x,%08x\n",rtl.x,rtl.y);
            printf("icb=%i\n",icb);
            printf("oldGrid: "); oldGrid.print();
            printf("oldChild: "); oldChild.print();
            printf("parentGrid: "); parentGrid.print();
            _processor->matrix()->_root->dump(_processor, _grid);
        }

        if (logPointsPerGroup() < 0)
            printf("Sub-zero point in descendQuadrant()!\n");

        if (logPointsPerGroup() < 0)
            printf("Sub-zero point in descend()!\n");

        if (s < 0)
            printf("Sub-zero point in child()\n");

    Grid() : _gridType(0, gridBlockType, 0) { }  // TODO: remove



        if (queue < 0 || queue >= _n)
            printf("Bad queue %i/%i\n",queue,_n);

start:
        if (blockEntirelyVisible())
            goto nextQuadrant;
        if (_logBlocksPerGroup == 0 && blockType() != gridBlockType)
            goto ascend;

        if (_logBlocksPerGroup == 0)
            goto ascend;
        if (_logBlocksPerGroup > 1 ||
            (blockType() == gridBlockType && !fourLeaves()))
            goto descend;


        if (!canConsolidateBlock() || visible() ||
            logPointsPerBlock > logPointsPerTile)
            goto nextQuadrant;
        consolidateLeaf();
        goto nextQuadrant;



            if (!manipulator->visible()) {
                unsigned int time = static_cast<unsigned int>(__rdtsc());
                if (manipulator->tryConsolidateLeafRepeatedly()) {
                    unsigned int t = static_cast<unsigned int>(__rdtsc()) - time;
                    if (t > LONG_TIME) {
                        printf("tclrf %i\n",t/33333);
                    }
                    // We did some consolidation - release the lock before
                    // doing any more.
                    _pointValid = false;
                    unsigned int t5 = static_cast<unsigned int>(__rdtsc()) - t4;
                    if (t5 > LONG_TIME)
                        printf("preconsolidated %i\n",t5/33333);
                    return false;
                    //continue;
                }
                unsigned int t = static_cast<unsigned int>(__rdtsc()) - time;
                if (t > LONG_TIME) {
                    printf("tclrs %i\n",t/33333);
                }
                // If we couldn't consolidate, iterate the leaf. Iterating
                // offscreen leaves may seem strange, but if we don't do so
                // we'll never finish (we'll end up going round and round a
                // queue full of incomplete leaves).
                // We need to refresh our leaf because even though the leaf
                // itself wasn't consolidated, some nearby leaves might have
                // been.
                leaf = manipulator->incompleteLeaf();
            }

    // If we can consolidate this non-visible leaf without any visible effects,
    // do so and return true. Performing this consolidation may require
    // consolidating neighbouring non-visible leaves and so on recursively.
    // We can use recusion here since we don't have any Grid pointers on the
    // stack.
    bool tryConsolidateLeaf()
    {
        // Make sure the new block wouldn't be too large.
        int logPointsPerBlock = logPointsPerGroup();
        if (_processor->screen()->logPointsPerTile() == logPointsPerBlock)
            return false;

        // Check what the visibility of the new block would be. If it would be
        // visible we won't do the consolidation.
        int pointsPerBlock = pointsPerGroup();
        Vector point = topLeft()&~pointsPerBlock;
        if (_processor->screen()->blockVisible(BlockLocation(
            point, logPointsPerBlock + 1)))
            return false;

        // Make sure all the sibling blocks are the same size.
        moveTo(point);
        while (logPointsPerGroup() < logPointsPerBlock)
            if (!tryConsolidateLeaf())
                return false;
        moveTo(point + Vector(pointsPerBlock, 0));
        while (logPointsPerGroup() < logPointsPerBlock)
            if (!tryConsolidateLeaf())
                return false;
        moveTo(point + Vector(pointsPerBlock, pointsPerBlock));
        while (logPointsPerGroup() < logPointsPerBlock)
            if (!tryConsolidateLeaf())
                return false;
        moveTo(point + Vector(0, pointsPerBlock));
        while (logPointsPerGroup() < logPointsPerBlock)
            if (!tryConsolidateLeaf())
                return false;

        // Check the size of the adjacent blocks. If any of them are too small,
        // tryConsolidateLeaf() recursively.
        int pointsPerTwoBlocks = pointsPerBlock << 1;
        if (point.x > 0) {
            moveTo(point + Vector(-1, 0));
            if (logPointsPerGroup() < logPointsPerBlock)
                if (!tryConsolidateLeaf())
                    return false;
            moveTo(point + Vector(-1, pointsPerBlock));
            if (logPointsPerGroup() < logPointsPerBlock)
                if (!tryConsolidateLeaf())
                    return false;
        }
        if (point.y > 0) {
            moveTo(point + Vector(0, -1));
            if (logPointsPerGroup() < logPointsPerBlock)
                if (!tryConsolidateLeaf())
                    return false;
            moveTo(point + Vector(pointsPerBlock, -1));
            if (logPointsPerGroup() < logPointsPerBlock)
                if (!tryConsolidateLeaf())
                    return false;
        }
        if (point.x < 0x40000000 - pointsPerTwoBlocks) {
            moveTo(point + Vector(pointsPerTwoBlocks, 0));
            if (logPointsPerGroup() < logPointsPerBlock)
                if (!tryConsolidateLeaf())
                    return false;
            moveTo(point + Vector(pointsPerTwoBlocks, pointsPerBlock));
            if (logPointsPerGroup() < logPointsPerBlock)
                if (!tryConsolidateLeaf())
                    return false;
        }
        if (point.y < 0x40000000 - pointsPerTwoBlocks) {
            moveTo(point + Vector(0, pointsPerTwoBlocks));
            if (logPointsPerGroup() < logPointsPerBlock)
                if (!tryConsolidateLeaf())
                    return false;
            moveTo(point + Vector(pointsPerBlock, pointsPerTwoBlocks));
            if (logPointsPerGroup() < logPointsPerBlock)
                if (!tryConsolidateLeaf())
                    return false;
        }

        // We are clear to consolidate.
        moveTo(point);
        consolidateLeaf();
        return true;
    }

    bool tryConsolidateLeafRepeatedly()
    {
        Vector point = topLeft();
        bool consolidated = false;
        /*while*/ if (tryConsolidateLeaf())
            consolidated = true;
        moveTo(point);
        return consolidated;
    }




                    //printf("_logBlocksPerGroup = %i\n",_logBlocksPerGroup);
                    //printf("blockType() = %s\n",_grid->toString(blockType()));
                    //if (_logBlocksPerGroup <= 1 && blockType()==gridBlockType)
                    //    printf("fourLeaves: %i\n",fourLeaves());
                    //printf("grid = 0x%08x\n",_grid);
                    //printf("_position = 0x%08x,0x%08x\n",_position.x,_position.y);

                    if (_logBlocksPerGroup > 1 ||
                        (blockType() == gridBlockType &&
                        (_logBlocksPerGroup != 1 || !fourLeaves()))) {
                        descendQuadrant();
                        continue;
                    }
                    if (canConsolidateBlock() && !visible() &&
                        logPointsPerGroup() <= logPointsPerTile) {
                        //_root->dump(_processor, _grid);
                        descendQuadrant();
                        consolidateLeaf();
                        //checkVectors();
                        //_root->dump(_processor, reinterpret_cast<Grid*>(1));
                        ascendQuadrant();
                        //printf("Consolidated leaf\n");
                        _consolidatedOffScreenLeaves = true;
                        continue;
                    }
                    if (!atLastQuadrant()) {
                        moveNextQuadrant();
                        continue;
                    }
                    // Ascend
                }


    bool canConsolidateLeaf()
    {
        Vector oldPoint = topLeft();
        // Make sure the new block wouldn't be too large.
        int logPointsPerLeaf = logPointsPerGroup();
        if (_processor->screen()->logPointsPerTile() == logPointsPerLeaf)
            return false;

        // Check what the visibility of the new block would be. If it would be
        // visible we won't do the consolidation.
        int pointsPerLeaf = pointsPerGroup();
        Vector point = oldPoint&~pointsPerLeaf;
        if (_processor->screen()->blockVisible(BlockLocation(
            point, logPointsPerLeaf + 1)))
            return false;

        // Make sure all the sibling blocks are the same size.
        moveTo(point);
        if (logPointsPerGroup() < logPointsPerLeaf) {
            moveTo(oldPoint);
            return false;
        }
        moveTo(point + Vector(pointsPerLeaf, 0));
        if (logPointsPerGroup() < logPointsPerLeaf) {
            moveTo(oldPoint);
            return false;
        }
        moveTo(point + Vector(pointsPerLeaf, pointsPerLeaf));
        if (logPointsPerGroup() < logPointsPerLeaf) {
            moveTo(oldPoint);
            return false;
        }
        moveTo(point + Vector(0, pointsPerLeaf));
        if (logPointsPerGroup() < logPointsPerLeaf) {
            moveTo(oldPoint);
            return false;
        }

        // Check the size of the adjacent blocks. If any of them are too small,
        // tryConsolidateLeaf() recursively.
        int pointsPerTwoLeaves = pointsPerLeaf << 1;
        if (point.x > 0) {
            moveTo(point + Vector(-1, 0));
            if (logPointsPerGroup() < logPointsPerLeaf) {
                moveTo(oldPoint);
                return false;
            }
            moveTo(point + Vector(-1, pointsPerLeaf));
            if (logPointsPerGroup() < logPointsPerLeaf) {
                moveTo(oldPoint);
                return false;
            }
        }
        if (point.y > 0) {
            moveTo(point + Vector(0, -1));
            if (logPointsPerGroup() < logPointsPerLeaf) {
                moveTo(oldPoint);
                return false;
            }
            moveTo(point + Vector(pointsPerLeaf, -1));
            if (logPointsPerGroup() < logPointsPerLeaf) {
                moveTo(oldPoint);
                return false;
            }
        }
        if (point.x < 0x40000000 - pointsPerTwoLeaves) {
            moveTo(point + Vector(pointsPerTwoLeaves, 0));
            if (logPointsPerGroup() < logPointsPerLeaf) {
                moveTo(oldPoint);
                return false;
            }
            moveTo(point + Vector(pointsPerTwoLeaves, pointsPerLeaf));
            if (logPointsPerGroup() < logPointsPerLeaf) {
                moveTo(oldPoint);
                return false;
            }
        }
        if (point.y < 0x40000000 - pointsPerTwoLeaves) {
            moveTo(point + Vector(0, pointsPerTwoLeaves));
            if (logPointsPerGroup() < logPointsPerLeaf) {
                moveTo(oldPoint);
                return false;
            }
            moveTo(point + Vector(pointsPerLeaf, pointsPerTwoLeaves));
            if (logPointsPerGroup() < logPointsPerLeaf) {
                moveTo(oldPoint);
                return false;
            }
        }

        // We are clear to consolidate.
        moveTo(oldPoint);
        return true;
    }

//        printf("Consolidating started\n");
        reset();
        moveTo(_consolidateOffScreenLeavesLocation);
        //int logPointsPerTile = _processor->screen()->logPointsPerTile();
        do {
            if (_interrupt) {
                _consolidateOffScreenLeavesPending = true;
                _consolidateOffScreenLeavesLocation = _location;
//                printf("Consolidating interrupted\n");
                return;
            }
            if (_processor->screen()->blockEntirelyVisible(_location)) {
                if (!atLastQuadrant()) {
                    moveNextQuadrant();
                    continue;
                }
                // Ascend
            }
            else
                if (_logBlocksPerGroup != 0 || blockType() == gridBlockType) {
                    descendQuadrant();
                    continue;
                }
                else {
                    if (canConsolidateLeaf()) {
                        consolidateLeaf();
                        _consolidatedOffScreenLeaves = true;
                        continue;
                    }
                    if (!atLastQuadrant()) {
                        moveNextQuadrant();
                        continue;
                    }
                    // Ascend
                }
            do {
                ascendQuadrant();
                if (atTop()) {
                    if (_consolidatedOffScreenLeaves) {
                        // Consolidating may have unblocked another
                        // consolidation. Keep going until there's no more to
                        // do.
//                        printf("Restarting consolidation\n");
                        _consolidatedOffScreenLeaves = false;
                        reset();
                        moveTo(BlockLocation(Vector(0, 0), 29));
                        break;
                    }
//                    printf("Consolidating complete\n");
                    return;
                }
                if (atLastQuadrant())
                    continue;
                moveNextQuadrant();
                break;
            } while (true);
        } while (true);


    public: void consolidateOffScreenLeaves()
    {
//        printf("Consolidating started\n");
        reset();
        moveTo(_consolidateOffScreenLeavesLocation);
        int logPointsPerTile = _processor->screen()->logPointsPerTile();
        do {
            if (_interrupt) {
                _consolidateOffScreenLeavesPending = true;
                _consolidateOffScreenLeavesLocation = _location;
//                printf("Consolidating interrupted\n");
                return;
            }
            if (_processor->screen()->blockEntirelyVisible(_location)) {
                if (!atLastQuadrant()) {
                    moveNextQuadrant();
                    continue;
                }
                // Ascend
            }
            else
                if (_logBlocksPerGroup != 0 || blockType() == gridBlockType) {
                    //printf("_logBlocksPerGroup = %i\n",_logBlocksPerGroup);
                    //printf("blockType() = %s\n",_grid->toString(blockType()));
                    //if (_logBlocksPerGroup <= 1 && blockType()==gridBlockType)
                    //    printf("fourLeaves: %i\n",fourLeaves());
                    //printf("grid = 0x%08x\n",_grid);
                    //printf("_position = 0x%08x,0x%08x\n",_position.x,_position.y);

                    if (_logBlocksPerGroup > 1 ||
                        (blockType() == gridBlockType &&
                        (_logBlocksPerGroup != 1 || !fourLeaves()))) {
                        descendQuadrant();
                        continue;
                    }
                    if (canConsolidateBlock() && !visible() &&
                        logPointsPerGroup() <= logPointsPerTile) {
                        //_root->dump(_processor, _grid);
                        descendQuadrant();
                        consolidateLeaf();
                        //checkVectors();
                        //_root->dump(_processor, reinterpret_cast<Grid*>(1));
                        ascendQuadrant();
                        //printf("Consolidated leaf\n");
                        _consolidatedOffScreenLeaves = true;
                        continue;
                    }
                    if (!atLastQuadrant()) {
                        moveNextQuadrant();
                        continue;
                    }
                    // Ascend
                }
            do {
                ascendQuadrant();
                if (atTop()) {
                    if (_consolidatedOffScreenLeaves) {
                        // Consolidating may have unblocked another
                        // consolidation. Keep going until there's no more to
                        // do.
                        printf("Restarting consolidation\n");
                        _consolidatedOffScreenLeaves = false;
                        reset();
                        moveTo(BlockLocation(Vector(0, 0), 29));
                        break;
                    }
//                    printf("Consolidating complete\n");
                    return;
                }
                if (atLastQuadrant())
                    continue;
                moveNextQuadrant();
                break;
            } while (true);
        } while (true);
    }


    bool fourLeaves()
    {
        for (int i = 0; i < 4; ++i) {
            GridType type = static_cast<GridBlock*>(_grid->blockAtPosition(
                _position | Vector(i&1, i>>1)))->_grid->_gridType;
            if (type.logBlocks() > 0 || type.blockType() == gridBlockType)
                return false;
        }
        return true;
    }


    // Keeping the tiles in the central 2x2 part of the 4x4 matrix.
    // Has the bad side effect of making splitMultiTileLeaves significant and
    // only postpones the inevitable offscreen consolidation.

            Vector tl = pointFromTexel(Vector(0, 0)) >> 27;
            Vector br = pointFromTexel(towerBottomRight) >> 27;
            //            tl.x
            //         0  1  2  3  4  5  6  7
            //     0   0
            //br.x 1   0  0
            //     2   0  0  0
            //     3   -  -  1  1
            //     4   -  -  -  1  1
            //     5   -  -  -  -  1  2
            //     6   -  -  -  -  -  2  2
            //     7   -  -  -  -  -  2  2  2
            if (tl.x == 2 && br.x == 3)
                ++tl.x;
            if (tl.x == 4 && br.x == 5)
                --br.x;
            if (tl.y == 2 && br.y == 3)
                ++tl.y;
            if (tl.y == 4 && br.y == 5)
                --br.y;
            tl.x = (clamp(1, tl.x, 6) - 1) >> 1;
            tl.y = (clamp(1, tl.y, 6) - 1) >> 1;
            br.x = (clamp(1, br.x, 6) - 1) >> 1;
            br.y = (clamp(1, br.y, 6) - 1) >> 1;
            if (tl == br) {


    // Returns false if this square has a leaf block of a size less than half
    // the size of this block on any side.
    bool canConsolidateBlock()
    {
//        printf("In canConsolidateBlock\n");
        Site l = *this;
        l.descendToLeaf(topLeft());
        if (l.moveLeft()) {
            if (l.logPointsPerGroup() < logPointsPerGroup() - 1)
                return false;
            if (l.logPointsPerGroup() == logPointsPerGroup() - 1) {
                l.moveDown();
                if (l.logPointsPerGroup() < logPointsPerGroup() - 1)
                    return false;
            }
        }
        Site t = *this;
        t.descendToLeaf(topLeft());
        if (t.moveUp()) {
            if (t.logPointsPerGroup() < logPointsPerGroup() - 1)
                return false;
            if (t.logPointsPerGroup() == logPointsPerGroup() - 1) {
                t.moveRight();
                if (t.logPointsPerGroup() < logPointsPerGroup() - 1)
                    return false;
            }
        }
        Vector rPoint =
            topLeft() + Vector(pointsPerGroup(), pointsPerGroup() >> 1);
        if (rPoint.x < 0x40000000) {
            Site r = *this;
            r.moveTo(rPoint);
            if (r.logPointsPerGroup() < logPointsPerGroup() - 1)
                return false;
            if (r.logPointsPerGroup() == logPointsPerGroup() - 1) {
                r.moveUp();
                if (r.logPointsPerGroup() < logPointsPerGroup() - 1)
                    return false;
            }
        }
        Vector dPoint =
            topLeft() + Vector(pointsPerGroup() >> 1, pointsPerGroup());
        if (dPoint.y < 0x40000000) {
            Site d = *this;
            d.moveTo(dPoint);
            if (d.logPointsPerGroup() < logPointsPerGroup() - 1)
                return false;
            if (d.logPointsPerGroup() == logPointsPerGroup() - 1) {
                d.moveLeft();
                if (d.logPointsPerGroup() < logPointsPerGroup() - 1)
                    return false;
            }
        }
//        printf("Found a consolidatable block\n");
        return true;
    }


    int logPointsPerBlock(Grid* grid)
    {
        if (grid == 0) return 30;
        if (grid == grid->_parent) {
            printf("Parent loop!\n");
            return 0;
        }
        return logPointsPerBlock(grid->_parent) - grid->_gridType.logBlocks();
    }
    Vector realTopLeft()
    {
        return Manipulator(_processor, _grid).child(_position).topLeft();
    }
    void checkVectors()
    {
        if (_grid == 0)
            return;
        Vector realPoint = realTopLeft();
        Vector point = topLeft();
        if (point != realPoint)
            printf("Incorrect point - cached %08x,%08x real %08x,%08x\n",point.x,point.y,realPoint.x,realPoint.y);
        if (_position.y < 0 || _position.x < 0 || _position.x >= 1<<logBlocksPerGrid() || _position.y >= 1<<logBlocksPerGrid())
            printf("position out of range\n");
        if (point.x < 0 || point.y < 0 || point.x >= 0x40000000 || point.y >= 0x40000000)
            printf("point out of range\n");
        if (logPointsPerGroup() < 0 || logPointsPerGroup()>30)
            printf("point size out of range\n");
        if ((point & (pointsPerGroup() - 1)) != Vector(0, 0))
            printf("misaligned point\n");
        if ((_position & ((1<<_logBlocksPerGroup) - 1)) != Vector(0, 0))
            printf("misaligned position\n");
        if (logPointsPerGroup() != logPointsPerBlock(_grid) + _logBlocksPerGroup)
            printf("point size incorrect - cached %i real %i logBlocksPerGroup=%i\n",logPointsPerGroup(),logPointsPerBlock(_grid) + _logBlocksPerGroup,_logBlocksPerGroup);
        Vector lPos = ((point >> (logPointsPerGroup() - _logBlocksPerGroup)) & ((1<<logBlocksPerGrid()) - 1));
        Vector rPos = _position;
        if (lPos != rPos)
            printf("misaligned point\n");
    }

            if (GetAsyncKeyState(VK_LSHIFT)&0x80000000) {
                TIME_BEGIN(dump);
                _matrix->dump();
                TIME_END(dump);
            }

                TIME_BEGIN(preShrinkConsolidate);
                _matrix->scheduleConsolidateOffScreenLeaves();
                _matrix->consolidateOffScreenLeaves();  // TODO: remove
                TIME_END(preShrinkConsolidate);

    void dump()
    {
        reset();
        FILE* out = fopen("matrix.raw","wb");
        int logPointsPerTexel = _processor->screen()->logPointsPerTexel();
        int logTexels = 30 - logPointsPerTexel;
        std::vector<char> raw(1<<(logTexels << 1), 0);
        do {
            if (blockType() == gridBlockType || _logBlocksPerGroup != 0) {
                descend();
                continue;
            }
            Vector t = topLeft() >> logPointsPerTexel;
            int logPoints = logPointsPerGroup() - logPointsPerTexel;
            if (logPoints < 0)
                logPoints = 0;
            int size = 1<<logPoints;
            for (int xx = 0; xx < size; ++xx)
                raw[(t.y<<logTexels) + t.x + xx] = -1;
            for (int yy = 0; yy < size; ++yy)
                raw[((t.y + yy)<<logTexels) + t.x] = -1;
            //for (int yy = 1; yy < size; ++yy)
            //    for (int xx = 1; xx < size; ++xx)
            //        raw[((t.y + yy)<<logTexels) + t.x + xx] = 0;
            moveNext();
            if (!atEnd())
                continue;
            do {
                ascend();
                if (atTop()) {
                    fwrite(&raw[0], 1<<(logTexels << 1), 1, out);
                    fclose(out);
                    return;
                }
                moveNext();
            } while (atEnd());
        } while (true);
    }


                printf("shrinking to semiQuadrant %i,%i\n",tl.x,tl.y);
            printf("Expanding from quadrant %i,%i\n",quadrant.x,quadrant.y);





    char* toString(BlockType type)
    {
        return type==gridBlockType ? "grid" : (type==completeLeafType ? "complete" : "incomplete");
    }
    int dump2(FractalProcessor* processor, int indent=2, Grid* isolate = 0, int size=30, size_t* memoryUsed = 0)
    {
        bool display = false;
        if (isolate != reinterpret_cast<Grid*>(1))
            if (isolate != 0) {
                Grid* isolate2 = isolate;
                while (isolate2 != 0) {
                    if (isolate2 == this)
                        display = true;
                    isolate2 = isolate2->_parent;
                }
            }
            else
                display = true;
        if (isolate == this)
            isolate = 0;
        if (display)
            printf("%*s0x%08x %s %i %i\n", indent, "", this, toString(_gridType._blockType), _gridType._logBlocks, size);
        int n = 1 << (_gridType._logBlocks << 1);
        int c = 0;
        for (int i = 0; i < n; ++i) {
            Block* block = blockAtIndex(i);
            if (((unsigned int)(block)&0x80000003) != 0)
                assertTrue(false);
            if (_gridType._blockType == gridBlockType) {
                Grid* g = static_cast<GridBlock*>(block)->_grid;
                if (g->_parent != this)
                    printf("Grid 0x%08x has incorrect parent 0x%08x, should be 0x%08x\n",g,g->_parent,this);
                if (g->_index != i)
                    printf("Grid 0x%08x has incorrect parent index %i, should be %i\n",g,g->_index,i);
                //int logPointsPerLeaf2 = 0;
                //while (g->_parent != 0) {
                //    logPointsPerLeaf2 -= g->_gridType._logBlocks;
                //    g = g->_parent;
                //}
                //logPointsPerLeaf2 += g->_index;
                //int logPointsPerLeaf =
                //    logPointsPerBlock(processor->matrix());
                //if (logPointsPerLeaf != logPointsPerLeaf2) {
                //    printf("logPointsPerLeaf is %i, should be %i\n",logPointsPerLeaf,logPointsPerLeaf2);
                //}
                c += g->dump2(processor, indent + 2, isolate, size - _gridType._logBlocks, memoryUsed);
            }
            else {
                unsigned int col = static_cast<Leaf*>(block)->_colour;
                if (col == 0xcdcdcdcd)
                    col = 0xcdcdcdce;
                if (_gridType._blockType == incompleteLeafType) {
                    IncompleteLeaf* p = static_cast<IncompleteLeaf*>(block);
                    if (p->_next->_prev != p || p->_prev->_next != p)
                        printf("Leaf not in list correctly\n");
                    if (display)
                        printf("%*s%08x: %08x %08x %08x\n", indent + 2, "", p, col, p->_prev, p->_next);
                    if (p->_parent != this)
                        printf("Point 0x%08x has incorrect parent 0x%08x, should be 0x%08x\n",p,p->_parent,this);
                    ++c;
                }
                else
                    if (display)
                        printf("%*s0x%08x\n", indent + 2, "", col);
            }
        }
        *memoryUsed += _gridType.bytes();
        return c;
    }
    void print()
    {
        printf("logBlocks = %i  blockType = %s  precision = %i  _parent = 0x%08x  _index = %i\n",_gridType._logBlocks,toString(_gridType._blockType),_gridType._precision,_parent,_index);
    }

    void dump(FractalProcessor* processor, Grid* isolate = 0, int indent = 0, int size = 30)
    {
        size_t memoryUsed = 0;
        int m = dump2(processor, indent, isolate, size, &memoryUsed);
        //if (m!=incompleteListCount) {
        //    printf("Wrong number of incomplete points in list. Matrix has %i list has %i\n",m,incompleteListCount);
        //    size = 30;
        //}
        //processor->checkForLeaks(memoryUsed);
    }



                    // We need to know the point size of these leaves.
                    Grid* g = this;
                    int logPointsPerLeaf2 = 0;
                    while (g->_parent != 0) {
                        logPointsPerLeaf2 -= g->_gridType._logBlocks;
                        g = g->_parent;
                    }
                    logPointsPerLeaf2 += g->_index;
                    if (logPointsPerLeaf != logPointsPerLeaf2) {
                        printf("logPointsPerLeaf is %i, should be %i\n",logPointsPerLeaf,logPointsPerLeaf2);
                    }

                        printf("Reseating leaf 0x%08x to 0x%08x size %i\n",oldLeaf, incompleteLeaf, logPointsPerLeaf);
                printf("Adding leaf 0x%08x size %i\n",leaf,logPointsPerGroup());
                printf("Removing leaf 0x%08x size %i\n",incompleteLeaf(),logPointsPerGroup());
                printf("Moving leaf 0x%08x from %i to %i\n",leaf,logPointsPerGroup(),logPointsPerGroup() + 1);
        printf("Growing from quadrant %i,%i - lppbb %i - %i\n",quadrant.x,quadrant.y,_logPointsPerBlockBase,_logPointsPerBlockBase-1);
        printf("Grow completed\n");
        printf("Shrinking to semiQuadrant %i,%i - lppbb %i - %i\n",semiQuadrant.x,semiQuadrant.y,_logPointsPerBlockBase,_logPointsPerBlockBase+1);
        printf("Shrink completed\n");

    int precision() const { return _precision; }
    BlockType blockType() const { return _blockType; }
    int logBlocks() const { return _logBlocks; }
    int logPointsPerBlockOffset() const { return _logPointsPerBlockOffset; }
private:


        if (_logBlocksPerGroup != 0)
            printf("createGrid() called on non-block\n");


            IncompleteLeaf* leaf = _list.getNext();
            bool found = false;
            while (leaf != 0) {
                if (leaf == newLeaf)
                    found = true;
                leaf = _list.getNext(leaf);
            }
            if (!found) {
                printf("leaf 0x%08x not in queue!\n",newLeaf);
                printf("Leaves in queue\n");
                IncompleteLeaf* leaf = _list.getNext();
                bool found = false;
                while (leaf != 0) {
                    printf("  0x%08x\n",leaf);
                    leaf = _list.getNext(leaf);
                }
            }

            if (_grid->_gridType._blockType != gridBlockType)
                printf("Moving non-blockGrid grid to deleted\n");



//* Saw a crashing bug in shrink(). When deleting the old quadrants, the pointer
//  passed to deallocate() is not a grid but z data. Incorrect block type
//  somewhere?
//  * This clearly happens very rarely, so is likely a race condition. Maybe a
//    queue's current leaf is moved out from under it - the logPointsPerLeaf
//    value passed from moveFrom() to reseatLeaf() is incorrect. Put back the
//    debugging code for this, especially since we're

    //* Cache relative block point size in grid.
    //  * Actual block point size is (relative block point size) +
    //    (absolute point size of relative block point size 0).
    //
    //* When doing so, make _index the block point size relative to the parent
    //  block.
    //  * Need to have _index intact so that when a grid in the deletion queue is
    //    moved, we can update the pointer to it in the parent.
    //    * Given the tile size to texel size ratio of (256:1)^2, we only need
    //      16 bits for _index. Actually a bit more because of temporatily
    //      sub-texel leaves.
    //    * We need 6 bits for logPoints at most, leaving 26 bits for _index
    //      which should be plenty (if we have a 8192*8192 grid we have bigger
    //      problems!)


    // If _parent == 0 then this is instead log(points per block).
        else
            _index = 29;
        _root->_index = 29;


#define LONG_TIME 33333333 /*66666666*/
#define TIME_BEGIN(x) unsigned int timeBegin##x = static_cast<unsigned int>(__rdtsc())
#define TIME_END(x) unsigned int timeEnd##x = static_cast<unsigned int>(__rdtsc()) - timeBegin##x; if (timeEnd##x > LONG_TIME) printf(#x " %i\n",timeEnd##x/33333)

        printf("_zoomUnitX   = "); print(_zoomUnitX, _precision); printf("\n");
        printf("_zoomUnitY   = "); print(_zoomUnitY, _precision); printf("\n");

        TIME_BEGIN(doPaint);
        unsigned int time = static_cast<unsigned int>(__rdtsc());
        if (time-_lastTime > LONG_TIME) {
            printf("  total = %i\n",(time-_lastTime)/33333);
        }
        _lastTime = time;
        printf("%f\n",
            _processor->getIterationsPerSecond()/1000000.0f);

    unsigned int _lastTime;


            case WM_PAINT:
                {
                    TIME_BEGIN(WM_PAINT);
                    LRESULT r = Base::handleMessage(uMsg, wParam, lParam);
                    TIME_END(WM_PAINT);
                    return r;
                }

//* Reduce refresh rate to 15fps when not zooming or rotating?
//


    void consolidateSmallLeaves()
    {
        reset();
        moveTo(_consolidateSmallLeavesLocation);
        int logPointsPerTexel = _processor->screen()->logPointsPerTexel();
        do {
            if (_interrupt) {
                _consolidateSmallLeavesPending = true;
                _consolidateSmallLeavesLocation = _location;
                return;
            }
            while (blockType() == gridBlockType || _logBlocksPerGroup > 0)
                descendQuadrant();
            if (logPointsPerGroup() < logPointsPerTexel) {
                consolidateLeaf();
                continue;
            }
            do {
                ascendQuadrant();
                if (atTop()) {
                    _consolidateSmallLeavesLocation =
                        BlockLocation(Vector(0, 0), 28);
                    return;
                }
                if (atLastQuadrant())
                    continue;
                moveNextQuadrant();
                break;
            } while (true);
        } while (true);
    }

//* Change LinkedListMember<T> to LinkedListMembership<T>
//  * Need a way to get a T* from a LinkedListMembership<T>*
