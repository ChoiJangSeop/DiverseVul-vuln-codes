static Image *ReadTIFFImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
#define ThrowTIFFException(severity,message) \
{ \
  if (tiff_pixels != (unsigned char *) NULL) \
    tiff_pixels=(unsigned char *) RelinquishMagickMemory(tiff_pixels); \
  if (quantum_info != (QuantumInfo *) NULL) \
    quantum_info=DestroyQuantumInfo(quantum_info); \
  TIFFClose(tiff); \
  ThrowReaderException(severity,message); \
}

  const char
    *option;

  float
    *chromaticity,
    x_position,
    y_position,
    x_resolution,
    y_resolution;

  Image
    *image;

  int
    tiff_status;

  MagickBooleanType
    status;

  QuantumInfo
    *quantum_info;

  QuantumType
    quantum_type;

  register ssize_t
    i;

  size_t
    number_pixels,
    pad;

  ssize_t
    y;

  TIFF
    *tiff;

  TIFFMethodType
    method;

  uint16
    compress_tag,
    bits_per_sample,
    endian,
    extra_samples,
    interlace,
    max_sample_value,
    min_sample_value,
    orientation,
    pages,
    photometric,
    *sample_info,
    sample_format,
    samples_per_pixel,
    units,
    value;

  uint32
    height,
    rows_per_strip,
    width;

  unsigned char
    *tiff_pixels;

  /*
    Open image.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  image=AcquireImage(image_info);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  (void) SetMagickThreadValue(tiff_exception,exception);
  tiff=TIFFClientOpen(image->filename,"rb",(thandle_t) image,TIFFReadBlob,
    TIFFWriteBlob,TIFFSeekBlob,TIFFCloseBlob,TIFFGetBlobSize,TIFFMapBlob,
    TIFFUnmapBlob);
  if (tiff == (TIFF *) NULL)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  if (image_info->number_scenes != 0)
    {
      /*
      Generate blank images for subimage specification (e.g. image.tif[4].
      We need to check the number of directores because it is possible that
      the subimage(s) are stored in the photoshop profile.
      */
      if (image_info->scene < (size_t)TIFFNumberOfDirectories(tiff))
        {
          for (i=0; i < (ssize_t) image_info->scene; i++)
          {
            status=TIFFReadDirectory(tiff) != 0 ? MagickTrue : MagickFalse;
            if (status == MagickFalse)
              {
                TIFFClose(tiff);
                image=DestroyImageList(image);
                return((Image *) NULL);
              }
            AcquireNextImage(image_info,image);
            if (GetNextImageInList(image) == (Image *) NULL)
              {
                TIFFClose(tiff);
                image=DestroyImageList(image);
                return((Image *) NULL);
              }
            image=SyncNextImageInList(image);
          }
        }
    }
  do
  {
DisableMSCWarning(4127)
    if (0 && (image_info->verbose != MagickFalse))
      TIFFPrintDirectory(tiff,stdout,MagickFalse);
RestoreMSCWarning
    photometric=PHOTOMETRIC_RGB;
    if ((TIFFGetField(tiff,TIFFTAG_IMAGEWIDTH,&width) != 1) ||
        (TIFFGetField(tiff,TIFFTAG_IMAGELENGTH,&height) != 1) ||
        (TIFFGetFieldDefaulted(tiff,TIFFTAG_PHOTOMETRIC,&photometric) != 1) ||
        (TIFFGetFieldDefaulted(tiff,TIFFTAG_COMPRESSION,&compress_tag) != 1) ||
        (TIFFGetFieldDefaulted(tiff,TIFFTAG_FILLORDER,&endian) != 1) ||
        (TIFFGetFieldDefaulted(tiff,TIFFTAG_PLANARCONFIG,&interlace) != 1) ||
        (TIFFGetFieldDefaulted(tiff,TIFFTAG_SAMPLESPERPIXEL,&samples_per_pixel) != 1) ||
        (TIFFGetFieldDefaulted(tiff,TIFFTAG_BITSPERSAMPLE,&bits_per_sample) != 1) ||
        (TIFFGetFieldDefaulted(tiff,TIFFTAG_SAMPLEFORMAT,&sample_format) != 1) ||
        (TIFFGetFieldDefaulted(tiff,TIFFTAG_MINSAMPLEVALUE,&min_sample_value) != 1) ||
        (TIFFGetFieldDefaulted(tiff,TIFFTAG_MAXSAMPLEVALUE,&max_sample_value) != 1))
      {
        TIFFClose(tiff);
        ThrowReaderException(CorruptImageError,"ImproperImageHeader");
      }
    if (sample_format == SAMPLEFORMAT_IEEEFP)
      (void) SetImageProperty(image,"quantum:format","floating-point");
    switch (photometric)
    {
      case PHOTOMETRIC_MINISBLACK:
      {
        (void) SetImageProperty(image,"tiff:photometric","min-is-black");
        break;
      }
      case PHOTOMETRIC_MINISWHITE:
      {
        (void) SetImageProperty(image,"tiff:photometric","min-is-white");
        break;
      }
      case PHOTOMETRIC_PALETTE:
      {
        (void) SetImageProperty(image,"tiff:photometric","palette");
        break;
      }
      case PHOTOMETRIC_RGB:
      {
        (void) SetImageProperty(image,"tiff:photometric","RGB");
        break;
      }
      case PHOTOMETRIC_CIELAB:
      {
        (void) SetImageProperty(image,"tiff:photometric","CIELAB");
        break;
      }
      case PHOTOMETRIC_LOGL:
      {
        (void) SetImageProperty(image,"tiff:photometric","CIE Log2(L)");
        break;
      }
      case PHOTOMETRIC_LOGLUV:
      {
        (void) SetImageProperty(image,"tiff:photometric","LOGLUV");
        break;
      }
#if defined(PHOTOMETRIC_MASK)
      case PHOTOMETRIC_MASK:
      {
        (void) SetImageProperty(image,"tiff:photometric","MASK");
        break;
      }
#endif
      case PHOTOMETRIC_SEPARATED:
      {
        (void) SetImageProperty(image,"tiff:photometric","separated");
        break;
      }
      case PHOTOMETRIC_YCBCR:
      {
        (void) SetImageProperty(image,"tiff:photometric","YCBCR");
        break;
      }
      default:
      {
        (void) SetImageProperty(image,"tiff:photometric","unknown");
        break;
      }
    }
    if (image->debug != MagickFalse)
      {
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Geometry: %ux%u",
          (unsigned int) width,(unsigned int) height);
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Interlace: %u",
          interlace);
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Bits per sample: %u",bits_per_sample);
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Min sample value: %u",min_sample_value);
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Max sample value: %u",max_sample_value);
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Photometric "
          "interpretation: %s",GetImageProperty(image,"tiff:photometric"));
      }
    image->columns=(size_t) width;
    image->rows=(size_t) height;
    image->depth=(size_t) bits_per_sample;
    if (image->debug != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Image depth: %.20g",
        (double) image->depth);
    image->endian=MSBEndian;
    if (endian == FILLORDER_LSB2MSB)
      image->endian=LSBEndian;
