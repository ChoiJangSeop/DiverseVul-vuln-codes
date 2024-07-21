TiledInputFile::TiledInputFile (const Header &header,
                                OPENEXR_IMF_INTERNAL_NAMESPACE::IStream *is,
                                int version,
                                int numThreads) :
    _data (new Data (numThreads))
{
    _data->_deleteStream=false;
    _data->_streamData = new InputStreamMutex();
    //
    // This constructor called by class Imf::InputFile
    // when a user wants to just read an image file, and
    // doesn't care or know if the file is tiled.
    // No need to have backward compatibility here, because
    // we have somehow got the header.
    //

    _data->_streamData->is = is;
    _data->header = header;
    _data->version = version;
    initialize();
    _data->tileOffsets.readFrom (*(_data->_streamData->is),_data->fileIsComplete,false,false);
    _data->memoryMapped = is->isMemoryMapped();
    _data->_streamData->currentPosition = _data->_streamData->is->tellg();
}