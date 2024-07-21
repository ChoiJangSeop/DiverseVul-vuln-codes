getScanlineChunkOffsetTableSize(const Header& header)
{
    const Box2i &dataWindow = header.dataWindow();

    vector<size_t> bytesPerLine;
    size_t maxBytesPerLine = bytesPerLineTable (header,
                                                bytesPerLine);

    Compressor* compressor = newCompressor(header.compression(),
                                           maxBytesPerLine,
                                           header);

    int linesInBuffer = numLinesInBuffer (compressor);

    int lineOffsetSize = (dataWindow.max.y - dataWindow.min.y +
                          linesInBuffer) / linesInBuffer;

    delete compressor;

    return lineOffsetSize;
}