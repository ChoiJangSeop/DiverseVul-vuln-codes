DeepScanLineInputFile::DeepScanLineInputFile
    (const char fileName[], int numThreads)
:
     _data (new Data (numThreads))
{
    _data->_streamData = new InputStreamMutex();
    _data->_deleteStream = true;
    OPENEXR_IMF_INTERNAL_NAMESPACE::IStream* is = 0;

    try
    {
        is = new StdIFStream (fileName);
        readMagicNumberAndVersionField(*is, _data->version);
        //
        // Backward compatibility to read multpart file.
        //
        if (isMultiPart(_data->version))
        {
            compatibilityInitialize(*is);
            return;
        }
        _data->_streamData->is = is;
        _data->memoryMapped = is->isMemoryMapped();
        _data->header.readFrom (*_data->_streamData->is, _data->version);
        _data->header.sanityCheck (isTiled (_data->version));

        initialize(_data->header);

        readLineOffsets (*_data->_streamData->is,
                         _data->lineOrder,
                         _data->lineOffsets,
                         _data->fileIsComplete);
    }
    catch (IEX_NAMESPACE::BaseExc &e)
    {
        if (is)          delete is;
        if (_data && _data->_streamData) delete _data->_streamData;
        if (_data)       delete _data;

        REPLACE_EXC (e, "Cannot read image file "
                     "\"" << fileName << "\". " << e.what());
        throw;
    }
    catch (...)
    {
        if (is)          delete is;
        if (_data && _data->_streamData) delete _data->_streamData;
        if (_data)       delete _data;

        throw;
    }
}