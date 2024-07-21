B44Compressor::uncompress (const char *inPtr,
			   int inSize,
			   IMATH_NAMESPACE::Box2i range,
			   const char *&outPtr)
{
    //
    // This function is the reverse of the compress() function,
    // above.  First all pixels are moved from the input buffer
    // into _tmpBuffer.  UINT and FLOAT channels are copied
    // verbatim; HALF channels are uncompressed in blocks of
    // 4x4 pixels.  Then the pixels in _tmpBuffer are copied
    // into the output buffer and rearranged such that the data
    // for for each scan line form a contiguous block.
    //

    outPtr = _outBuffer;

    if (inSize == 0)
    {
	return 0;
    }

    int minX = range.min.x;
    int maxX = min (range.max.x, _maxX);
    int minY = range.min.y;
    int maxY = min (range.max.y, _maxY);
    
    unsigned short *tmpBufferEnd = _tmpBuffer;
    int i = 0;

    for (ChannelList::ConstIterator c = _channels.begin();
	 c != _channels.end();
	 ++c, ++i)
    {
	ChannelData &cd = _channelData[i];

	cd.start = tmpBufferEnd;
	cd.end = cd.start;

	cd.nx = numSamples (c.channel().xSampling, minX, maxX);
	cd.ny = numSamples (c.channel().ySampling, minY, maxY);

	tmpBufferEnd += cd.nx * cd.ny * cd.size;
    }

    for (int i = 0; i < _numChans; ++i)
    {
	ChannelData &cd = _channelData[i];

	if (cd.type != HALF)
	{
	    //
	    // UINT or FLOAT channel.
	    //

	    int n = cd.nx * cd.ny * cd.size * sizeof (unsigned short);

	    if (inSize < n)
		notEnoughData();

	    memcpy (cd.start, inPtr, n);
	    inPtr += n;
	    inSize -= n;

	    continue;
	}

	//
	// HALF channel
	//

	for (int y = 0; y < cd.ny; y += 4)
	{
	    unsigned short *row0 = cd.start + y * cd.nx;
	    unsigned short *row1 = row0 + cd.nx;
	    unsigned short *row2 = row1 + cd.nx;
	    unsigned short *row3 = row2 + cd.nx;

	    for (int x = 0; x < cd.nx; x += 4)
	    {
		unsigned short s[16]; 

		if (inSize < 3)
		    notEnoughData();

		if (((const unsigned char *)inPtr)[2] == 0xfc)
		{
		    unpack3 ((const unsigned char *)inPtr, s);
		    inPtr += 3;
		    inSize -= 3;
		}
		else
		{
		    if (inSize < 14)
			notEnoughData();

		    unpack14 ((const unsigned char *)inPtr, s);
		    inPtr += 14;
		    inSize -= 14;
		}

		if (cd.pLinear)
		    convertToLinear (s);

		int n = (x + 3 < cd.nx)?
			    4 * sizeof (unsigned short) :
			    (cd.nx - x) * sizeof (unsigned short);

		if (y + 3 < cd.ny)
		{
		    memcpy (row0, &s[ 0], n);
		    memcpy (row1, &s[ 4], n);
		    memcpy (row2, &s[ 8], n);
		    memcpy (row3, &s[12], n);
		}
		else
		{
		    memcpy (row0, &s[ 0], n);

		    if (y + 1 < cd.ny)
			memcpy (row1, &s[ 4], n);

		    if (y + 2 < cd.ny)
			memcpy (row2, &s[ 8], n);
		}

		row0 += 4;
		row1 += 4;
		row2 += 4;
		row3 += 4;
	    }
	}
    }

    char *outEnd = _outBuffer;

    if (_format == XDR)
    {
	for (int y = minY; y <= maxY; ++y)
	{
	    for (int i = 0; i < _numChans; ++i)
	    {
		ChannelData &cd = _channelData[i];

		if (modp (y, cd.ys) != 0)
		    continue;

		if (cd.type == HALF)
		{
		    for (int x = cd.nx; x > 0; --x)
		    {
			Xdr::write <CharPtrIO> (outEnd, *cd.end);
			++cd.end;
		    }
		}
		else
		{
		    int n = cd.nx * cd.size;
		    memcpy (outEnd, cd.end, n * sizeof (unsigned short));
		    outEnd += n * sizeof (unsigned short);
		    cd.end += n;
		}
	    }
	}
    }
    else
    {
	for (int y = minY; y <= maxY; ++y)
	{
	    for (int i = 0; i < _numChans; ++i)
	    {
		ChannelData &cd = _channelData[i];

		#if defined (DEBUG)
		    assert (cd.type == HALF);
		#endif

		if (modp (y, cd.ys) != 0)
		    continue;

		int n = cd.nx * cd.size;
		memcpy (outEnd, cd.end, n * sizeof (unsigned short));
		outEnd += n * sizeof (unsigned short);
		cd.end += n;
	    }
	}
    }

    #if defined (DEBUG)

	for (int i = 1; i < _numChans; ++i)
	    assert (_channelData[i-1].end == _channelData[i].start);

	assert (_channelData[_numChans-1].end == tmpBufferEnd);

    #endif

    if (inSize > 0)
	tooMuchData();

    outPtr = _outBuffer;
    return outEnd - _outBuffer;
}