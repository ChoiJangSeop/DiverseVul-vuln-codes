ScanLineInputFile::ScanLineInputFile
    (const Header &header,
     OPENEXR_IMF_INTERNAL_NAMESPACE::IStream *is,
     int numThreads)
:
    _data (new Data (numThreads)),
    _streamData (new InputStreamMutex())
{
    _streamData->is = is;
    _data->memoryMapped = is->isMemoryMapped();

    initialize(header);
    
    //
    // (TODO) this is nasty - we need a better way of working out what type of file has been used.
    // in any case I believe this constructor only gets used with single part files
    // and 'version' currently only tracks multipart state, so setting to 0 (not multipart) works for us
    //
    
    _data->version=0;
    readLineOffsets (*_streamData->is,
                     _data->lineOrder,
                     _data->lineOffsets,
                     _data->fileIsComplete);
}