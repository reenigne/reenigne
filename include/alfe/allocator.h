#include "alfe/main.h"

#ifndef INCLUDED_ALLOCATOR_H
#define INCLUDED_ALLOCATOR_H

#include <map>

// This class allocates and frees memory.
//
// malloc() and free() have a severe problem for this application - they assume
// that a block, once allocated, cannot be moved. This causes a high degree of
// fragmentation in the common case that we allocate a large number of blocks
// and then remove most of them, since the remainder will cause most of the
// memory pages to remain committed.
//
// However, many of our blocks are quite easy to move, given just a pointer to
// it, because we know where all pointers that point to it are, and we know
// that no other threads will be accessing it. Also, the blocks of interest
// come in a small number of sizes (~20 in practice). This means it is
// practical to use a separate compacting heap for each block size, and keep
// these heaps compacted by moving the last block into the gap whenever a block
// is deallocated.
//
// These heaps grow in chunks of at least 1Mb and at least one block. Chunks
// need not be contiguous in memory, allowing the heaps to be interspersed.
template<class FractalProcessor> class Allocator : Uncopyable
{
public:
    Allocator()
    {
        SYSTEM_INFO systemInfo;
        GetSystemInfo(&systemInfo);
        _granularity = systemInfo.dwAllocationGranularity;
    }

    void setProcessor(FractalProcessor* processor) { _processor = processor; }

    // A heap for a particular size of block. This is implemented using
    // VirtualAlloc() and VirtualFree() instead of HeapAlloc() and HeapFree()
    // because the latter set of functions never release address space.
    class Heap
    {
        // A Chunk is a unit of allocation of address space.
        class ChunkHeader
        {
        public:
            ChunkHeader* _previous;
        };

        // Each block holds one object. BlockHeaders always start on a word
        // boundary, so the least significant bits will always be 0. Hence we
        // can re-use the least significant bit for a deleted flag.
        class BlockHeader
        {
        public:
            BlockHeader* _previous;
        };

    public:
        // We need a default constructor to be able to put a Heap in a
        // std::map. However, default-constructed Heaps are never actually
        // used.
        Heap() { }

        Heap(FractalProcessor* processor, Allocator* allocator, int size)
          : _blockSize(size + _blockHeaderSize),
            _chunk(0),
            _freeBytes(0),
            _block(0),
            _processor(processor),
            _allocator(allocator)
        {
            // We avoid allocating chunks less than 1Mb, but we also round up
            // to a full number of chunks to avoid wasting space.
            _chunkSize = ((0x100000 - _chunkHeaderSize) / _blockSize + 1)
                * _blockSize + _chunkHeaderSize;

            // Round up to the granularity. With 64Kb granularity, we allocate
            // 1Mb+64Kb for all grid sizes up to 512*512 for 4-byte blocks and
            // 32*32 for 56-byte blocks. This means we rarely allocate chunks
            // of any other size, which helps keep the address space
            // defragmented.
            _chunkSize = _allocator->align(_chunkSize);

            // This is the number of spare bytes at the end of each chunk, due
            // to alignment requirements.
            _slackBytes = (_chunkSize - _chunkHeaderSize) % _blockSize;
        }

        // Allocates a new block and returns a pointer to it.
        template<class T> T* allocate()
        {
            BlockHeader* newBlock;
            if (_blockSize > _freeBytes) {
                // Allocate a new chunk. We'll let VirtualAlloc choose where to
                // put it.
                ChunkHeader* chunk = reinterpret_cast<ChunkHeader*>(
                    VirtualAlloc(NULL, _chunkSize, MEM_COMMIT | MEM_RESERVE,
                        PAGE_READWRITE));
                IF_ZERO_CHECK_THROW_LAST_ERROR(chunk);
                if (chunk == 0)
                    return 0;
                _processor->tracker()->adjustMemory(_chunkSize);
                chunk->_previous = _chunk;
                _chunk = chunk;
                _freeBytes = _chunkSize - _chunkHeaderSize;
                newBlock = reinterpret_cast<BlockHeader*>(
                    reinterpret_cast<Byte*>(chunk) + _chunkHeaderSize);
            }
            else {
                // Allocate out of the current chunk.
                newBlock = reinterpret_cast<BlockHeader*>(
                    reinterpret_cast<Byte*>(_block) + _blockSize);
            }
            _freeBytes -= _blockSize;
            newBlock->_previous = _block;
            _block = newBlock;
            return object<T>();
        }

        // Deallocates the block at p. To avoid memory fragmentation and keep
        // memory usage to a minimum, the object highest in memory is moved to
        // p with a call to T::moveFrom().
        template<class T> void deallocate(T* p)
        {
            BlockHeader* block = reinterpret_cast<BlockHeader*>(
                reinterpret_cast<Byte*>(p) - _blockHeaderSize);
            if (block != _block) {
                T* old = object<T>();
                p->moveFrom(_processor, old);
            }
            _block = _block->_previous;
            _freeBytes += _blockSize;
            if (_freeBytes == _chunkSize - _chunkHeaderSize) {
                // We have an entirely empty chunk. Free it.
                ChunkHeader* oldChunk = _chunk;
                _chunk = _chunk->_previous;
                VirtualFree(oldChunk, 0, MEM_RELEASE);
                _freeBytes = _slackBytes;
                _processor->tracker()->adjustMemory(
                    -static_cast<int>(_chunkSize));
            }
        }

    private:
        template<class T> T* object() const
        {
            return reinterpret_cast<T*>(
                reinterpret_cast<Byte*>(_block) + _blockHeaderSize);
        }

        // Size of a block header.
        static const size_t _blockHeaderSize = 4;

        // Size of a chunk header.
        static const size_t _chunkHeaderSize = 4;

        // Size of a block.
        size_t _blockSize;

        // Size of a chunk.
        size_t _chunkSize;

        // Pointer to the chunk containing the most recently allocated block.
        ChunkHeader* _chunk;

        // Pointer to the most recently allocated block.
        BlockHeader* _block;

        // Number of bytes free in the current chunk.
        size_t _freeBytes;

        // Number of bytes free in a full chunk.
        size_t _slackBytes;

        FractalProcessor* _processor;

        Allocator* _allocator;
    };

    // Return a the heap that should be used for allocating blocks of size
    // "size".
    Heap* heapForSize(int size)
    {
        std::map<int, Heap>::iterator i = _heaps.find(size);
        if (i != _heaps.end())
            return &i->second;
        _heaps[size] = Heap(_processor, this, size);
        return &_heaps[size];
    }

    // Round a value up to the allocation granularity
    size_t align(size_t value)
    {
        return (value + _granularity - 1) & ~(_granularity - 1);
    }

private:
    // Allocation granularity. We always allocate multiples of this size, and
    // allocations are aligned to this size.
    size_t _granularity;

    std::map<int, Heap> _heaps;
    FractalProcessor* _processor;
};


#endif // INCLUDED_ALLOCATOR_H
