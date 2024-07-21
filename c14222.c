static MagickBooleanType WriteJPEGImage(const ImageInfo *image_info,
  Image *image,ExceptionInfo *exception)
{
  const char
    *option,
    *sampling_factor,
    *value;

  ErrorManager
    error_manager;

  Image
    *volatile volatile_image;

  int
    colorspace,
    quality;

  JSAMPLE
    *volatile jpeg_pixels;

  JSAMPROW
    scanline[1];

  MagickBooleanType
    status;

  MemoryInfo
    *memory_info;

  register JSAMPLE
    *q;

  register ssize_t
    i;

  ssize_t
    y;

  struct jpeg_compress_struct
    jpeg_info;

  struct jpeg_error_mgr
    jpeg_error;

  unsigned short
    scale;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if ((LocaleCompare(image_info->magick,"JPS") == 0) &&
      (image->next != (Image *) NULL))
    image=AppendImages(image,MagickFalse,exception);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,exception);
  if (status == MagickFalse)
    return(status);
  /*
    Initialize JPEG parameters.
  */
  (void) ResetMagickMemory(&error_manager,0,sizeof(error_manager));
  (void) ResetMagickMemory(&jpeg_info,0,sizeof(jpeg_info));
  (void) ResetMagickMemory(&jpeg_error,0,sizeof(jpeg_error));
  volatile_image=image;
  jpeg_info.client_data=(void *) volatile_image;
  jpeg_info.err=jpeg_std_error(&jpeg_error);
  jpeg_info.err->emit_message=(void (*)(j_common_ptr,int)) JPEGWarningHandler;
  jpeg_info.err->error_exit=(void (*)(j_common_ptr)) JPEGErrorHandler;
  error_manager.exception=exception;
  error_manager.image=volatile_image;
  memory_info=(MemoryInfo *) NULL;
  if (setjmp(error_manager.error_recovery) != 0)
    {
      jpeg_destroy_compress(&jpeg_info);
      (void) CloseBlob(volatile_image);
      return(MagickFalse);
    }
  jpeg_info.client_data=(void *) &error_manager;
  jpeg_create_compress(&jpeg_info);
  JPEGDestinationManager(&jpeg_info,image);
  if ((image->columns != (unsigned int) image->columns) ||
      (image->rows != (unsigned int) image->rows))
    ThrowWriterException(ImageError,"WidthOrHeightExceedsLimit");
  jpeg_info.image_width=(unsigned int) image->columns;
  jpeg_info.image_height=(unsigned int) image->rows;
  jpeg_info.input_components=3;
  jpeg_info.data_precision=8;
  jpeg_info.in_color_space=JCS_RGB;
  switch (image->colorspace)
  {
    case CMYKColorspace:
    {
      jpeg_info.input_components=4;
      jpeg_info.in_color_space=JCS_CMYK;
      break;
    }
    case YCbCrColorspace:
    case Rec601YCbCrColorspace:
    case Rec709YCbCrColorspace:
    {
      jpeg_info.in_color_space=JCS_YCbCr;
      break;
    }
    case GRAYColorspace:
    {
      if (image_info->type == TrueColorType)
        break;
      jpeg_info.input_components=1;
      jpeg_info.in_color_space=JCS_GRAYSCALE;
      break;
    }
    default:
    {
      (void) TransformImageColorspace(image,sRGBColorspace,exception);
      if (image_info->type == TrueColorType)
        break;
      if (SetImageGray(image,exception) != MagickFalse)
        {
          jpeg_info.input_components=1;
          jpeg_info.in_color_space=JCS_GRAYSCALE;
        }
      break;
    }
  }
  jpeg_set_defaults(&jpeg_info);
  if (jpeg_info.in_color_space == JCS_CMYK)
    jpeg_set_colorspace(&jpeg_info,JCS_YCCK);
  if ((jpeg_info.data_precision != 12) && (image->depth <= 8))
    jpeg_info.data_precision=8;
  else
    jpeg_info.data_precision=BITS_IN_JSAMPLE;
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "Image resolution: %.20g,%.20g",image->resolution.x,image->resolution.y);
  if ((image->resolution.x != 0.0) && (image->resolution.y != 0.0))
    {
      /*
        Set image resolution.
      */
      jpeg_info.write_JFIF_header=TRUE;
      jpeg_info.X_density=(UINT16) image->resolution.x;
      jpeg_info.Y_density=(UINT16) image->resolution.y;
      /*
        Set image resolution units.
      */
      if (image->units == PixelsPerInchResolution)
        jpeg_info.density_unit=(UINT8) 1;
      if (image->units == PixelsPerCentimeterResolution)
        jpeg_info.density_unit=(UINT8) 2;
    }
  jpeg_info.dct_method=JDCT_FLOAT;
  option=GetImageOption(image_info,"jpeg:dct-method");
  if (option != (const char *) NULL)
    switch (*option)
    {
      case 'D':
      case 'd':
      {
        if (LocaleCompare(option,"default") == 0)
          jpeg_info.dct_method=JDCT_DEFAULT;
        break;
      }
      case 'F':
      case 'f':
      {
        if (LocaleCompare(option,"fastest") == 0)
          jpeg_info.dct_method=JDCT_FASTEST;
        if (LocaleCompare(option,"float") == 0)
          jpeg_info.dct_method=JDCT_FLOAT;
        break;
      }
      case 'I':
      case 'i':
      {
        if (LocaleCompare(option,"ifast") == 0)
          jpeg_info.dct_method=JDCT_IFAST;
        if (LocaleCompare(option,"islow") == 0)
          jpeg_info.dct_method=JDCT_ISLOW;
        break;
      }
    }
  option=GetImageOption(image_info,"jpeg:optimize-coding");
  if (option != (const char *) NULL)
    jpeg_info.optimize_coding=IsStringTrue(option) != MagickFalse ? TRUE :
      FALSE;
  else
    {
      MagickSizeType
        length;

      length=(MagickSizeType) jpeg_info.input_components*image->columns*
        image->rows*sizeof(JSAMPLE);
      if (length == (MagickSizeType) ((size_t) length))
        {
          /*
            Perform optimization only if available memory resources permit it.
          */
          status=AcquireMagickResource(MemoryResource,length);
          RelinquishMagickResource(MemoryResource,length);
          jpeg_info.optimize_coding=status == MagickFalse ? FALSE : TRUE;
        }
    }
