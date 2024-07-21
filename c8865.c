TiledInputFile::TiledInputFile (OPENEXR_IMF_INTERNAL_NAMESPACE::IStream &is, int numThreads):
    _data (new Data (numThreads))
{
    _data->_deleteStream=false;
    //
    // This constructor is called when a user
    // explicitly wants to read a tiled file.
    //

    bool streamDataCreated = false;

    try
    {
	readMagicNumberAndVersionField(is, _data->version);

	//
	// Backward compatibility to read multpart file.
	//
	if (isMultiPart(_data->version))
        {
	    compatibilityInitialize(is);
            return;
        }

	streamDataCreated = true;
	_data->_streamData = new InputStreamMutex();
	_data->_streamData->is = &is;
	_data->header.readFrom (*_data->_streamData->is, _data->version);
	initialize();
        // file is guaranteed to be single part, regular image
        _data->tileOffsets.readFrom (*(_data->_streamData->is), _data->fileIsComplete,false,false);
	_data->memoryMapped = _data->_streamData->is->isMemoryMapped();
	_data->_streamData->currentPosition = _data->_streamData->is->tellg();
    }
    catch (IEX_NAMESPACE::BaseExc &e)
    {
        if (streamDataCreated) delete _data->_streamData;
	delete _data;

	REPLACE_EXC (e, "Cannot open image file "
                 "\"" << is.fileName() << "\". " << e.what());
	throw;
    }
    catch (...)
    {
        if (streamDataCreated) delete _data->_streamData;
	delete _data;
        throw;
    }
}