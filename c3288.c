int LibRaw::unpack(void)
{
  CHECK_ORDER_HIGH(LIBRAW_PROGRESS_LOAD_RAW);
  CHECK_ORDER_LOW(LIBRAW_PROGRESS_IDENTIFY);
  try {

    if(!libraw_internal_data.internal_data.input)
      return LIBRAW_INPUT_CLOSED;

    RUN_CALLBACK(LIBRAW_PROGRESS_LOAD_RAW,0,2);
    if (O.shot_select >= P1.raw_count)
      return LIBRAW_REQUEST_FOR_NONEXISTENT_IMAGE;

    if(!load_raw)
      return LIBRAW_UNSPECIFIED_ERROR;

    // already allocated ?
    if(imgdata.image)
      {
        free(imgdata.image);
        imgdata.image = 0;
      }
    if(imgdata.rawdata.raw_alloc)
      {
        free(imgdata.rawdata.raw_alloc);
        imgdata.rawdata.raw_alloc = 0;
      }
    if (libraw_internal_data.unpacker_data.meta_length)
      {
        libraw_internal_data.internal_data.meta_data =
          (char *) malloc (libraw_internal_data.unpacker_data.meta_length);
        merror (libraw_internal_data.internal_data.meta_data, "LibRaw::unpack()");
      }

    libraw_decoder_info_t decoder_info;
    get_decoder_info(&decoder_info);

    int save_iwidth = S.iwidth, save_iheight = S.iheight, save_shrink = IO.shrink;

    int rwidth = S.raw_width, rheight = S.raw_height;
    if( !IO.fuji_width)
      {
        // adjust non-Fuji allocation
        if(rwidth < S.width + S.left_margin)
          rwidth = S.width + S.left_margin;
        if(rheight < S.height + S.top_margin)
          rheight = S.height + S.top_margin;
      }

    imgdata.rawdata.raw_image = 0;
    imgdata.rawdata.color4_image = 0;
    imgdata.rawdata.color3_image = 0;
#ifdef USE_RAWSPEED
	int rawspeed_enabled = 1;
	if(imgdata.idata.dng_version && libraw_internal_data.unpacker_data.tiff_samples == 2)
		rawspeed_enabled = 0;
	// Disable rawspeed for double-sized Oly files
	if(!strncasecmp(imgdata.idata.make,"Olympus",7) && 
		( (!strncasecmp(imgdata.idata.model,"E-M5MarkII",10) && imgdata.sizes.raw_width == 9280) || !strncasecmp(imgdata.idata.model,"SH-2",4) || !strncasecmp(imgdata.idata.model,"TG-4",4))
		)
		rawspeed_enabled = 0;

	if(!strncasecmp(imgdata.idata.make,"Canon",5) 
 		&& !strncasecmp(imgdata.idata.model,"EOS 5DS",7) 
		&& (load_raw == &LibRaw::canon_sraw_load_raw))
		rawspeed_enabled = 0;

    // RawSpeed Supported,
    if(O.use_rawspeed  && rawspeed_enabled
       && !(is_sraw() && O.sraw_ycc)
       && (decoder_info.decoder_flags & LIBRAW_DECODER_TRYRAWSPEED) && _rawspeed_camerameta)
      {
        INT64 spos = ID.input->tell();
        void *_rawspeed_buffer = 0;
        try
          {
            //                printf("Using rawspeed\n");
            ID.input->seek(0,SEEK_SET);
            INT64 _rawspeed_buffer_sz = ID.input->size()+32;
            _rawspeed_buffer = malloc(_rawspeed_buffer_sz);
            if(!_rawspeed_buffer) throw LIBRAW_EXCEPTION_ALLOC;
            ID.input->read(_rawspeed_buffer,_rawspeed_buffer_sz,1);
            FileMap map((uchar8*)_rawspeed_buffer,_rawspeed_buffer_sz);
            RawParser t(&map);
            RawDecoder *d = 0;
            CameraMetaDataLR *meta = static_cast<CameraMetaDataLR*>(_rawspeed_camerameta);
            d = t.getDecoder();
            if(!d) throw "Unable to find decoder";
            try {
              d->checkSupport(meta);
            }
            catch (const RawDecoderException& e)
              {
                imgdata.process_warnings |= LIBRAW_WARN_RAWSPEED_UNSUPPORTED;
                throw e;
              }
            d->interpolateBadPixels = FALSE;
            d->applyStage1DngOpcodes = FALSE;
            _rawspeed_decoder = static_cast<void*>(d);
            d->decodeRaw();
            d->decodeMetaData(meta);
            RawImage r = d->mRaw;
            if( r->errors.size()>0)
              {
                delete d;
                _rawspeed_decoder = 0;
                throw 1;
              }
            if (r->isCFA)
			{
              imgdata.rawdata.raw_image = (ushort*) r->getDataUncropped(0,0);
            }
			else if(r->getCpp()==4)
			{
              imgdata.rawdata.color4_image = (ushort(*)[4]) r->getDataUncropped(0,0);
			  if(r->whitePoint > 0 && r->whitePoint < 65536)
					C.maximum = r->whitePoint;
            } else if(r->getCpp() == 3)
              {
                imgdata.rawdata.color3_image = (ushort(*)[3]) r->getDataUncropped(0,0);
				if(r->whitePoint > 0 && r->whitePoint < 65536)
					C.maximum = r->whitePoint;
              }
            else
              {
                delete d;
                _rawspeed_decoder = 0;
              }
            if(_rawspeed_decoder)
              {
                // set sizes
                iPoint2D rsdim = r->getUncroppedDim();
                S.raw_pitch = r->pitch;
                S.raw_width = rsdim.x;
                S.raw_height = rsdim.y;
                //C.maximum = r->whitePoint;
                fix_after_rawspeed(r->blackLevel);
              }
            free(_rawspeed_buffer);
            _rawspeed_buffer = 0;
            imgdata.process_warnings |= LIBRAW_WARN_RAWSPEED_PROCESSED;
          }
		catch (const RawDecoderException& RDE)
		{
			imgdata.process_warnings |= LIBRAW_WARN_RAWSPEED_PROBLEM;
			if (_rawspeed_buffer)
			{
				free(_rawspeed_buffer);
				_rawspeed_buffer = 0;
			}
			const char *p = RDE.what();
			if (!strncmp(RDE.what(), "Decoder canceled", strlen("Decoder canceled")))
				throw LIBRAW_EXCEPTION_CANCELLED_BY_CALLBACK;
		}
        catch (...)
          {
            // We may get here due to cancellation flag
            imgdata.process_warnings |= LIBRAW_WARN_RAWSPEED_PROBLEM;
            if(_rawspeed_buffer)
              {
                free(_rawspeed_buffer);
                _rawspeed_buffer = 0;
              }
          }
        ID.input->seek(spos,SEEK_SET);
      }
#endif
    if(!imgdata.rawdata.raw_image && !imgdata.rawdata.color4_image && !imgdata.rawdata.color3_image) //RawSpeed failed!
      {
        // Not allocated on RawSpeed call, try call LibRaw
        if(decoder_info.decoder_flags &  LIBRAW_DECODER_OWNALLOC)
          {
            // x3f foveon decoder
            // Do nothing! Decoder will allocate data internally
          }
        else if(imgdata.idata.filters || P1.colors == 1) // Bayer image or single color -> decode to raw_image
          {
            imgdata.rawdata.raw_alloc = malloc(rwidth*(rheight+8)*sizeof(imgdata.rawdata.raw_image[0]));
            imgdata.rawdata.raw_image = (ushort*) imgdata.rawdata.raw_alloc;
            if(!S.raw_pitch)
                S.raw_pitch = S.raw_width*2; // Bayer case, not set before
          }
        else // NO LEGACY FLAG if (decoder_info.decoder_flags & LIBRAW_DECODER_LEGACY)
          {
            // sRAW and old Foveon decoders only, so extra buffer size is just 1/4
            S.iwidth = S.width;
            S.iheight= S.height;
            IO.shrink = 0;
            S.raw_pitch = S.width*8;
            // allocate image as temporary buffer, size
            imgdata.rawdata.raw_alloc = 0;
            imgdata.image = (ushort (*)[4]) calloc(S.iwidth*S.iheight,sizeof(*imgdata.image));
          }
        ID.input->seek(libraw_internal_data.unpacker_data.data_offset, SEEK_SET);

        unsigned m_save = C.maximum;
        if(load_raw == &LibRaw::unpacked_load_raw && !strcasecmp(imgdata.idata.make,"Nikon"))
          C.maximum=65535;
        (this->*load_raw)();
        if(load_raw == &LibRaw::unpacked_load_raw && !strcasecmp(imgdata.idata.make,"Nikon"))
          C.maximum = m_save;
        if(decoder_info.decoder_flags &  LIBRAW_DECODER_OWNALLOC)
          {
            // x3f foveon decoder only: do nothing

          }
        else if (!(imgdata.idata.filters || P1.colors == 1))
          {
            // successfully decoded legacy image, attach image to raw_alloc
            imgdata.rawdata.raw_alloc = imgdata.image;
            imgdata.image = 0;
            // Restore saved values. Note: Foveon have masked frame
            // Other 4-color legacy data: no borders
            S.raw_width = S.width;
            S.left_margin = 0;
            S.raw_height = S.height;
            S.top_margin = 0;
          }
      }

    if(imgdata.rawdata.raw_image)
      crop_masked_pixels(); // calculate black levels

    // recover saved
    if( !(imgdata.idata.filters || P1.colors == 1) && !imgdata.rawdata.color4_image)
      {
        imgdata.image = 0;
        imgdata.rawdata.color4_image = (ushort (*)[4]) imgdata.rawdata.raw_alloc;
      }

    // recover image sizes
    S.iwidth = save_iwidth;
    S.iheight = save_iheight;
    IO.shrink = save_shrink;

    // adjust black to possible maximum
    unsigned int i = C.cblack[3];
    unsigned int c;
    for(c=0;c<3;c++)
      if (i > C.cblack[c]) i = C.cblack[c];
    for (c=0;c<4;c++)
      C.cblack[c] -= i;
    C.black += i;

    // Save color,sizes and internal data into raw_image fields
    memmove(&imgdata.rawdata.color,&imgdata.color,sizeof(imgdata.color));
    memmove(&imgdata.rawdata.sizes,&imgdata.sizes,sizeof(imgdata.sizes));
    memmove(&imgdata.rawdata.iparams,&imgdata.idata,sizeof(imgdata.idata));
    memmove(&imgdata.rawdata.ioparams,&libraw_internal_data.internal_output_params,sizeof(libraw_internal_data.internal_output_params));

    SET_PROC_FLAG(LIBRAW_PROGRESS_LOAD_RAW);
    RUN_CALLBACK(LIBRAW_PROGRESS_LOAD_RAW,1,2);

    return 0;
  }
  catch ( LibRaw_exceptions err) {
    EXCEPTION_HANDLER(err);
  }
  catch (std::exception ee) {
    EXCEPTION_HANDLER(LIBRAW_EXCEPTION_IO_CORRUPT);
  }
}