#if (JPEG_LIB_VERSION >= 61) && defined(C_PROGRESSIVE_SUPPORTED)
  if ((LocaleCompare(image_info->magick,"PJPEG") == 0) ||
      (image_info->interlace != NoInterlace))
    {
      if (image->debug != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Interlace: progressive");
      jpeg_simple_progression(&jpeg_info);
    }
  else
    if (image->debug != MagickFalse)
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "Interlace: non-progressive");
#else
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),
      "Interlace: nonprogressive");
#endif
  quality=92;
  if ((image_info->compression != LosslessJPEGCompression) &&
      (image->quality <= 100))
    {
      if (image->quality != UndefinedCompressionQuality)
        quality=(int) image->quality;
      if (image->debug != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Quality: %.20g",
          (double) image->quality);
    }
  else
    {
#if !defined(C_LOSSLESS_SUPPORTED)
      quality=100;
      if (image->debug != MagickFalse)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Quality: 100");
#else
      if (image->quality < 100)
        (void) ThrowMagickException(exception,GetMagickModule(),CoderWarning,
          "LosslessToLossyJPEGConversion",image->filename);
      else
        {
          int
            point_transform,
            predictor;

          predictor=image->quality/100;  /* range 1-7 */
          point_transform=image->quality % 20;  /* range 0-15 */
          jpeg_simple_lossless(&jpeg_info,predictor,point_transform);
          if (image->debug != MagickFalse)
            {
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                "Compression: lossless");
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                "Predictor: %d",predictor);
              (void) LogMagickEvent(CoderEvent,GetMagickModule(),
                "Point Transform: %d",point_transform);
            }
        }
