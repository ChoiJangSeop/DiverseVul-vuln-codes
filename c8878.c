void ScanLineInputFile::initialize(const Header& header)
{
    try
    {
        _data->header = header;

        _data->lineOrder = _data->header.lineOrder();

        const Box2i &dataWindow = _data->header.dataWindow();

        _data->minX = dataWindow.min.x;
        _data->maxX = dataWindow.max.x;
        _data->minY = dataWindow.min.y;
        _data->maxY = dataWindow.max.y;

        size_t maxBytesPerLine = bytesPerLineTable (_data->header,
                                                    _data->bytesPerLine);

        for (size_t i = 0; i < _data->lineBuffers.size(); i++)
        {
            _data->lineBuffers[i] = new LineBuffer (newCompressor
                                                (_data->header.compression(),
                                                 maxBytesPerLine,
                                                 _data->header));
        }

        _data->linesInBuffer =
            numLinesInBuffer (_data->lineBuffers[0]->compressor);

        _data->lineBufferSize = maxBytesPerLine * _data->linesInBuffer;

        if (!_streamData->is->isMemoryMapped())
        {
            for (size_t i = 0; i < _data->lineBuffers.size(); i++)
            {
                _data->lineBuffers[i]->buffer = (char *) EXRAllocAligned(_data->lineBufferSize*sizeof(char),16);
            }
        }
        _data->nextLineBufferMinY = _data->minY - 1;

        offsetInLineBufferTable (_data->bytesPerLine,
                                 _data->linesInBuffer,
                                 _data->offsetInLineBuffer);

        int lineOffsetSize = (dataWindow.max.y - dataWindow.min.y +
                              _data->linesInBuffer) / _data->linesInBuffer;

        _data->lineOffsets.resize (lineOffsetSize);
    }
    catch (...)
    {
        delete _data;
        _data=NULL;
        throw;
    }
}