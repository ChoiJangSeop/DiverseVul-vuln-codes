InputFile::setFrameBuffer (const FrameBuffer &frameBuffer)
{
    if (_data->isTiled)
    {
	Lock lock (*_data);

	//
        // We must invalidate the cached buffer if the new frame
	// buffer has a different set of channels than the old
	// frame buffer, or if the type of a channel has changed.
	//

	const FrameBuffer &oldFrameBuffer = _data->tFileBuffer;

	FrameBuffer::ConstIterator i = oldFrameBuffer.begin();
	FrameBuffer::ConstIterator j = frameBuffer.begin();

	while (i != oldFrameBuffer.end() && j != frameBuffer.end())
	{
	    if (strcmp (i.name(), j.name()) || i.slice().type != j.slice().type)
		break;

	    ++i;
	    ++j;
	}

	if (i != oldFrameBuffer.end() || j != frameBuffer.end())
        {
	    //
	    // Invalidate the cached buffer.
	    //

            _data->deleteCachedBuffer ();
	    _data->cachedTileY = -1;

	    //
	    // Create new a cached frame buffer.  It can hold a single
	    // row of tiles.  The cached buffer can be reused for each
	    // row of tiles because we set the yTileCoords parameter of
	    // each Slice to true.
	    //

	    const Box2i &dataWindow = _data->header.dataWindow();
	    _data->cachedBuffer = new FrameBuffer();
	    _data->offset = dataWindow.min.x;
	    
	    unsigned int tileRowSize =
                uiMult(dataWindow.max.x - dataWindow.min.x + 1U,
                       _data->tFile->tileYSize());

	    for (FrameBuffer::ConstIterator k = frameBuffer.begin();
		 k != frameBuffer.end();
		 ++k)
	    {
		Slice s = k.slice();

		switch (s.type)
		{
		  case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:

		    _data->cachedBuffer->insert
			(k.name(),
			 Slice (UINT,
				(char *)(new unsigned int[tileRowSize] - 
					_data->offset),
				sizeof (unsigned int),
				sizeof (unsigned int) *
				    _data->tFile->levelWidth(0),
				1, 1,
				s.fillValue,
				false, true));
		    break;

		  case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

		    _data->cachedBuffer->insert
			(k.name(),
			 Slice (HALF,
				(char *)(new half[tileRowSize] - 
					_data->offset),
				sizeof (half),
				sizeof (half) *
				    _data->tFile->levelWidth(0),
				1, 1,
				s.fillValue,
				false, true));
		    break;

		  case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

		    _data->cachedBuffer->insert
			(k.name(),
			 Slice (OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT,
				(char *)(new float[tileRowSize] - 
					_data->offset),
				sizeof(float),
				sizeof(float) *
				    _data->tFile->levelWidth(0),
				1, 1,
				s.fillValue,
				false, true));
		    break;

		  default:

		    throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
		}
	    }

	    _data->tFile->setFrameBuffer (*_data->cachedBuffer);
        }

	_data->tFileBuffer = frameBuffer;
    }
    else if(_data->compositor)
    {
        _data->compositor->setFrameBuffer(frameBuffer);
    }else {
        _data->sFile->setFrameBuffer(frameBuffer);
        _data->tFileBuffer = frameBuffer;
    }
}