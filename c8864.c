Header::sanityCheck (bool isTiled, bool isMultipartFile) const
{
    //
    // The display window and the data window must each
    // contain at least one pixel.  In addition, the
    // coordinates of the window corners must be small
    // enough to keep expressions like max-min+1 or
    // max+min from overflowing.
    //

    const Box2i &displayWindow = this->displayWindow();

    if (displayWindow.min.x > displayWindow.max.x ||
	displayWindow.min.y > displayWindow.max.y ||
	displayWindow.min.x <= -(INT_MAX / 2) ||
	displayWindow.min.y <= -(INT_MAX / 2) ||
	displayWindow.max.x >=  (INT_MAX / 2) ||
	displayWindow.max.y >=  (INT_MAX / 2))
    {
	throw IEX_NAMESPACE::ArgExc ("Invalid display window in image header.");
    }

    const Box2i &dataWindow = this->dataWindow();

    if (dataWindow.min.x > dataWindow.max.x ||
	dataWindow.min.y > dataWindow.max.y ||
	dataWindow.min.x <= -(INT_MAX / 2) ||
	dataWindow.min.y <= -(INT_MAX / 2) ||
	dataWindow.max.x >=  (INT_MAX / 2) ||
	dataWindow.max.y >=  (INT_MAX / 2))
    {
	throw IEX_NAMESPACE::ArgExc ("Invalid data window in image header.");
    }

    int w = (dataWindow.max.x - dataWindow.min.x + 1);
    if (maxImageWidth > 0 && maxImageWidth < w)
    {
	THROW (IEX_NAMESPACE::ArgExc, "The width of the data window exceeds the "
			    "maximum width of " << maxImageWidth << "pixels.");
    }

    int h = (dataWindow.max.y - dataWindow.min.y + 1);
    if (maxImageHeight > 0 && maxImageHeight < h)
    {
	THROW (IEX_NAMESPACE::ArgExc, "The height of the data window exceeds the "
			    "maximum height of " << maxImageHeight << "pixels.");
    }

    // chunk table must be smaller than the maximum image area
    // (only reachable for unknown types or damaged files: will have thrown earlier
    //  for regular image types)
    if( maxImageHeight>0 && maxImageWidth>0 && 
	hasChunkCount() && static_cast<Int64>(chunkCount())>Int64(maxImageWidth)*Int64(maxImageHeight))
    {
	THROW (IEX_NAMESPACE::ArgExc, "chunkCount exceeds maximum area of "
	       << Int64(maxImageWidth)*Int64(maxImageHeight) << " pixels." );
       
    }


    //
    // The pixel aspect ratio must be greater than 0.
    // In applications, numbers like the the display or
    // data window dimensions are likely to be multiplied
    // or divided by the pixel aspect ratio; to avoid
    // arithmetic exceptions, we limit the pixel aspect
    // ratio to a range that is smaller than theoretically
    // possible (real aspect ratios are likely to be close
    // to 1.0 anyway).
    //

    float pixelAspectRatio = this->pixelAspectRatio();

    const float MIN_PIXEL_ASPECT_RATIO = 1e-6f;
    const float MAX_PIXEL_ASPECT_RATIO = 1e+6f;

    if (!std::isnormal(pixelAspectRatio) ||
        pixelAspectRatio < MIN_PIXEL_ASPECT_RATIO ||
        pixelAspectRatio > MAX_PIXEL_ASPECT_RATIO)
    {
        throw IEX_NAMESPACE::ArgExc ("Invalid pixel aspect ratio in image header.");
    }

    //
    // The screen window width must not be less than 0.
    // The size of the screen window can vary over a wide
    // range (fish-eye lens to astronomical telescope),
    // so we can't limit the screen window width to a
    // small range.
    //

    float screenWindowWidth = this->screenWindowWidth();

    if (screenWindowWidth < 0)
	throw IEX_NAMESPACE::ArgExc ("Invalid screen window width in image header.");

    //
    // If the file has multiple parts, verify that each header has attribute
    // name and type.
    // (TODO) We may want to check more stuff here.
    //

    if (isMultipartFile)
    {
        if (!hasName())
        {
            throw IEX_NAMESPACE::ArgExc ("Headers in a multipart file should"
                               " have name attribute.");
        }

        if (!hasType())
        {
            throw IEX_NAMESPACE::ArgExc ("Headers in a multipart file should"
                               " have type attribute.");
        }

    }
    
    const std::string & part_type=hasType() ? type() : "";
    
    if(part_type!="" && !isSupportedType(part_type))
    {
        //
        // skip remaining sanity checks with unsupported types - they may not hold
        //
        return;
    }
    
   
    //
    // If the file is tiled, verify that the tile description has reasonable
    // values and check to see if the lineOrder is one of the predefined 3.
    // If the file is not tiled, then the lineOrder can only be INCREASING_Y
    // or DECREASING_Y.
    //

    LineOrder lineOrder = this->lineOrder();

    if (isTiled)
    {
	if (!hasTileDescription())
	{
	    throw IEX_NAMESPACE::ArgExc ("Tiled image has no tile "
			       "description attribute.");
	}

	const TileDescription &tileDesc = tileDescription();

	if (tileDesc.xSize <= 0 || tileDesc.ySize <= 0)
	    throw IEX_NAMESPACE::ArgExc ("Invalid tile size in image header.");

	if (maxTileWidth > 0 &&
	    maxTileWidth < int(tileDesc.xSize))
	{
	    THROW (IEX_NAMESPACE::ArgExc, "The width of the tiles exceeds the maximum "
				"width of " << maxTileWidth << "pixels.");
	}

	if (maxTileHeight > 0 &&
	    maxTileHeight < int(tileDesc.ySize))
	{
	    THROW (IEX_NAMESPACE::ArgExc, "The width of the tiles exceeds the maximum "
				"width of " << maxTileHeight << "pixels.");
	}

    // computes size of chunk offset table. Throws an exception if this exceeds
    // the maximum allowable size
    getTiledChunkOffsetTableSize(*this);

	if (tileDesc.mode != ONE_LEVEL &&
	    tileDesc.mode != MIPMAP_LEVELS &&
	    tileDesc.mode != RIPMAP_LEVELS)
	    throw IEX_NAMESPACE::ArgExc ("Invalid level mode in image header.");

	if (tileDesc.roundingMode != ROUND_UP &&
	    tileDesc.roundingMode != ROUND_DOWN)
	    throw IEX_NAMESPACE::ArgExc ("Invalid level rounding mode in image header.");

	if (lineOrder != INCREASING_Y &&
	    lineOrder != DECREASING_Y &&
	    lineOrder != RANDOM_Y)
	    throw IEX_NAMESPACE::ArgExc ("Invalid line order in image header.");
    }
    else
    {
        if (lineOrder != INCREASING_Y &&
            lineOrder != DECREASING_Y)
            throw IEX_NAMESPACE::ArgExc ("Invalid line order in image header.");
        
        
    }

    //
    // The compression method must be one of the predefined values.
    //

    if (!isValidCompression (this->compression()))
  	throw IEX_NAMESPACE::ArgExc ("Unknown compression type in image header.");
    
    if(isDeepData(part_type))
    {
        if (!isValidDeepCompression (this->compression()))
            throw IEX_NAMESPACE::ArgExc ("Compression type in header not valid for deep data");
    }

    //
    // Check the channel list:
    //
    // If the file is tiled then for each channel, the type must be one of the
    // predefined values, and the x and y sampling must both be 1.
    //
    // If the file is not tiled then for each channel, the type must be one
    // of the predefined values, the x and y coordinates of the data window's
    // upper left corner must be divisible by the x and y subsampling factors,
    // and the width and height of the data window must be divisible by the
    // x and y subsampling factors.
    //

    const ChannelList &channels = this->channels();
    
    if (isTiled)
    {
	for (ChannelList::ConstIterator i = channels.begin();
	     i != channels.end();
	     ++i)
	{
	    if (i.channel().type != OPENEXR_IMF_INTERNAL_NAMESPACE::UINT &&
		    i.channel().type != OPENEXR_IMF_INTERNAL_NAMESPACE::HALF &&
		    i.channel().type != OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT)
	    {
		THROW (IEX_NAMESPACE::ArgExc, "Pixel type of \"" << i.name() << "\" "
			            "image channel is invalid.");
	    }

	    if (i.channel().xSampling != 1)
	    {
		THROW (IEX_NAMESPACE::ArgExc, "The x subsampling factor for the "
				    "\"" << i.name() << "\" channel "
				    "is not 1.");
	    }	

	    if (i.channel().ySampling != 1)
	    {
		THROW (IEX_NAMESPACE::ArgExc, "The y subsampling factor for the "
				    "\"" << i.name() << "\" channel "
				    "is not 1.");
	    }	
	}
    }
    else
    {
	for (ChannelList::ConstIterator i = channels.begin();
	     i != channels.end();
	     ++i)
	{
	    if (i.channel().type != OPENEXR_IMF_INTERNAL_NAMESPACE::UINT &&
		    i.channel().type != OPENEXR_IMF_INTERNAL_NAMESPACE::HALF &&
		    i.channel().type != OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT)
	    {
		THROW (IEX_NAMESPACE::ArgExc, "Pixel type of \"" << i.name() << "\" "
			            "image channel is invalid.");
	    }

	    if (i.channel().xSampling < 1)
	    {
		THROW (IEX_NAMESPACE::ArgExc, "The x subsampling factor for the "
				    "\"" << i.name() << "\" channel "
				    "is invalid.");
	    }

	    if (i.channel().ySampling < 1)
	    {
		THROW (IEX_NAMESPACE::ArgExc, "The y subsampling factor for the "
				    "\"" << i.name() << "\" channel "
				    "is invalid.");
	    }

	    if (dataWindow.min.x % i.channel().xSampling)
	    {
		THROW (IEX_NAMESPACE::ArgExc, "The minimum x coordinate of the "
				    "image's data window is not a multiple "
				    "of the x subsampling factor of "
				    "the \"" << i.name() << "\" channel.");
	    }

	    if (dataWindow.min.y % i.channel().ySampling)
	    {
		THROW (IEX_NAMESPACE::ArgExc, "The minimum y coordinate of the "
				    "image's data window is not a multiple "
				    "of the y subsampling factor of "
				    "the \"" << i.name() << "\" channel.");
	    }

	    if ((dataWindow.max.x - dataWindow.min.x + 1) %
		    i.channel().xSampling)
	    {
		THROW (IEX_NAMESPACE::ArgExc, "Number of pixels per row in the "
				    "image's data window is not a multiple "
				    "of the x subsampling factor of "
				    "the \"" << i.name() << "\" channel.");
	    }

	    if ((dataWindow.max.y - dataWindow.min.y + 1) %
		    i.channel().ySampling)
	    {
		THROW (IEX_NAMESPACE::ArgExc, "Number of pixels per column in the "
				    "image's data window is not a multiple "
				    "of the y subsampling factor of "
				    "the \"" << i.name() << "\" channel.");
	    }
	}
    }
}