bufferedReadPixels (InputFile::Data* ifd, int scanLine1, int scanLine2)
{
    //
    // bufferedReadPixels reads each row of tiles that intersect the
    // scan-line range (scanLine1 to scanLine2). The previous row of
    // tiles is cached in order to prevent redundent tile reads when
    // accessing scanlines sequentially.
    //

    int minY = std::min (scanLine1, scanLine2);
    int maxY = std::max (scanLine1, scanLine2);

    if (minY < ifd->minY || maxY >  ifd->maxY)
    {
        throw IEX_NAMESPACE::ArgExc ("Tried to read scan line outside "
			   "the image file's data window.");
    }

    //
    // The minimum and maximum y tile coordinates that intersect this
    // scanline range
    //

    int minDy = (minY - ifd->minY) / ifd->tFile->tileYSize();
    int maxDy = (maxY - ifd->minY) / ifd->tFile->tileYSize();

    //
    // Figure out which one is first in the file so we can read without seeking
    //

    int yStart, yEnd, yStep;

    if (ifd->lineOrder == DECREASING_Y)
    {
        yStart = maxDy;
        yEnd = minDy - 1;
        yStep = -1;
    }
    else
    {
        yStart = minDy;
        yEnd = maxDy + 1;
        yStep = 1;
    }

    //
    // the number of pixels in a row of tiles
    //

    Box2i levelRange = ifd->tFile->dataWindowForLevel(0);
    
    //
    // Read the tiles into our temporary framebuffer and copy them into
    // the user's buffer
    //

    for (int j = yStart; j != yEnd; j += yStep)
    {
        Box2i tileRange = ifd->tFile->dataWindowForTile (0, j, 0);

        int minYThisRow = std::max (minY, tileRange.min.y);
        int maxYThisRow = std::min (maxY, tileRange.max.y);

        if (j != ifd->cachedTileY)
        {
            //
            // We don't have any valid buffered info, so we need to read in
            // from the file.
            //

            ifd->tFile->readTiles (0, ifd->tFile->numXTiles (0) - 1, j, j);
            ifd->cachedTileY = j;
        }

        //
        // Copy the data from our cached framebuffer into the user's
        // framebuffer.
        //

        for (FrameBuffer::ConstIterator k = ifd->cachedBuffer->begin();
             k != ifd->cachedBuffer->end();
             ++k)
        {
            Slice fromSlice = k.slice();		// slice to write from
            Slice toSlice = ifd->tFileBuffer[k.name()];	// slice to write to

            char *fromPtr, *toPtr;
            int size = pixelTypeSize (toSlice.type);

	    int xStart = levelRange.min.x;
	    int yStart = minYThisRow;

	    while (modp (xStart, toSlice.xSampling) != 0)
		++xStart;

	    while (modp (yStart, toSlice.ySampling) != 0)
		++yStart;


            intptr_t fromBase = reinterpret_cast<intptr_t>(fromSlice.base);
            intptr_t toBase = reinterpret_cast<intptr_t>(toSlice.base);

            for (int y = yStart;
		 y <= maxYThisRow;
		 y += toSlice.ySampling)
            {
		//
                // Set the pointers to the start of the y scanline in
                // this row of tiles
		//
                fromPtr = reinterpret_cast<char*> (fromBase +
                          (y - tileRange.min.y) * fromSlice.yStride +
                          xStart * fromSlice.xStride);

                toPtr = reinterpret_cast<char*> (toBase +
                        divp (y, toSlice.ySampling) * toSlice.yStride +
                        divp (xStart, toSlice.xSampling) * toSlice.xStride);

		//
                // Copy all pixels for the scanline in this row of tiles
		//

                for (int x = xStart;
		     x <= levelRange.max.x;
		     x += toSlice.xSampling)
                {
		    for (int i = 0; i < size; ++i)
			toPtr[i] = fromPtr[i];

		    fromPtr += fromSlice.xStride * toSlice.xSampling;
		    toPtr += toSlice.xStride;
                }
            }
        }
    }
}