DeepTiledInputFile::Data::Data (int numThreads):
    numXTiles (0),
    numYTiles (0),
    partNumber (-1),
    multiPartBackwardSupport(false),
    numThreads(numThreads),
    memoryMapped(false),
    _streamData(NULL),
    _deleteStream(false)
{
    //
    // We need at least one tileBuffer, but if threading is used,
    // to keep n threads busy we need 2*n tileBuffers
    //

    tileBuffers.resize (max (1, 2 * numThreads));
}