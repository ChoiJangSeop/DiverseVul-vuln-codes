DwaCompressor::uncompress
    (const char *inPtr,
     int inSize,
     Imath::Box2i range,
     const char *&outPtr)
{
    int minX = range.min.x;
    int maxX = std::min (range.max.x, _max[0]);
    int minY = range.min.y;
    int maxY = std::min (range.max.y, _max[1]);

    int headerSize = NUM_SIZES_SINGLE*sizeof(Int64);
    if (inSize < headerSize) 
    {
        throw Iex::InputExc("Error uncompressing DWA data"
                            "(truncated header).");
    }

    // 
    // Flip the counters from XDR to NATIVE
    //

    for (int i = 0; i < NUM_SIZES_SINGLE; ++i)
    {
        Int64      *dst =  (((Int64 *)inPtr) + i);
        const char *src = (char *)(((Int64 *)inPtr) + i);

        Xdr::read<CharPtrIO> (src, *dst);
    }

    //
    // Unwind all the counter info
    //

    const Int64 *inPtr64 = (const Int64*) inPtr;

    Int64 version                  = *(inPtr64 + VERSION);
    Int64 unknownUncompressedSize  = *(inPtr64 + UNKNOWN_UNCOMPRESSED_SIZE);
    Int64 unknownCompressedSize    = *(inPtr64 + UNKNOWN_COMPRESSED_SIZE);
    Int64 acCompressedSize         = *(inPtr64 + AC_COMPRESSED_SIZE);
    Int64 dcCompressedSize         = *(inPtr64 + DC_COMPRESSED_SIZE);
    Int64 rleCompressedSize        = *(inPtr64 + RLE_COMPRESSED_SIZE);
    Int64 rleUncompressedSize      = *(inPtr64 + RLE_UNCOMPRESSED_SIZE);
    Int64 rleRawSize               = *(inPtr64 + RLE_RAW_SIZE);
 
    Int64 totalAcUncompressedCount = *(inPtr64 + AC_UNCOMPRESSED_COUNT); 
    Int64 totalDcUncompressedCount = *(inPtr64 + DC_UNCOMPRESSED_COUNT); 

    Int64 acCompression            = *(inPtr64 + AC_COMPRESSION); 

    Int64 compressedSize           = unknownCompressedSize + 
                                     acCompressedSize +
                                     dcCompressedSize +
                                     rleCompressedSize;

    const char *dataPtr            = inPtr + NUM_SIZES_SINGLE * sizeof(Int64);

    if (inSize < headerSize + compressedSize) 
    {
        throw Iex::InputExc("Error uncompressing DWA data"
                            "(truncated file).");
    }

    if (unknownUncompressedSize < 0  || 
        unknownCompressedSize < 0    ||
        acCompressedSize < 0         || 
        dcCompressedSize < 0         ||
        rleCompressedSize < 0        || 
        rleUncompressedSize < 0      ||
        rleRawSize < 0               ||  
        totalAcUncompressedCount < 0 || 
        totalDcUncompressedCount < 0) 
    {
        throw Iex::InputExc("Error uncompressing DWA data"
                            " (corrupt header).");
    }

    if (version < 2) 
        initializeLegacyChannelRules();
    else
    {
        unsigned short ruleSize = 0;
        Xdr::read<CharPtrIO>(dataPtr, ruleSize);

        if (ruleSize < 0) 
            throw Iex::InputExc("Error uncompressing DWA data"
                                " (corrupt header file).");

        headerSize += ruleSize;
        if (inSize < headerSize + compressedSize)
            throw Iex::InputExc("Error uncompressing DWA data"
                                " (truncated file).");

        _channelRules.clear();
        ruleSize -= Xdr::size<unsigned short> ();
        while (ruleSize > 0) 
        {
            Classifier rule(dataPtr, ruleSize);
            
            _channelRules.push_back(rule);
            ruleSize -= rule.size();
        }
    }


    size_t outBufferSize = 0;
    initializeBuffers(outBufferSize);

    //
    // Allocate _outBuffer, if we haven't done so already
    //

    if (_maxScanLineSize * numScanLines() > _outBufferSize) 
    {
        _outBufferSize = _maxScanLineSize * numScanLines();
        if (_outBuffer != 0)
            delete[] _outBuffer;
        _outBuffer = new char[_maxScanLineSize * numScanLines()];
    }


    char *outBufferEnd = _outBuffer;

       
    //
    // Find the start of the RLE packed AC components and
    // the DC components for each channel. This will be handy   
    // if you want to decode the channels in parallel later on.
    //

    char *packedAcBufferEnd = 0; 

    if (_packedAcBuffer)
        packedAcBufferEnd = _packedAcBuffer;

    char *packedDcBufferEnd = 0;

    if (_packedDcBuffer)
        packedDcBufferEnd = _packedDcBuffer;

    //
    // UNKNOWN data is packed first, followed by the 
    // Huffman-compressed AC, then the DC values, 
    // and then the zlib compressed RLE data.
    //
    
    const char *compressedUnknownBuf = dataPtr;

    const char *compressedAcBuf      = compressedUnknownBuf + 
                                  static_cast<ptrdiff_t>(unknownCompressedSize);
    const char *compressedDcBuf      = compressedAcBuf +
                                  static_cast<ptrdiff_t>(acCompressedSize);
    const char *compressedRleBuf     = compressedDcBuf + 
                                  static_cast<ptrdiff_t>(dcCompressedSize);

    // 
    // Sanity check that the version is something we expect. Right now, 
    // we can decode version 0, 1, and 2. v1 adds 'end of block' symbols
    // to the AC RLE. v2 adds channel classification rules at the 
    // start of the data block.
    //

    if ((version < 0) || (version > 2))
        throw Iex::InputExc ("Invalid version of compressed data block");    

    setupChannelData(minX, minY, maxX, maxY);

    // 
    // Uncompress the UNKNOWN data into _planarUncBuffer[UNKNOWN]
    //

    if (unknownCompressedSize > 0)
    {
        uLongf outSize = static_cast<uLongf>(
                ceil( (float)unknownUncompressedSize * 1.01) + 100);

        if (unknownUncompressedSize < 0 || 
            outSize > _planarUncBufferSize[UNKNOWN]) 
        {
            throw Iex::InputExc("Error uncompressing DWA data"
                                "(corrupt header).");
        }

        if (Z_OK != ::uncompress
                        ((Bytef *)_planarUncBuffer[UNKNOWN],
                         &outSize,
                         (Bytef *)compressedUnknownBuf,
                         (uLong)unknownCompressedSize))
        {
            throw Iex::BaseExc("Error uncompressing UNKNOWN data.");
        }
    }

    // 
    // Uncompress the AC data into _packedAcBuffer
    //

    if (acCompressedSize > 0)
    {
        if (totalAcUncompressedCount*sizeof(unsigned short) > _packedAcBufferSize)
        {
            throw Iex::InputExc("Error uncompressing DWA data"
                                "(corrupt header).");
        }

        //
        // Don't trust the user to get it right, look in the file.
        //

        switch (acCompression)
        {
          case STATIC_HUFFMAN:

            hufUncompress
                (compressedAcBuf, 
                 (int)acCompressedSize, 
                 (unsigned short *)_packedAcBuffer, 
                 (int)totalAcUncompressedCount); 

            break;

          case DEFLATE:
            {
                uLongf destLen =
                    (int)(totalAcUncompressedCount) * sizeof (unsigned short);

                if (Z_OK != ::uncompress
                                ((Bytef *)_packedAcBuffer,
                                 &destLen,
                                 (Bytef *)compressedAcBuf,
                                 (uLong)acCompressedSize))
                {
                    throw Iex::InputExc ("Data decompression (zlib) failed.");
                }

                if (totalAcUncompressedCount * sizeof (unsigned short) !=
                                destLen)
                {
                    throw Iex::InputExc ("AC data corrupt.");     
                }
            }
            break;

          default:

            throw Iex::NoImplExc ("Unknown AC Compression");
            break;
        }
    }

    //
    // Uncompress the DC data into _packedDcBuffer
    //

    if (dcCompressedSize > 0)
    {
        if (totalDcUncompressedCount*sizeof(unsigned short) > _packedDcBufferSize)
        {
            throw Iex::InputExc("Error uncompressing DWA data"
                                "(corrupt header).");
        }

        if (_zip->uncompress
                    (compressedDcBuf, (int)dcCompressedSize, _packedDcBuffer)
            != (int)totalDcUncompressedCount * sizeof (unsigned short))
        {
            throw Iex::BaseExc("DC data corrupt.");
        }
    }

    //
    // Uncompress the RLE data into _rleBuffer, then unRLE the results
    // into _planarUncBuffer[RLE]
    //

    if (rleRawSize > 0)
    {
        if (rleUncompressedSize > _rleBufferSize ||
            rleRawSize > _planarUncBufferSize[RLE])
        {
            throw Iex::InputExc("Error uncompressing DWA data"
                                "(corrupt header).");
        }
 
        uLongf dstLen = (uLongf)rleUncompressedSize;

        if (Z_OK != ::uncompress
                        ((Bytef *)_rleBuffer,
                         &dstLen,
                         (Bytef *)compressedRleBuf,
                         (uLong)rleCompressedSize))
        {
            throw Iex::BaseExc("Error uncompressing RLE data.");
        }

        if (dstLen != rleUncompressedSize)
            throw Iex::BaseExc("RLE data corrupted");

        if (rleUncompress
                ((int)rleUncompressedSize, 
                 (int)rleRawSize,
                 (signed char *)_rleBuffer,
                 _planarUncBuffer[RLE]) != rleRawSize)
        {        
            throw Iex::BaseExc("RLE data corrupted");
        }
    }

    //
    // Determine the start of each row in the output buffer
    //

    std::vector<bool> decodedChannels (_channelData.size());
    std::vector< std::vector<char *> > rowPtrs (_channelData.size());

    for (unsigned int chan = 0; chan < _channelData.size(); ++chan)
        decodedChannels[chan] = false;

    outBufferEnd = _outBuffer;

    for (int y = minY; y <= maxY; ++y)
    {
        for (unsigned int chan = 0; chan < _channelData.size(); ++chan)
        {
            ChannelData *cd = &_channelData[chan];

            if (Imath::modp (y, cd->ySampling) != 0)
                continue;

            rowPtrs[chan].push_back (outBufferEnd);
            outBufferEnd += cd->width * Imf::pixelTypeSize (cd->type);
        }
    }

    //
    // Setup to decode each block of 3 channels that need to
    // be handled together
    //

    for (unsigned int csc = 0; csc < _cscSets.size(); ++csc)
    {
        int rChan = _cscSets[csc].idx[0];    
        int gChan = _cscSets[csc].idx[1];    
        int bChan = _cscSets[csc].idx[2];    


        LossyDctDecoderCsc decoder
            (rowPtrs[rChan],
             rowPtrs[gChan],
             rowPtrs[bChan],
             packedAcBufferEnd,
             packedDcBufferEnd,
             dwaCompressorToLinear,
             _channelData[rChan].width,
             _channelData[rChan].height,
             _channelData[rChan].type,
             _channelData[gChan].type,
             _channelData[bChan].type);

        decoder.execute();

        packedAcBufferEnd +=
            decoder.numAcValuesEncoded() * sizeof (unsigned short);

        packedDcBufferEnd +=
            decoder.numDcValuesEncoded() * sizeof (unsigned short);

        decodedChannels[rChan] = true;
        decodedChannels[gChan] = true;
        decodedChannels[bChan] = true;
    }

    //
    // Setup to handle the remaining channels by themselves
    //

    for (unsigned int chan = 0; chan < _channelData.size(); ++chan)
    {
        if (decodedChannels[chan])
            continue;

        ChannelData *cd = &_channelData[chan];
        int pixelSize = Imf::pixelTypeSize (cd->type);

        switch (cd->compression)
        {
          case LOSSY_DCT:

            //
            // Setup a single-channel lossy DCT decoder pointing
            // at the output buffer
            //

            {
                const unsigned short *linearLut = 0;

                if (!cd->pLinear)
                    linearLut = dwaCompressorToLinear;

                LossyDctDecoder decoder
                    (rowPtrs[chan],
                     packedAcBufferEnd,
                     packedDcBufferEnd,
                     linearLut,
                     cd->width,
                     cd->height,
                     cd->type);

                decoder.execute();   

                packedAcBufferEnd += 
                    decoder.numAcValuesEncoded() * sizeof (unsigned short);

                packedDcBufferEnd += 
                    decoder.numDcValuesEncoded() * sizeof (unsigned short);
            }

            break;

          case RLE:

            //
            // For the RLE case, the data has been un-RLE'd into
            // planarUncRleEnd[], but is still split out by bytes.
            // We need to rearrange the bytes back into the correct
            // order in the output buffer;
            //

            {
                int row = 0;

                for (int y = minY; y <= maxY; ++y)
                {
                    if (Imath::modp (y, cd->ySampling) != 0)
                        continue;

                    char *dst = rowPtrs[chan][row];

                    if (pixelSize == 2)
                    {
                        interleaveByte2 (dst, 
                                         cd->planarUncRleEnd[0],
                                         cd->planarUncRleEnd[1],
                                         cd->width);
                                            
                        cd->planarUncRleEnd[0] += cd->width;
                        cd->planarUncRleEnd[1] += cd->width;
                    }
                    else
                    {
                        for (int x = 0; x < cd->width; ++x)
                        {
                            for (int byte = 0; byte < pixelSize; ++byte)
                            {
                               *dst++ = *cd->planarUncRleEnd[byte]++;
                            }
                        }
                    }

                    row++;
                }
            }

            break;

          case UNKNOWN:

            //
            // In the UNKNOWN case, data is already in planarUncBufferEnd
            // and just needs to copied over to the output buffer
            //

            {
                int row             = 0;
                int dstScanlineSize = cd->width * Imf::pixelTypeSize (cd->type);

                for (int y = minY; y <= maxY; ++y)
                {
                    if (Imath::modp (y, cd->ySampling) != 0)
                        continue;

                    memcpy (rowPtrs[chan][row],
                            cd->planarUncBufferEnd,
                            dstScanlineSize);

                    cd->planarUncBufferEnd += dstScanlineSize;
                    row++;
                }
            }

            break;

          default:

            throw Iex::NoImplExc ("Unhandled compression scheme case");
            break;
        }

        decodedChannels[chan] = true;
    }

    //
    // Return a ptr to _outBuffer
    //

    outPtr = _outBuffer;
    return (int)(outBufferEnd - _outBuffer);
}