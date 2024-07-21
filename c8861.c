ScanLineInputFile::setFrameBuffer (const FrameBuffer &frameBuffer)
{
    Lock lock (*_streamData);

    
    
    const ChannelList &channels = _data->header.channels();
    for (FrameBuffer::ConstIterator j = frameBuffer.begin();
	 j != frameBuffer.end();
	 ++j)
    {
	ChannelList::ConstIterator i = channels.find (j.name());

	if (i == channels.end())
	    continue;

	if (i.channel().xSampling != j.slice().xSampling ||
	    i.channel().ySampling != j.slice().ySampling)
	    THROW (IEX_NAMESPACE::ArgExc, "X and/or y subsampling factors "
				"of \"" << i.name() << "\" channel "
				"of input file \"" << fileName() << "\" are "
				"not compatible with the frame buffer's "
				"subsampling factors.");
    }

    // optimization is possible if this is a little endian system
    // and both inputs and outputs are half floats
    // 
    bool optimizationPossible = true;
    
    if (!GLOBAL_SYSTEM_LITTLE_ENDIAN)
    {
        optimizationPossible =false;
    }
    
    vector<sliceOptimizationData> optData;
    

    //
    // Initialize the slice table for readPixels().
    //

    vector<InSliceInfo> slices;
    ChannelList::ConstIterator i = channels.begin();
    
    // current offset of channel: pixel data starts at offset*width into the
    // decompressed scanline buffer
    size_t offset = 0;
    
    for (FrameBuffer::ConstIterator j = frameBuffer.begin();
	 j != frameBuffer.end();
	 ++j)
    {
	while (i != channels.end() && strcmp (i.name(), j.name()) < 0)
	{
	    //
	    // Channel i is present in the file but not
	    // in the frame buffer; data for channel i
	    // will be skipped during readPixels().
	    //

	    slices.push_back (InSliceInfo (i.channel().type,
					   i.channel().type,
					   0, // base
					   0, // xStride
					   0, // yStride
					   i.channel().xSampling,
					   i.channel().ySampling,
					   false,  // fill
					   true, // skip
					   0.0)); // fillValue
	    
              switch(i.channel().type)
              {
                  case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF :
                      offset++;
                      break;
                  case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT :
                      offset+=2;
                      break;
                  case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT :
                      offset+=2;
                      break;
                  case OPENEXR_IMF_INTERNAL_NAMESPACE::NUM_PIXELTYPES:
                  default:
                      // not possible.
                      break;
              }
              ++i;
	}

	bool fill = false;

	if (i == channels.end() || strcmp (i.name(), j.name()) > 0)
	{
	    //
	    // Channel i is present in the frame buffer, but not in the file.
	    // In the frame buffer, slice j will be filled with a default value.
	    //

	    fill = true;
	}

	slices.push_back (InSliceInfo (j.slice().type,
				       fill? j.slice().type:
				             i.channel().type,
				       j.slice().base,
				       j.slice().xStride,
				       j.slice().yStride,
				       j.slice().xSampling,
				       j.slice().ySampling,
				       fill,
				       false, // skip
				       j.slice().fillValue));

          if(!fill && i.channel().type!=OPENEXR_IMF_INTERNAL_NAMESPACE::HALF)
          {
              optimizationPossible = false;
          }
          
          if(j.slice().type != OPENEXR_IMF_INTERNAL_NAMESPACE::HALF)
          {
              optimizationPossible = false;
          }
          if(j.slice().xSampling!=1 || j.slice().ySampling!=1)
          {
              optimizationPossible = false;
          }

          
          if(optimizationPossible)
          {
              sliceOptimizationData dat;
              dat.base = j.slice().base;
              dat.fill = fill;
              dat.fillValue = j.slice().fillValue;
              dat.offset = offset;
              dat.xStride = j.slice().xStride;
              dat.yStride = j.slice().yStride;
              dat.xSampling = j.slice().xSampling;
              dat.ySampling = j.slice().ySampling;
              optData.push_back(dat);
          }
          
          if(!fill)
          {
              switch(i.channel().type)
              {
                  case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF :
                      offset++;
                      break;
                  case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT :
                      offset+=2;
                      break;
                  case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT :
                      offset+=2;
                      break;
                  case OPENEXR_IMF_INTERNAL_NAMESPACE::NUM_PIXELTYPES:
                  default:
                      // not possible.
                      break;
              }
          }
          

          
	if (i != channels.end() && !fill)
	    ++i;
    }

   
   if(optimizationPossible)
   {
       //
       // check optimisibility
       // based on channel ordering and fill channel positions
       //
       sort(optData.begin(),optData.end());
       _data->optimizationMode = detectOptimizationMode(optData);
   }
   
   if(!optimizationPossible || _data->optimizationMode._optimizable==false)
   {   
       optData = vector<sliceOptimizationData>();
       _data->optimizationMode._optimizable=false;
   }
    
    //
    // Store the new frame buffer.
    //

    _data->frameBuffer = frameBuffer;
    _data->slices = slices;
    _data->optimizationData = optData;
}