#endif
    }
  option=GetImageOption(image_info,"jpeg:extent");
  if (option != (const char *) NULL)
    {
      Image
        *jpeg_image;

      ImageInfo
        *extent_info;

      extent_info=CloneImageInfo(image_info);
      extent_info->blob=NULL;
      jpeg_image=CloneImage(image,0,0,MagickTrue,exception);
      if (jpeg_image != (Image *) NULL)
        {
          MagickSizeType
            extent;

          size_t
            maximum,
            minimum;

          /*
            Search for compression quality that does not exceed image extent.
          */
          extent_info->quality=0;
          extent=(MagickSizeType) SiPrefixToDoubleInterval(option,100.0);
          (void) DeleteImageOption(extent_info,"jpeg:extent");
          (void) DeleteImageArtifact(jpeg_image,"jpeg:extent");
          maximum=image_info->quality;
          if (maximum < 2)
            maximum=101;
          for (minimum=2; minimum < maximum; )
          {
            (void) AcquireUniqueFilename(jpeg_image->filename);
            jpeg_image->quality=minimum+(maximum-minimum+1)/2;
            status=WriteJPEGImage(extent_info,jpeg_image,exception);
            if (GetBlobSize(jpeg_image) <= extent)
              minimum=jpeg_image->quality+1;
            else
              maximum=jpeg_image->quality-1;
            (void) RelinquishUniqueFileResource(jpeg_image->filename);
          }
          quality=(int) minimum-1;
          jpeg_image=DestroyImage(jpeg_image);
        }
      extent_info=DestroyImageInfo(extent_info);
    }
  jpeg_set_quality(&jpeg_info,quality,TRUE);
#if (JPEG_LIB_VERSION >= 70)
  option=GetImageOption(image_info,"quality");
  if (option != (const char *) NULL)
    {
      GeometryInfo
        geometry_info;

      int
        flags;

      /*
        Set quality scaling for luminance and chrominance separately.
      */
      flags=ParseGeometry(option,&geometry_info);
      if (((flags & RhoValue) != 0) && ((flags & SigmaValue) != 0))
        {
          jpeg_info.q_scale_factor[0]=jpeg_quality_scaling((int)
            (geometry_info.rho+0.5));
          jpeg_info.q_scale_factor[1]=jpeg_quality_scaling((int)
            (geometry_info.sigma+0.5));
          jpeg_default_qtables(&jpeg_info,TRUE);
        }
    }
