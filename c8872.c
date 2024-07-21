TiledInputFile::TiledInputFile (const char fileName[], int numThreads):
    _data (new Data (numThreads))
{
    _data->_streamData=NULL;
    _data->_deleteStream=true;
    
    //
    // This constructor is called when a user
    // explicitly wants to read a tiled file.
    //


    IStream* is = 0;
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

	_data->_streamData = new InputStreamMutex();
	_data->_streamData->is = is;
	_data->header.readFrom (*_data->_streamData->is, _data->version);
	initialize();
        //read tile offsets - we are not multipart or deep
        _data->tileOffsets.readFrom (*(_data->_streamData->is), _data->fileIsComplete,false,false);
	_data->_streamData->currentPosition = _data->_streamData->is->tellg();
    }
    catch (IEX_NAMESPACE::BaseExc &e)
    {
        if (_data->_streamData != 0)
        {
            if (_data->_streamData->is != 0)
            {
                delete _data->_streamData->is;
                _data->_streamData->is = is = 0;
            }

            delete _data->_streamData;
        }

        if (is != 0)
            delete is;

	REPLACE_EXC (e, "Cannot open image file "
                 "\"" << fileName << "\". " << e.what());
	throw;
    }
    catch (...)
    {
        if ( _data->_streamData != 0)
        {
            if ( _data->_streamData->is != 0)
            {
                delete _data->_streamData->is;
                _data->_streamData->is = is = 0;
            }

            delete _data->_streamData;
        }

        if (is != 0)
            delete is;
        throw;
    }
}