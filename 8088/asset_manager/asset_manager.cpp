// static const int assetCount = 99;  // Filled in by asset preparer

Word getLowestFreeSegment()
{
    // TODO
}

Word getEndOfConventionalMemorySegment()
{
    geninterrupt(0x12);  // returns amount of conventional RAM in 1kB blocks
    return _AX << 6;     // multiply by 64 to get segments
}

Word getCurrentTime()
{
    // TODO
}

struct AssetData
{
    Word segment;  // 0 for not loaded
    Word firstUsed;
    Word lastUsed;
    Word uncompressedLength; // in segments
    DWord diskOffset;
      // 0x80000000 = locked from moving
      // 0x40000000 = locked from discarding
    Byte decompressorAsset;
    Byte dictionaryAsset;
    Word previousAsset;  // or previous free segment, if it points to past the end of the asset table
};

class AssetManager
{
public:
    AssetManager()
    {
        tableSegment = getLowestFreeSegment();
        startSegment = tableSegment + assetCount;
        endSegment = getEndOfConventionalMemorySegment();
        firstFreeSegment = startSegment;
        // TODO: read asset data

    }
    Word getAsset(Word asset)
    {
        AssetData far* data = getAssetData(asset);
        // See if it's already loaded
        data->lastUsed = getCurrentTime();
        if (data->segment != 0)
            return data->segment;
        // See if we have room to load it

    }
private:
    AssetData far* getAssetData(Word asset)
    {
        return (AssetData far *)MK_FP(tableSegment + asset, 0);
    }

    Word tableSegment;
    Word startSegment;
    Word endSegment;
    Word firstFreeSegment;
};

// Types of asset:
//   Loaded but not compressed on disk
//     Initial decompressor
//     Dictionaries?
//   Normal (loaded, compressed on disk)
//   Generated (decompressor but no on-disk data - dictionary and diskOffset may be used as parameters)
//     read only data
//     read/write data - locked from unloading
//       BSS
//   Uninitialised
//     Stack
//     Allocated data
//   Disk cluster cache


// Do we need a segment header with the uncompressed data?
//   Probably not - keep all data for loaded assets in the AssetData arrays, and thread free areas together in a list
// How should we organise the data structures? Linked list? Tree?
// When program is loaded, load an initial set of assets as well (asset table, decompressor, dictionary needed to load the first thing)

/*
* Ideal asset manager for this sort of thing:
  * Keeps track of most recently used
    * Timestamps are 16-bit. 1 second resolution gives 18 hour span, 2 second resolution gives 36 hour span
      * If we run out of time, make timestamps in first half of span updated to midpoint.
  * When a new asset requested, all unlocked older asset pointers are invalidated
  * Preload all assets for a particular level
  * When we run out of memory, mark as "to unload" assets is LRU order until there will be enough room
    * Then work backwards towards LRU, unmarking assets if doing so would not cause there to be not enough room
  * If there is enough room but not enough contiguous room, compact
    * We want to try to keep assets at low addresses that are likely to stick around for a long time - the ones which have a long lifespan
      * So keep track of loaded time as well as last used
    * Try to find a gap to move the highest block into
      * If there is one, move it and try again
      * If there isn't one, mark as "to unload" a set of assets until we have a big enough space (then unmark any that we don't actually need to unload)
  * Game can hint that an old asset is unimportant by changing its load time to "just now".
  * Game can hint that a new asset will be needed again by changing its load time to old.
    * Same with compressed versions of assets
  * To take advantage of EMS/XMS, just use as a disk cache
    * What about intermediate, may-be-needed-again assets? Can move those to EMS/XMS from conventional memory
  * Use https://github.com/vkrasnov/dictator to make an initial dictionary so that we get compression on small items
  * Add an asynchronous load from floppy routine
  * We want to cache sectors so that a sector that spans assets doesn't need to reloaded when decompressing both
*/


/* Rejected:

// It'd be nice to not have to keep AssetData for entire game in memory at once - have directories?
//   Two level lookup for asset data - AssetData page and then item within page
//   Then pages loaded on demand
//   Only do this if we are spending too much RAM on AssetData - it slows everything down

*/