#endif
  colorspace=jpeg_info.in_color_space;
  value=GetImageOption(image_info,"jpeg:colorspace");
  if (value == (char *) NULL)
    value=GetImageProperty(image,"jpeg:colorspace",exception);
  if (value != (char *) NULL)
    colorspace=StringToInteger(value);
  sampling_factor=(const char *) NULL;
  if (colorspace == jpeg_info.in_color_space)
    {
      value=GetImageOption(image_info,"jpeg:sampling-factor");
      if (value == (char *) NULL)
        value=GetImageProperty(image,"jpeg:sampling-factor",exception);
      if (value != (char *) NULL)
        {
          sampling_factor=value;
          if (image->debug != MagickFalse)
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "  Input sampling-factors=%s",sampling_factor);
        }
    }
  value=GetImageOption(image_info,"jpeg:sampling-factor");
  if (image_info->sampling_factor != (char *) NULL)
    sampling_factor=image_info->sampling_factor;
  if (sampling_factor == (const char *) NULL)
    {
      if (quality >= 90)
        for (i=0; i < MAX_COMPONENTS; i++)
        {
          jpeg_info.comp_info[i].h_samp_factor=1;
          jpeg_info.comp_info[i].v_samp_factor=1;
        }
    }
  else
    {
      char
        **factors;

      GeometryInfo
        geometry_info;

      MagickStatusType
        flags;

      /*
        Set sampling factor.
      */
      i=0;
      factors=SamplingFactorToList(sampling_factor);
      if (factors != (char **) NULL)
        {
          for (i=0; i < MAX_COMPONENTS; i++)
          {
            if (factors[i] == (char *) NULL)
              break;
            flags=ParseGeometry(factors[i],&geometry_info);
            if ((flags & SigmaValue) == 0)
              geometry_info.sigma=geometry_info.rho;
            jpeg_info.comp_info[i].h_samp_factor=(int) geometry_info.rho;
            jpeg_info.comp_info[i].v_samp_factor=(int) geometry_info.sigma;
            factors[i]=(char *) RelinquishMagickMemory(factors[i]);
          }
          factors=(char **) RelinquishMagickMemory(factors);
        }
      for ( ; i < MAX_COMPONENTS; i++)
      {
        jpeg_info.comp_info[i].h_samp_factor=1;
        jpeg_info.comp_info[i].v_samp_factor=1;
      }
    }
  option=GetImageOption(image_info,"jpeg:q-table");
  if (option != (const char *) NULL)
    {
      QuantizationTable
        *table;

      /*
        Custom quantization tables.
      */
      table=GetQuantizationTable(option,"0",exception);
      if (table != (QuantizationTable *) NULL)
        {
          for (i=0; i < MAX_COMPONENTS; i++)
            jpeg_info.comp_info[i].quant_tbl_no=0;
          jpeg_add_quant_table(&jpeg_info,0,table->levels,
            jpeg_quality_scaling(quality),0);
          table=DestroyQuantizationTable(table);
        }
      table=GetQuantizationTable(option,"1",exception);
      if (table != (QuantizationTable *) NULL)
        {
          for (i=1; i < MAX_COMPONENTS; i++)
            jpeg_info.comp_info[i].quant_tbl_no=1;
          jpeg_add_quant_table(&jpeg_info,1,table->levels,
            jpeg_quality_scaling(quality),0);
          table=DestroyQuantizationTable(table);
        }
      table=GetQuantizationTable(option,"2",exception);
      if (table != (QuantizationTable *) NULL)
        {
          for (i=2; i < MAX_COMPONENTS; i++)
            jpeg_info.comp_info[i].quant_tbl_no=2;
          jpeg_add_quant_table(&jpeg_info,2,table->levels,
            jpeg_quality_scaling(quality),0);
          table=DestroyQuantizationTable(table);
        }
      table=GetQuantizationTable(option,"3",exception);
      if (table != (QuantizationTable *) NULL)
        {
          for (i=3; i < MAX_COMPONENTS; i++)
            jpeg_info.comp_info[i].quant_tbl_no=3;
          jpeg_add_quant_table(&jpeg_info,3,table->levels,
            jpeg_quality_scaling(quality),0);
          table=DestroyQuantizationTable(table);
        }
    }
  jpeg_start_compress(&jpeg_info,TRUE);
  if (image->debug != MagickFalse)
    {
      if (image->storage_class == PseudoClass)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Storage class: PseudoClass");
      else
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Storage class: DirectClass");
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Depth: %.20g",
        (double) image->depth);
      if (image->colors != 0)
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Number of colors: %.20g",(double) image->colors);
      else
        (void) LogMagickEvent(CoderEvent,GetMagickModule(),
          "Number of colors: unspecified");
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "JPEG data precision: %d",(int) jpeg_info.data_precision);
      switch (image->colorspace)
      {
        case CMYKColorspace:
        {
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Storage class: DirectClass");
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Colorspace: CMYK");
          break;
        }
        case YCbCrColorspace:
        case Rec601YCbCrColorspace:
        case Rec709YCbCrColorspace:
        {
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Colorspace: YCbCr");
          break;
        }
        default:
          break;
      }
      switch (image->colorspace)
      {
        case CMYKColorspace:
        {
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Colorspace: CMYK");
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Sampling factors: %dx%d,%dx%d,%dx%d,%dx%d",
            jpeg_info.comp_info[0].h_samp_factor,
            jpeg_info.comp_info[0].v_samp_factor,
            jpeg_info.comp_info[1].h_samp_factor,
            jpeg_info.comp_info[1].v_samp_factor,
            jpeg_info.comp_info[2].h_samp_factor,
            jpeg_info.comp_info[2].v_samp_factor,
            jpeg_info.comp_info[3].h_samp_factor,
            jpeg_info.comp_info[3].v_samp_factor);
          break;
        }
        case GRAYColorspace:
        {
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Colorspace: GRAY");
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Sampling factors: %dx%d",jpeg_info.comp_info[0].h_samp_factor,
            jpeg_info.comp_info[0].v_samp_factor);
          break;
        }
        case sRGBColorspace:
        case RGBColorspace:
        {
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Image colorspace is RGB");
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Sampling factors: %dx%d,%dx%d,%dx%d",
            jpeg_info.comp_info[0].h_samp_factor,
            jpeg_info.comp_info[0].v_samp_factor,
            jpeg_info.comp_info[1].h_samp_factor,
            jpeg_info.comp_info[1].v_samp_factor,
            jpeg_info.comp_info[2].h_samp_factor,
            jpeg_info.comp_info[2].v_samp_factor);
          break;
        }
        case YCbCrColorspace:
        case Rec601YCbCrColorspace:
        case Rec709YCbCrColorspace:
        {
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Colorspace: YCbCr");
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Sampling factors: %dx%d,%dx%d,%dx%d",
            jpeg_info.comp_info[0].h_samp_factor,
            jpeg_info.comp_info[0].v_samp_factor,
            jpeg_info.comp_info[1].h_samp_factor,
            jpeg_info.comp_info[1].v_samp_factor,
            jpeg_info.comp_info[2].h_samp_factor,
            jpeg_info.comp_info[2].v_samp_factor);
          break;
        }
        default:
        {
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),"Colorspace: %d",
            image->colorspace);
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Sampling factors: %dx%d,%dx%d,%dx%d,%dx%d",
            jpeg_info.comp_info[0].h_samp_factor,
            jpeg_info.comp_info[0].v_samp_factor,
            jpeg_info.comp_info[1].h_samp_factor,
            jpeg_info.comp_info[1].v_samp_factor,
            jpeg_info.comp_info[2].h_samp_factor,
            jpeg_info.comp_info[2].v_samp_factor,
            jpeg_info.comp_info[3].h_samp_factor,
            jpeg_info.comp_info[3].v_samp_factor);
          break;
        }
      }
    }
  /*
    Write JPEG profiles.
  */
  value=GetImageProperty(image,"comment",exception);
  if (value != (char *) NULL)
    for (i=0; i < (ssize_t) strlen(value); i+=65533L)
      jpeg_write_marker(&jpeg_info,JPEG_COM,(unsigned char *) value+i,
        (unsigned int) MagickMin((size_t) strlen(value+i),65533L));
  if (image->profiles != (void *) NULL)
    WriteProfile(&jpeg_info,image);
  /*
    Convert MIFF to JPEG raster pixels.
  */
  memory_info=AcquireVirtualMemory((size_t) image->columns,
    jpeg_info.input_components*sizeof(*jpeg_pixels));
  if (memory_info == (MemoryInfo *) NULL)
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
  jpeg_pixels=(JSAMPLE *) GetVirtualMemoryBlob(memory_info);
  if (setjmp(error_manager.error_recovery) != 0)
    {
      jpeg_destroy_compress(&jpeg_info);
      if (memory_info != (MemoryInfo *) NULL)
        memory_info=RelinquishVirtualMemory(memory_info);
      (void) CloseBlob(image);
      return(MagickFalse);
    }
  scanline[0]=(JSAMPROW) jpeg_pixels;
  scale=65535/(unsigned short) GetQuantumRange((size_t)
    jpeg_info.data_precision);
  if (scale == 0)
    scale=1;
  if (jpeg_info.data_precision <= 8)
    {
      if ((jpeg_info.in_color_space == JCS_RGB) ||
          (jpeg_info.in_color_space == JCS_YCbCr))
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          register const Quantum
            *p;

          register ssize_t
            x;

          p=GetVirtualPixels(image,0,y,image->columns,1,exception);
          if (p == (const Quantum *) NULL)
            break;
          q=jpeg_pixels;
          for (x=0; x < (ssize_t) image->columns; x++)
          {
            *q++=(JSAMPLE) ScaleQuantumToChar(GetPixelRed(image,p));
            *q++=(JSAMPLE) ScaleQuantumToChar(GetPixelGreen(image,p));
            *q++=(JSAMPLE) ScaleQuantumToChar(GetPixelBlue(image,p));
            p+=GetPixelChannels(image);
          }
          (void) jpeg_write_scanlines(&jpeg_info,scanline,1);
          status=SetImageProgress(image,SaveImageTag,(MagickOffsetType) y,
            image->rows);
          if (status == MagickFalse)
            break;
        }
      else
        if (jpeg_info.in_color_space == JCS_GRAYSCALE)
          for (y=0; y < (ssize_t) image->rows; y++)
          {
            register const Quantum
              *p;

            register ssize_t
              x;

            p=GetVirtualPixels(image,0,y,image->columns,1,exception);
            if (p == (const Quantum *) NULL)
              break;
            q=jpeg_pixels;
            for (x=0; x < (ssize_t) image->columns; x++)
            {
              *q++=(JSAMPLE) ScaleQuantumToChar(ClampToQuantum(GetPixelLuma(
                image,p)));
              p+=GetPixelChannels(image);
            }
            (void) jpeg_write_scanlines(&jpeg_info,scanline,1);
            status=SetImageProgress(image,SaveImageTag,(MagickOffsetType) y,
              image->rows);
            if (status == MagickFalse)
              break;
            }
        else
          for (y=0; y < (ssize_t) image->rows; y++)
          {
            register const Quantum
              *p;

            register ssize_t
              x;

            p=GetVirtualPixels(image,0,y,image->columns,1,exception);
            if (p == (const Quantum *) NULL)
              break;
            q=jpeg_pixels;
            for (x=0; x < (ssize_t) image->columns; x++)
            {
              /*
                Convert DirectClass packets to contiguous CMYK scanlines.
              */
              *q++=(JSAMPLE) (ScaleQuantumToChar((Quantum) (QuantumRange-
                GetPixelCyan(image,p))));
              *q++=(JSAMPLE) (ScaleQuantumToChar((Quantum) (QuantumRange-
                GetPixelMagenta(image,p))));
              *q++=(JSAMPLE) (ScaleQuantumToChar((Quantum) (QuantumRange-
                GetPixelYellow(image,p))));
              *q++=(JSAMPLE) (ScaleQuantumToChar((Quantum) (QuantumRange-
                GetPixelBlack(image,p))));
              p+=GetPixelChannels(image);
            }
            (void) jpeg_write_scanlines(&jpeg_info,scanline,1);
            status=SetImageProgress(image,SaveImageTag,(MagickOffsetType) y,
              image->rows);
            if (status == MagickFalse)
              break;
          }
    }
  else
    if (jpeg_info.in_color_space == JCS_GRAYSCALE)
      for (y=0; y < (ssize_t) image->rows; y++)
      {
        register const Quantum
          *p;

        register ssize_t
          x;

        p=GetVirtualPixels(image,0,y,image->columns,1,exception);
        if (p == (const Quantum *) NULL)
          break;
        q=jpeg_pixels;
        for (x=0; x < (ssize_t) image->columns; x++)
        {
          *q++=(JSAMPLE) (ScaleQuantumToShort(ClampToQuantum(GetPixelLuma(image,
            p)))/scale);
          p+=GetPixelChannels(image);
        }
        (void) jpeg_write_scanlines(&jpeg_info,scanline,1);
        status=SetImageProgress(image,SaveImageTag,(MagickOffsetType) y,
          image->rows);
        if (status == MagickFalse)
          break;
      }
    else
      if ((jpeg_info.in_color_space == JCS_RGB) ||
          (jpeg_info.in_color_space == JCS_YCbCr))
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          register const Quantum
            *p;

          register ssize_t
            x;

          p=GetVirtualPixels(image,0,y,image->columns,1,exception);
          if (p == (const Quantum *) NULL)
            break;
          q=jpeg_pixels;
          for (x=0; x < (ssize_t) image->columns; x++)
          {
            *q++=(JSAMPLE) (ScaleQuantumToShort(GetPixelRed(image,p))/scale);
            *q++=(JSAMPLE) (ScaleQuantumToShort(GetPixelGreen(image,p))/scale);
            *q++=(JSAMPLE) (ScaleQuantumToShort(GetPixelBlue(image,p))/scale);
            p+=GetPixelChannels(image);
          }
          (void) jpeg_write_scanlines(&jpeg_info,scanline,1);
          status=SetImageProgress(image,SaveImageTag,(MagickOffsetType) y,
            image->rows);
          if (status == MagickFalse)
            break;
        }
      else
        for (y=0; y < (ssize_t) image->rows; y++)
        {
          register const Quantum
            *p;

          register ssize_t
            x;

          p=GetVirtualPixels(image,0,y,image->columns,1,exception);
          if (p == (const Quantum *) NULL)
            break;
          q=jpeg_pixels;
          for (x=0; x < (ssize_t) image->columns; x++)
          {
            /*
              Convert DirectClass packets to contiguous CMYK scanlines.
            */
            *q++=(JSAMPLE) (ScaleQuantumToShort(QuantumRange-GetPixelRed(
              image,p))/scale);
            *q++=(JSAMPLE) (ScaleQuantumToShort(QuantumRange-GetPixelGreen(
              image,p))/scale);
            *q++=(JSAMPLE) (ScaleQuantumToShort(QuantumRange-GetPixelBlue(
              image,p))/scale);
            *q++=(JSAMPLE) (ScaleQuantumToShort(QuantumRange-GetPixelBlack(
              image,p))/scale);
            p+=GetPixelChannels(image);
          }
          (void) jpeg_write_scanlines(&jpeg_info,scanline,1);
          status=SetImageProgress(image,SaveImageTag,(MagickOffsetType) y,
            image->rows);
          if (status == MagickFalse)
            break;
        }
  if (y == (ssize_t) image->rows)
    jpeg_finish_compress(&jpeg_info);
  /*
    Relinquish resources.
  */
  jpeg_destroy_compress(&jpeg_info);
  memory_info=RelinquishVirtualMemory(memory_info);
  (void) CloseBlob(image);
  return(MagickTrue);
}