#if defined(MAGICKCORE_HAVE_TIFFISBIGENDIAN)
    if (TIFFIsBigEndian(tiff) == 0)
      {
        (void) SetImageProperty(image,"tiff:endian","lsb");
        image->endian=LSBEndian;
      }
    else
      {
        (void) SetImageProperty(image,"tiff:endian","msb");
        image->endian=MSBEndian;
      }
#endif
    if ((photometric == PHOTOMETRIC_MINISBLACK) ||
        (photometric == PHOTOMETRIC_MINISWHITE))
      SetImageColorspace(image,GRAYColorspace);
    if (photometric == PHOTOMETRIC_SEPARATED)
      SetImageColorspace(image,CMYKColorspace);
    if (photometric == PHOTOMETRIC_CIELAB)
      SetImageColorspace(image,LabColorspace);
    TIFFGetProfiles(tiff,image);
    TIFFGetProperties(tiff,image);
    option=GetImageOption(image_info,"tiff:exif-properties");
    if ((option == (const char *) NULL) ||
        (IsMagickTrue(option) != MagickFalse))
      TIFFGetEXIFProperties(tiff,image);
    if ((TIFFGetFieldDefaulted(tiff,TIFFTAG_XRESOLUTION,&x_resolution) == 1) &&
        (TIFFGetFieldDefaulted(tiff,TIFFTAG_YRESOLUTION,&y_resolution) == 1))
      {
        image->x_resolution=x_resolution;
        image->y_resolution=y_resolution;
      }
    if (TIFFGetFieldDefaulted(tiff,TIFFTAG_RESOLUTIONUNIT,&units) == 1)
      {
        if (units == RESUNIT_INCH)
          image->units=PixelsPerInchResolution;
        if (units == RESUNIT_CENTIMETER)
          image->units=PixelsPerCentimeterResolution;
      }
    if ((TIFFGetFieldDefaulted(tiff,TIFFTAG_XPOSITION,&x_position) == 1) &&
        (TIFFGetFieldDefaulted(tiff,TIFFTAG_YPOSITION,&y_position) == 1))
      {
        image->page.x=(ssize_t) ceil(x_position*image->x_resolution-0.5);
        image->page.y=(ssize_t) ceil(y_position*image->y_resolution-0.5);
      }
    if (TIFFGetFieldDefaulted(tiff,TIFFTAG_ORIENTATION,&orientation) == 1)
      image->orientation=(OrientationType) orientation;
    if (TIFFGetField(tiff,TIFFTAG_WHITEPOINT,&chromaticity) == 1)
      {
        if (chromaticity != (float *) NULL)
          {
            image->chromaticity.white_point.x=chromaticity[0];
            image->chromaticity.white_point.y=chromaticity[1];
          }
      }
    if (TIFFGetField(tiff,TIFFTAG_PRIMARYCHROMATICITIES,&chromaticity) == 1)
      {
        if (chromaticity != (float *) NULL)
          {
            image->chromaticity.red_primary.x=chromaticity[0];
            image->chromaticity.red_primary.y=chromaticity[1];
            image->chromaticity.green_primary.x=chromaticity[2];
            image->chromaticity.green_primary.y=chromaticity[3];
            image->chromaticity.blue_primary.x=chromaticity[4];
            image->chromaticity.blue_primary.y=chromaticity[5];
          }
      }
