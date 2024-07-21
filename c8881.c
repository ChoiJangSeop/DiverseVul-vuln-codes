void DeepScanLineInputFile::initialize(const Header& header)
{
    try
    {
        if (header.type() != DEEPSCANLINE)
            throw IEX_NAMESPACE::ArgExc("Can't build a DeepScanLineInputFile from "
            "a type-mismatched part.");
        
        if(header.version()!=1)
        {
            THROW(IEX_NAMESPACE::ArgExc, "Version " << header.version() << " not supported for deepscanline images in this version of the library");
        }
        
        _data->header = header;

        _data->lineOrder = _data->header.lineOrder();

        const Box2i &dataWindow = _data->header.dataWindow();

        _data->minX = dataWindow.min.x;
        _data->maxX = dataWindow.max.x;
        _data->minY = dataWindow.min.y;
        _data->maxY = dataWindow.max.y;

        _data->sampleCount.resizeErase(_data->maxY - _data->minY + 1,
                                       _data->maxX - _data->minX + 1);
        _data->lineSampleCount.resizeErase(_data->maxY - _data->minY + 1);

        Compressor* compressor = newCompressor(_data->header.compression(),
                                               0,
                                               _data->header);

        _data->linesInBuffer = numLinesInBuffer (compressor);

        delete compressor;

        _data->nextLineBufferMinY = _data->minY - 1;

        int lineOffsetSize = (dataWindow.max.y - dataWindow.min.y +
                              _data->linesInBuffer) / _data->linesInBuffer;

        _data->lineOffsets.resize (lineOffsetSize);

        for (size_t i = 0; i < _data->lineBuffers.size(); i++)
            _data->lineBuffers[i] = new LineBuffer ();

        _data->gotSampleCount.resizeErase(_data->maxY - _data->minY + 1);
        for (int i = 0; i < _data->maxY - _data->minY + 1; i++)
            _data->gotSampleCount[i] = false;

        _data->maxSampleCountTableSize = min(_data->linesInBuffer, _data->maxY - _data->minY + 1) *
                                        (_data->maxX - _data->minX + 1) *
                                        sizeof(unsigned int);

        _data->sampleCountTableBuffer.resizeErase(_data->maxSampleCountTableSize);

        _data->sampleCountTableComp = newCompressor(_data->header.compression(),
                                                    _data->maxSampleCountTableSize,
                                                    _data->header);

        _data->bytesPerLine.resize (_data->maxY - _data->minY + 1);
        
        const ChannelList & c=header.channels();
        
        _data->combinedSampleSize=0;
        for(ChannelList::ConstIterator i=c.begin();i!=c.end();i++)
        {
            switch(i.channel().type)
            {
                case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF  :
                    _data->combinedSampleSize+=Xdr::size<half>();
                    break;
                case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT :
                    _data->combinedSampleSize+=Xdr::size<float>();
                    break;
                case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT  :
                    _data->combinedSampleSize+=Xdr::size<unsigned int>();
                    break;
                default :
                    THROW(IEX_NAMESPACE::ArgExc, "Bad type for channel " << i.name() << " initializing deepscanline reader");
                    
            }
        }
        
    }
    catch (...)
    {
        delete _data;
        _data=NULL;
        throw;
    }
}