#if defined(MAGICKCORE_HAVE_TIFFISCODECCONFIGURED) || (TIFFLIB_VERSION > 20040919)
    if ((compress_tag != COMPRESSION_NONE) &&
        (TIFFIsCODECConfigured(compress_tag) == 0))
      {
        TIFFClose(tiff);
        ThrowReaderException(CoderError,"CompressNotSupported");
      }
#endif
    switch (compress_tag)
    {
      case COMPRESSION_NONE: image->compression=NoCompression; break;
      case COMPRESSION_CCITTFAX3: image->compression=FaxCompression; break;
      case COMPRESSION_CCITTFAX4: image->compression=Group4Compression; break;
      case COMPRESSION_JPEG:
      {
         image->compression=JPEGCompression;
#if defined(JPEG_SUPPORT)
         {
           char
             sampling_factor[MaxTextExtent];

           int
             tiff_status;

           uint16
             horizontal,
             vertical;

           tiff_status=TIFFGetField(tiff,TIFFTAG_YCBCRSUBSAMPLING,&horizontal,
             &vertical);
           if (tiff_status == 1)
             {
               (void) FormatLocaleString(sampling_factor,MaxTextExtent,"%dx%d",
                 horizontal,vertical);
               (void) SetImageProperty(image,"jpeg:sampling-factor",
                 sampling_factor);
               (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                 "Sampling Factors: %s",sampling_factor);
             }
         }
#endif
        break;
      }
      case COMPRESSION_OJPEG: image->compression=JPEGCompression; break;
#if defined(COMPRESSION_LZMA)
      case COMPRESSION_LZMA: image->compression=LZMACompression; break;
#endif
      case COMPRESSION_LZW: image->compression=LZWCompression; break;
      case COMPRESSION_DEFLATE: image->compression=ZipCompression; break;
      case COMPRESSION_ADOBE_DEFLATE: image->compression=ZipCompression; break;
      default: image->compression=RLECompression; break;
    }
    tiff_pixels=(unsigned char *) NULL;
    quantum_info=(QuantumInfo *) NULL;
    if ((photometric == PHOTOMETRIC_PALETTE) &&
        (pow(2.0,1.0*bits_per_sample) <= MaxColormapSize))
      {
        size_t
          colors;

        colors=(size_t) GetQuantumRange(bits_per_sample)+1;
        if (AcquireImageColormap(image,colors) == MagickFalse)
          {
            TIFFClose(tiff);
            ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
          }
      }
    if (TIFFGetFieldDefaulted(tiff,TIFFTAG_PAGENUMBER,&value,&pages) == 1)
      image->scene=value;
    if (image->storage_class == PseudoClass)
      {
        int
          tiff_status;

        size_t
          range;

        uint16
          *blue_colormap,
          *green_colormap,
          *red_colormap;

        /*
          Initialize colormap.
        */
        tiff_status=TIFFGetField(tiff,TIFFTAG_COLORMAP,&red_colormap,
          &green_colormap,&blue_colormap);
        if (tiff_status == 1)
          {
            if ((red_colormap != (uint16 *) NULL) &&
                (green_colormap != (uint16 *) NULL) &&
                (blue_colormap != (uint16 *) NULL))
              {
                range=255;  /* might be old style 8-bit colormap */
                for (i=0; i < (ssize_t) image->colors; i++)
                  if ((red_colormap[i] >= 256) || (green_colormap[i] >= 256) ||
                      (blue_colormap[i] >= 256))
                    {
                      range=65535;
                      break;
                    }
                for (i=0; i < (ssize_t) image->colors; i++)
                {
                  image->colormap[i].red=ClampToQuantum(((double)
                    QuantumRange*red_colormap[i])/range);
                  image->colormap[i].green=ClampToQuantum(((double)
                    QuantumRange*green_colormap[i])/range);
                  image->colormap[i].blue=ClampToQuantum(((double)
                    QuantumRange*blue_colormap[i])/range);
                }
              }
          }
      }
    if (image_info->ping != MagickFalse)
      {
        if (image_info->number_scenes != 0)
          if (image->scene >= (image_info->scene+image_info->number_scenes-1))
            break;
        goto next_tiff_frame;
      }
    status=SetImageExtent(image,image->columns,image->rows);
    if (status == MagickFalse)
      {
        TIFFClose(tiff);
        InheritException(exception,&image->exception);
        return(DestroyImageList(image));
      }
    status=ResetImagePixels(image,exception);
    if (status == MagickFalse)
      {
        TIFFClose(tiff);
        InheritException(exception,&image->exception);
        return(DestroyImageList(image));
      }
    /*
      Allocate memory for the image and pixel buffer.
    */
    quantum_info=AcquireQuantumInfo(image_info,image);
    if (quantum_info == (QuantumInfo *) NULL)
      ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
    if (sample_format == SAMPLEFORMAT_UINT)
      status=SetQuantumFormat(image,quantum_info,UnsignedQuantumFormat);
    if (sample_format == SAMPLEFORMAT_INT)
      status=SetQuantumFormat(image,quantum_info,SignedQuantumFormat);
    if (sample_format == SAMPLEFORMAT_IEEEFP)
      status=SetQuantumFormat(image,quantum_info,FloatingPointQuantumFormat);
    if (status == MagickFalse)
      ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
    status=MagickTrue;
    switch (photometric)
    {
      case PHOTOMETRIC_MINISBLACK:
      {
        quantum_info->min_is_white=MagickFalse;
        break;
      }
      case PHOTOMETRIC_MINISWHITE:
      {
        quantum_info->min_is_white=MagickTrue;
        break;
      }
      default:
        break;
    }
    tiff_status=TIFFGetFieldDefaulted(tiff,TIFFTAG_EXTRASAMPLES,&extra_samples,
      &sample_info);
    if (tiff_status == 1)
      {
        (void) SetImageProperty(image,"tiff:alpha","unspecified");
        if (extra_samples == 0)
          {
            if ((samples_per_pixel == 4) && (photometric == PHOTOMETRIC_RGB))
              image->matte=MagickTrue;
          }
        else
          for (i=0; i < extra_samples; i++)
          {
            image->matte=MagickTrue;
            if (sample_info[i] == EXTRASAMPLE_ASSOCALPHA)
              {
                SetQuantumAlphaType(quantum_info,DisassociatedQuantumAlpha);
                (void) SetImageProperty(image,"tiff:alpha","associated");
              }
            else
              if (sample_info[i] == EXTRASAMPLE_UNASSALPHA)
               (void) SetImageProperty(image,"tiff:alpha","unassociated");
          }
      }
    if (image->matte != MagickFalse)
      (void) SetImageAlphaChannel(image,OpaqueAlphaChannel);
    method=ReadGenericMethod;
    rows_per_strip=(uint32) image->rows;
    if (TIFFGetField(tiff,TIFFTAG_ROWSPERSTRIP,&rows_per_strip) == 1)
      {
        char
          value[MaxTextExtent];

        method=ReadStripMethod;
        (void) FormatLocaleString(value,MaxTextExtent,"%u",(unsigned int)
          rows_per_strip);
        (void) SetImageProperty(image,"tiff:rows-per-strip",value);
      }
    if (rows_per_strip > (uint32) image->rows)
      rows_per_strip=(uint32) image->rows;
    if ((samples_per_pixel >= 3) && (interlace == PLANARCONFIG_CONTIG))
      if ((image->matte == MagickFalse) || (samples_per_pixel >= 4))
        method=ReadRGBAMethod;
    if ((samples_per_pixel >= 4) && (interlace == PLANARCONFIG_SEPARATE))
      if ((image->matte == MagickFalse) || (samples_per_pixel >= 5))
        method=ReadCMYKAMethod;
    if ((photometric != PHOTOMETRIC_RGB) &&
        (photometric != PHOTOMETRIC_CIELAB) &&
        (photometric != PHOTOMETRIC_SEPARATED))
      method=ReadGenericMethod;
    if (image->storage_class == PseudoClass)
      method=ReadSingleSampleMethod;
    if ((photometric == PHOTOMETRIC_MINISBLACK) ||
        (photometric == PHOTOMETRIC_MINISWHITE))
      method=ReadSingleSampleMethod;
    if ((photometric != PHOTOMETRIC_SEPARATED) &&
        (interlace == PLANARCONFIG_SEPARATE) && (bits_per_sample < 64))
      method=ReadGenericMethod;
    if (image->compression == JPEGCompression)
      method=GetJPEGMethod(image,tiff,photometric,bits_per_sample,
        samples_per_pixel);
    if (compress_tag == COMPRESSION_JBIG)
      method=ReadStripMethod;
    if (TIFFIsTiled(tiff) != MagickFalse)
      method=ReadTileMethod;
    quantum_info->endian=LSBEndian;
    quantum_type=RGBQuantum;
    if (TIFFScanlineSize(tiff) <= 0)
      ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
    if (((MagickSizeType) TIFFScanlineSize(tiff)) > GetBlobSize(image))
      ThrowTIFFException(CorruptImageError,"InsufficientImageDataInFile");
    number_pixels=MagickMax(TIFFScanlineSize(tiff),MagickMax((ssize_t) 
      image->columns*samples_per_pixel*pow(2.0,ceil(log(bits_per_sample)/
      log(2.0))),image->columns*rows_per_strip)*sizeof(uint32));
    tiff_pixels=(unsigned char *) AcquireMagickMemory(number_pixels);
    if (tiff_pixels == (unsigned char *) NULL)
      ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
    (void) ResetMagickMemory(tiff_pixels,0,number_pixels);
    switch (method)
    {
      case ReadSingleSampleMethod:
      {
        /*
          Convert TIFF image to PseudoClass MIFF image.
        */
        quantum_type=IndexQuantum;
        pad=(size_t) MagickMax((ssize_t) samples_per_pixel-1,0);
        if (image->matte != MagickFalse)
          {
            if (image->storage_class != PseudoClass)
              {
                quantum_type=samples_per_pixel == 1 ? AlphaQuantum :
                  GrayAlphaQuantum;
                pad=(size_t) MagickMax((ssize_t) samples_per_pixel-2,0);
              }
            else
              {
                quantum_type=IndexAlphaQuantum;
                pad=(size_t) MagickMax((ssize_t) samples_per_pixel-2,0);
              }
          }
        else
          if (image->storage_class != PseudoClass)
            {
              quantum_type=GrayQuantum;
              pad=(size_t) MagickMax((ssize_t) samples_per_pixel-1,0);
            }
        status=SetQuantumPad(image,quantum_info,pad*pow(2,ceil(log(
          bits_per_sample)/log(2))));
        if (status == MagickFalse)
          ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          int
            status;

          register PixelPacket
            *magick_restrict q;

          status=TIFFReadPixels(tiff,0,y,(char *) tiff_pixels);
          if (status == -1)
            break;
          q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
          if (q == (PixelPacket *) NULL)
            break;
          (void) ImportQuantumPixels(image,(CacheView *) NULL,quantum_info,
            quantum_type,tiff_pixels,exception);
          if (SyncAuthenticPixels(image,exception) == MagickFalse)
            break;
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,LoadImageTag,(MagickOffsetType) y,
                image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        break;
      }
      case ReadRGBAMethod:
      {
        /*
          Convert TIFF image to DirectClass MIFF image.
        */
        pad=(size_t) MagickMax((size_t) samples_per_pixel-3,0);
        quantum_type=RGBQuantum;
        if (image->matte != MagickFalse)
          {
            quantum_type=RGBAQuantum;
            pad=(size_t) MagickMax((size_t) samples_per_pixel-4,0);
          }
        if (image->colorspace == CMYKColorspace)
          {
            pad=(size_t) MagickMax((size_t) samples_per_pixel-4,0);
            quantum_type=CMYKQuantum;
            if (image->matte != MagickFalse)
              {
                quantum_type=CMYKAQuantum;
                pad=(size_t) MagickMax((size_t) samples_per_pixel-5,0);
              }
          }
        status=SetQuantumPad(image,quantum_info,pad*((bits_per_sample+7) >> 3));
        if (status == MagickFalse)
          ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          int
            status;

          register PixelPacket
            *magick_restrict q;

          status=TIFFReadPixels(tiff,0,y,(char *) tiff_pixels);
          if (status == -1)
            break;
          q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
          if (q == (PixelPacket *) NULL)
            break;
          (void) ImportQuantumPixels(image,(CacheView *) NULL,quantum_info,
            quantum_type,tiff_pixels,exception);
          if (SyncAuthenticPixels(image,exception) == MagickFalse)
            break;
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,LoadImageTag,(MagickOffsetType) y,
                image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        break;
      }
      case ReadCMYKAMethod:
      {
        /*
          Convert TIFF image to DirectClass MIFF image.
        */
        for (i=0; i < (ssize_t) samples_per_pixel; i++)
        {
          for (y=0; y < (ssize_t) image->rows; y++)
          {
            register PixelPacket
              *magick_restrict q;

            int
              status;

            status=TIFFReadPixels(tiff,(tsample_t) i,y,(char *) tiff_pixels);
            if (status == -1)
              break;
            q=GetAuthenticPixels(image,0,y,image->columns,1,exception);
            if (q == (PixelPacket *) NULL)
              break;
            if (image->colorspace != CMYKColorspace)
              switch (i)
              {
                case 0: quantum_type=RedQuantum; break;
                case 1: quantum_type=GreenQuantum; break;
                case 2: quantum_type=BlueQuantum; break;
                case 3: quantum_type=AlphaQuantum; break;
                default: quantum_type=UndefinedQuantum; break;
              }
            else
              switch (i)
              {
                case 0: quantum_type=CyanQuantum; break;
                case 1: quantum_type=MagentaQuantum; break;
                case 2: quantum_type=YellowQuantum; break;
                case 3: quantum_type=BlackQuantum; break;
                case 4: quantum_type=AlphaQuantum; break;
                default: quantum_type=UndefinedQuantum; break;
              }
            (void) ImportQuantumPixels(image,(CacheView *) NULL,quantum_info,
              quantum_type,tiff_pixels,exception);
            if (SyncAuthenticPixels(image,exception) == MagickFalse)
              break;
          }
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,LoadImageTag,(MagickOffsetType) y,
                image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        break;
      }
      case ReadYCCKMethod:
      {
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          int
            status;

          register IndexPacket
            *indexes;

          register PixelPacket
            *magick_restrict q;

          register ssize_t
            x;

          unsigned char
            *p;

          status=TIFFReadPixels(tiff,0,y,(char *) tiff_pixels);
          if (status == -1)
            break;
          q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
          if (q == (PixelPacket *) NULL)
            break;
          indexes=GetAuthenticIndexQueue(image);
          p=tiff_pixels;
          for (x=0; x < (ssize_t) image->columns; x++)
          {
            SetPixelCyan(q,ScaleCharToQuantum(ClampYCC((double) *p+
              (1.402*(double) *(p+2))-179.456)));
            SetPixelMagenta(q,ScaleCharToQuantum(ClampYCC((double) *p-
              (0.34414*(double) *(p+1))-(0.71414*(double ) *(p+2))+
              135.45984)));
            SetPixelYellow(q,ScaleCharToQuantum(ClampYCC((double) *p+
              (1.772*(double) *(p+1))-226.816)));
            SetPixelBlack(indexes+x,ScaleCharToQuantum((unsigned char)*(p+3)));
            q++;
            p+=4;
          }
          if (SyncAuthenticPixels(image,exception) == MagickFalse)
            break;
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,LoadImageTag,(MagickOffsetType) y,
                image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        break;
      }
      case ReadStripMethod:
      {
        register uint32
          *p;

        /*
          Convert stripped TIFF image to DirectClass MIFF image.
        */
        i=0;
        p=(uint32 *) NULL;
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          register ssize_t
            x;

          register PixelPacket
            *magick_restrict q;

          q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
          if (q == (PixelPacket *) NULL)
            break;
          if (i == 0)
            {
              if (TIFFReadRGBAStrip(tiff,(tstrip_t) y,(uint32 *) tiff_pixels) == 0)
                break;
              i=(ssize_t) MagickMin((ssize_t) rows_per_strip,(ssize_t)
                image->rows-y);
            }
          i--;
          p=((uint32 *) tiff_pixels)+image->columns*i;
          for (x=0; x < (ssize_t) image->columns; x++)
          {
            SetPixelRed(q,ScaleCharToQuantum((unsigned char)
              (TIFFGetR(*p))));
            SetPixelGreen(q,ScaleCharToQuantum((unsigned char)
              (TIFFGetG(*p))));
            SetPixelBlue(q,ScaleCharToQuantum((unsigned char)
              (TIFFGetB(*p))));
            if (image->matte != MagickFalse)
              SetPixelOpacity(q,ScaleCharToQuantum((unsigned char)
                (TIFFGetA(*p))));
            p++;
            q++;
          }
          if (SyncAuthenticPixels(image,exception) == MagickFalse)
            break;
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,LoadImageTag,(MagickOffsetType) y,
                image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        break;
      }
      case ReadTileMethod:
      {
        register uint32
          *p;

        uint32
          *tile_pixels,
          columns,
          rows;

        /*
          Convert tiled TIFF image to DirectClass MIFF image.
        */
        if ((TIFFGetField(tiff,TIFFTAG_TILEWIDTH,&columns) != 1) ||
            (TIFFGetField(tiff,TIFFTAG_TILELENGTH,&rows) != 1))
          ThrowTIFFException(CoderError,"ImageIsNotTiled");
        if ((AcquireMagickResource(WidthResource,columns) == MagickFalse) ||
            (AcquireMagickResource(HeightResource,rows) == MagickFalse))
          ThrowTIFFException(ImageError,"WidthOrHeightExceedsLimit");
        (void) SetImageStorageClass(image,DirectClass);
        if (HeapOverflowSanityCheck(rows,sizeof(*tile_pixels)) != MagickFalse)
          ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
        tile_pixels=(uint32 *) AcquireQuantumMemory(columns,rows*
           sizeof(*tile_pixels));
        if (tile_pixels == (uint32 *) NULL)
          ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
        for (y=0; y < (ssize_t) image->rows; y+=rows)
        {
          PixelPacket
            *tile;

          register ssize_t
            x;

          register PixelPacket
            *magick_restrict q;

          size_t
            columns_remaining,
            rows_remaining;

          rows_remaining=image->rows-y;
          if ((ssize_t) (y+rows) < (ssize_t) image->rows)
            rows_remaining=rows;
          tile=QueueAuthenticPixels(image,0,y,image->columns,rows_remaining,
            exception);
          if (tile == (PixelPacket *) NULL)
            break;
          for (x=0; x < (ssize_t) image->columns; x+=columns)
          {
            size_t
              column,
              row;

            if (TIFFReadRGBATile(tiff,(uint32) x,(uint32) y,tile_pixels) == 0)
              break;
            columns_remaining=image->columns-x;
            if ((ssize_t) (x+columns) < (ssize_t) image->columns)
              columns_remaining=columns;
            p=tile_pixels+(rows-rows_remaining)*columns;
            q=tile+(image->columns*(rows_remaining-1)+x);
            for (row=rows_remaining; row > 0; row--)
            {
              if (image->matte != MagickFalse)
                for (column=columns_remaining; column > 0; column--)
                {
                  SetPixelRed(q,ScaleCharToQuantum((unsigned char)
                    TIFFGetR(*p)));
                  SetPixelGreen(q,ScaleCharToQuantum((unsigned char)
                    TIFFGetG(*p)));
                  SetPixelBlue(q,ScaleCharToQuantum((unsigned char)
                    TIFFGetB(*p)));
                  SetPixelAlpha(q,ScaleCharToQuantum((unsigned char)
                    TIFFGetA(*p)));
                  q++;
                  p++;
                }
              else
                for (column=columns_remaining; column > 0; column--)
                {
                  SetPixelRed(q,ScaleCharToQuantum((unsigned char)
                    TIFFGetR(*p)));
                  SetPixelGreen(q,ScaleCharToQuantum((unsigned char)
                    TIFFGetG(*p)));
                  SetPixelBlue(q,ScaleCharToQuantum((unsigned char)
                    TIFFGetB(*p)));
                  q++;
                  p++;
                }
              p+=columns-columns_remaining;
              q-=(image->columns+columns_remaining);
            }
          }
          if (SyncAuthenticPixels(image,exception) == MagickFalse)
            break;
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,LoadImageTag,(MagickOffsetType) y,
                image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        tile_pixels=(uint32 *) RelinquishMagickMemory(tile_pixels);
        break;
      }
      case ReadGenericMethod:
      default:
      {
        MemoryInfo
          *pixel_info;

        MagickSizeType
          number_pixels;

        register uint32
          *p;

        uint32
          *pixels;

        /*
          Convert TIFF image to DirectClass MIFF image.
        */
        number_pixels=(MagickSizeType) image->columns*image->rows;
        if (HeapOverflowSanityCheck(image->rows,sizeof(*pixels)) != MagickFalse)
          ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
        pixel_info=AcquireVirtualMemory(image->columns,image->rows*
          sizeof(*pixels));
        if (pixel_info == (MemoryInfo *) NULL)
          ThrowTIFFException(ResourceLimitError,"MemoryAllocationFailed");
        pixels=(uint32 *) GetVirtualMemoryBlob(pixel_info);
        (void) TIFFReadRGBAImage(tiff,(uint32) image->columns,(uint32)
          image->rows,(uint32 *) pixels,0);
        /*
          Convert image to DirectClass pixel packets.
        */
        p=pixels+number_pixels-1;
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          register ssize_t
            x;

          register PixelPacket
            *magick_restrict q;

          q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
          if (q == (PixelPacket *) NULL)
            break;
          q+=image->columns-1;
          for (x=0; x < (ssize_t) image->columns; x++)
          {
            SetPixelRed(q,ScaleCharToQuantum((unsigned char) TIFFGetR(*p)));
            SetPixelGreen(q,ScaleCharToQuantum((unsigned char) TIFFGetG(*p)));
            SetPixelBlue(q,ScaleCharToQuantum((unsigned char) TIFFGetB(*p)));
            if (image->matte != MagickFalse)
              SetPixelAlpha(q,ScaleCharToQuantum((unsigned char) TIFFGetA(*p)));
            p--;
            q--;
          }
          if (SyncAuthenticPixels(image,exception) == MagickFalse)
            break;
          if (image->previous == (Image *) NULL)
            {
              status=SetImageProgress(image,LoadImageTag,(MagickOffsetType) y,
                image->rows);
              if (status == MagickFalse)
                break;
            }
        }
        pixel_info=RelinquishVirtualMemory(pixel_info);
        break;
      }
    }
    tiff_pixels=(unsigned char *) RelinquishMagickMemory(tiff_pixels);
    SetQuantumImageType(image,quantum_type);
  next_tiff_frame:
    if (quantum_info != (QuantumInfo *) NULL)
      quantum_info=DestroyQuantumInfo(quantum_info);
    if (photometric == PHOTOMETRIC_CIELAB)
      DecodeLabImage(image,exception);
    if ((photometric == PHOTOMETRIC_LOGL) ||
        (photometric == PHOTOMETRIC_MINISBLACK) ||
        (photometric == PHOTOMETRIC_MINISWHITE))
      {
        image->type=GrayscaleType;
        if (bits_per_sample == 1)
          image->type=BilevelType;
      }
    /*
      Proceed to next image.
    */
    if (image_info->number_scenes != 0)
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    status=TIFFReadDirectory(tiff) != 0 ? MagickTrue : MagickFalse;
    if (status != MagickFalse)
      {
        /*
          Allocate next image structure.
        */
        AcquireNextImage(image_info,image);
        if (GetNextImageInList(image) == (Image *) NULL)
          {
            image=DestroyImageList(image);
            return((Image *) NULL);
          }
        image=SyncNextImageInList(image);
        status=SetImageProgress(image,LoadImagesTag,image->scene-1,
          image->scene);
        if (status == MagickFalse)
          break;
      }
  } while (status != MagickFalse);
  TIFFClose(tiff);
  TIFFReadPhotoshopLayers(image,image_info,exception);
  if (image_info->number_scenes != 0)
  {
    if (image_info->scene >= GetImageListLength(image))
    {
      /* Subimage was not found in the Photoshop layer */
      image = DestroyImageList(image);
      return((Image *)NULL);
    }
  }
  return(GetFirstImageInList(image));
}