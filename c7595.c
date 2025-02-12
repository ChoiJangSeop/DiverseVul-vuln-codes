static Image *ReadDPXImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  char
    magick[4],
    value[MaxTextExtent];

  DPXInfo
    dpx;

  Image
    *image;

  MagickBooleanType
    status;

  MagickOffsetType
    offset;

  QuantumInfo
    *quantum_info;

  QuantumType
    quantum_type;

  register ssize_t
    i;

  size_t
    extent,
    samples_per_pixel;

  ssize_t
    count,
    n,
    row,
    y;

  unsigned char
    component_type;

  /*
    Open image file.
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
  /*
    Read DPX file header.
  */
  offset=0;
  count=ReadBlob(image,4,(unsigned char *) magick);
  offset+=count;
  if ((count != 4) || ((LocaleNCompare(magick,"SDPX",4) != 0) &&
      (LocaleNCompare((char *) magick,"XPDS",4) != 0)))
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  image->endian=LSBEndian;
  if (LocaleNCompare(magick,"SDPX",4) == 0)
    image->endian=MSBEndian;
  (void) memset(&dpx,0,sizeof(dpx));
  dpx.file.image_offset=ReadBlobLong(image);
  offset+=4;
  offset+=ReadBlob(image,sizeof(dpx.file.version),(unsigned char *)
    dpx.file.version);
  (void) FormatImageProperty(image,"dpx:file.version","%.8s",dpx.file.version);
  dpx.file.file_size=ReadBlobLong(image);
  if (0 && dpx.file.file_size > GetBlobSize(image))
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  offset+=4;
  dpx.file.ditto_key=ReadBlobLong(image);
  offset+=4;
  if (dpx.file.ditto_key != ~0U)
    (void) FormatImageProperty(image,"dpx:file.ditto.key","%u",
      dpx.file.ditto_key);
  dpx.file.generic_size=ReadBlobLong(image);
  if (0 && dpx.file.generic_size > GetBlobSize(image))
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  offset+=4;
  dpx.file.industry_size=ReadBlobLong(image);
  if (0 && dpx.file.industry_size > GetBlobSize(image))
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  offset+=4;
  dpx.file.user_size=ReadBlobLong(image);
  if (0 && dpx.file.user_size > GetBlobSize(image))
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  offset+=4;
  offset+=ReadBlob(image,sizeof(dpx.file.filename),(unsigned char *)
    dpx.file.filename);
  (void) FormatImageProperty(image,"dpx:file.filename","%.100s",
    dpx.file.filename);
  (void) FormatImageProperty(image,"document","%.100s",dpx.file.filename);
  offset+=ReadBlob(image,sizeof(dpx.file.timestamp),(unsigned char *)
    dpx.file.timestamp);
  if (*dpx.file.timestamp != '\0')
    (void) FormatImageProperty(image,"dpx:file.timestamp","%.24s",
      dpx.file.timestamp);
  offset+=ReadBlob(image,sizeof(dpx.file.creator),(unsigned char *)
    dpx.file.creator);
  if (*dpx.file.creator == '\0')
    {
      char
        *url;

      url=GetMagickHomeURL();
      (void) FormatImageProperty(image,"dpx:file.creator","%.100s",url);
      (void) FormatImageProperty(image,"software","%.100s",url);
      url=DestroyString(url);
    }
  else
    {
      (void) FormatImageProperty(image,"dpx:file.creator","%.100s",
        dpx.file.creator);
      (void) FormatImageProperty(image,"software","%.100s",dpx.file.creator);
    }
  offset+=ReadBlob(image,sizeof(dpx.file.project),(unsigned char *)
    dpx.file.project);
  if (*dpx.file.project != '\0')
    {
      (void) FormatImageProperty(image,"dpx:file.project","%.200s",
        dpx.file.project);
      (void) FormatImageProperty(image,"comment","%.100s",dpx.file.project);
    }
  offset+=ReadBlob(image,sizeof(dpx.file.copyright),(unsigned char *)
    dpx.file.copyright);
  if (*dpx.file.copyright != '\0')
    {
      (void) FormatImageProperty(image,"dpx:file.copyright","%.200s",
        dpx.file.copyright);
      (void) FormatImageProperty(image,"copyright","%.100s",
        dpx.file.copyright);
    }
  dpx.file.encrypt_key=ReadBlobLong(image);
  offset+=4;
  if (dpx.file.encrypt_key != ~0U)
    (void) FormatImageProperty(image,"dpx:file.encrypt_key","%u",
      dpx.file.encrypt_key);
  offset+=ReadBlob(image,sizeof(dpx.file.reserve),(unsigned char *)
    dpx.file.reserve);
  /*
    Read DPX image header.
  */
  dpx.image.orientation=ReadBlobShort(image);
  if (dpx.image.orientation > 7)
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  offset+=2;
  if (dpx.image.orientation != (unsigned short) ~0)
    (void) FormatImageProperty(image,"dpx:image.orientation","%d",
      dpx.image.orientation);
  switch (dpx.image.orientation)
  {
    default:
    case 0: image->orientation=TopLeftOrientation; break;
    case 1: image->orientation=TopRightOrientation; break;
    case 2: image->orientation=BottomLeftOrientation; break;
    case 3: image->orientation=BottomRightOrientation; break;
    case 4: image->orientation=LeftTopOrientation; break;
    case 5: image->orientation=RightTopOrientation; break;
    case 6: image->orientation=LeftBottomOrientation; break;
    case 7: image->orientation=RightBottomOrientation; break;
  }
  dpx.image.number_elements=ReadBlobShort(image);
  if ((dpx.image.number_elements < 1) ||
      (dpx.image.number_elements > MaxNumberImageElements))
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  offset+=2;
  dpx.image.pixels_per_line=ReadBlobLong(image);
  offset+=4;
  image->columns=dpx.image.pixels_per_line;
  dpx.image.lines_per_element=ReadBlobLong(image);
  offset+=4;
  image->rows=dpx.image.lines_per_element;
  for (i=0; i < 8; i++)
  {
    char
      property[MaxTextExtent];

    dpx.image.image_element[i].data_sign=ReadBlobLong(image);
    offset+=4;
    dpx.image.image_element[i].low_data=ReadBlobLong(image);
    offset+=4;
    dpx.image.image_element[i].low_quantity=ReadBlobFloat(image);
    offset+=4;
    dpx.image.image_element[i].high_data=ReadBlobLong(image);
    offset+=4;
    dpx.image.image_element[i].high_quantity=ReadBlobFloat(image);
    offset+=4;
    dpx.image.image_element[i].descriptor=(unsigned char) ReadBlobByte(image);
    offset++;
    dpx.image.image_element[i].transfer_characteristic=(unsigned char)
      ReadBlobByte(image);
    (void) FormatLocaleString(property,MaxTextExtent,
      "dpx:image.element[%lu].transfer-characteristic",(long) i);
    (void) FormatImageProperty(image,property,"%s",
      GetImageTransferCharacteristic((DPXTransferCharacteristic)
      dpx.image.image_element[i].transfer_characteristic));
    offset++;
    dpx.image.image_element[i].colorimetric=(unsigned char) ReadBlobByte(image);
    offset++;
    dpx.image.image_element[i].bit_size=(unsigned char) ReadBlobByte(image);
    offset++;
    dpx.image.image_element[i].packing=ReadBlobShort(image);
    if (dpx.image.image_element[i].packing > 2)
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    offset+=2;
    dpx.image.image_element[i].encoding=ReadBlobShort(image);
    offset+=2;
    dpx.image.image_element[i].data_offset=ReadBlobLong(image);
    offset+=4;
    dpx.image.image_element[i].end_of_line_padding=ReadBlobLong(image);
    offset+=4;
    dpx.image.image_element[i].end_of_image_padding=ReadBlobLong(image);
    offset+=4;
    offset+=ReadBlob(image,sizeof(dpx.image.image_element[i].description),
      (unsigned char *) dpx.image.image_element[i].description);
  }
  (void) SetImageColorspace(image,RGBColorspace);
  offset+=ReadBlob(image,sizeof(dpx.image.reserve),(unsigned char *)
    dpx.image.reserve);
  if (dpx.file.image_offset >= 1664U)
    {
      /*
        Read DPX orientation header.
      */
      dpx.orientation.x_offset=ReadBlobLong(image);
      offset+=4;
      if (dpx.orientation.x_offset != ~0U)
        (void) FormatImageProperty(image,"dpx:orientation.x_offset","%u",
          dpx.orientation.x_offset);
      dpx.orientation.y_offset=ReadBlobLong(image);
      offset+=4;
      if (dpx.orientation.y_offset != ~0U)
        (void) FormatImageProperty(image,"dpx:orientation.y_offset","%u",
          dpx.orientation.y_offset);
      dpx.orientation.x_center=ReadBlobFloat(image);
      offset+=4;
      if (IsFloatDefined(dpx.orientation.x_center) != MagickFalse)
        (void) FormatImageProperty(image,"dpx:orientation.x_center","%g",
          dpx.orientation.x_center);
      dpx.orientation.y_center=ReadBlobFloat(image);
      offset+=4;
      if (IsFloatDefined(dpx.orientation.y_center) != MagickFalse)
        (void) FormatImageProperty(image,"dpx:orientation.y_center","%g",
          dpx.orientation.y_center);
      dpx.orientation.x_size=ReadBlobLong(image);
      offset+=4;
      if (dpx.orientation.x_size != ~0U)
        (void) FormatImageProperty(image,"dpx:orientation.x_size","%u",
          dpx.orientation.x_size);
      dpx.orientation.y_size=ReadBlobLong(image);
      offset+=4;
      if (dpx.orientation.y_size != ~0U)
        (void) FormatImageProperty(image,"dpx:orientation.y_size","%u",
          dpx.orientation.y_size);
      offset+=ReadBlob(image,sizeof(dpx.orientation.filename),(unsigned char *)
        dpx.orientation.filename);
      if (*dpx.orientation.filename != '\0')
        (void) FormatImageProperty(image,"dpx:orientation.filename","%.100s",
          dpx.orientation.filename);
      offset+=ReadBlob(image,sizeof(dpx.orientation.timestamp),(unsigned char *)
        dpx.orientation.timestamp);
      if (*dpx.orientation.timestamp != '\0')
        (void) FormatImageProperty(image,"dpx:orientation.timestamp","%.24s",
          dpx.orientation.timestamp);
      offset+=ReadBlob(image,sizeof(dpx.orientation.device),(unsigned char *)
        dpx.orientation.device);
      if (*dpx.orientation.device != '\0')
        (void) FormatImageProperty(image,"dpx:orientation.device","%.32s",
          dpx.orientation.device);
      offset+=ReadBlob(image,sizeof(dpx.orientation.serial),(unsigned char *)
        dpx.orientation.serial);
      if (*dpx.orientation.serial != '\0')
        (void) FormatImageProperty(image,"dpx:orientation.serial","%.32s",
          dpx.orientation.serial);
      for (i=0; i < 4; i++)
      {
        dpx.orientation.border[i]=ReadBlobShort(image);
        offset+=2;
      }
      if ((dpx.orientation.border[0] != (unsigned short) (~0)) &&
          (dpx.orientation.border[1] != (unsigned short) (~0)))
        (void) FormatImageProperty(image,"dpx:orientation.border","%dx%d%+d%+d",          dpx.orientation.border[0],dpx.orientation.border[1],
          dpx.orientation.border[2],dpx.orientation.border[3]);
      for (i=0; i < 2; i++)
      {
        dpx.orientation.aspect_ratio[i]=ReadBlobLong(image);
        offset+=4;
      }
      if ((dpx.orientation.aspect_ratio[0] != ~0U) &&
          (dpx.orientation.aspect_ratio[1] != ~0U))
        (void) FormatImageProperty(image,"dpx:orientation.aspect_ratio",
          "%ux%u",dpx.orientation.aspect_ratio[0],
          dpx.orientation.aspect_ratio[1]);
      offset+=ReadBlob(image,sizeof(dpx.orientation.reserve),(unsigned char *)
        dpx.orientation.reserve);
    }
  if (dpx.file.image_offset >= 1920U)
    {
      /*
        Read DPX film header.
      */
      offset+=ReadBlob(image,sizeof(dpx.film.id),(unsigned char *) dpx.film.id);
      if (*dpx.film.id != '\0')
        (void) FormatImageProperty(image,"dpx:film.id","%.2s",dpx.film.id);
      offset+=ReadBlob(image,sizeof(dpx.film.type),(unsigned char *)
        dpx.film.type);
      if (*dpx.film.type != '\0')
        (void) FormatImageProperty(image,"dpx:film.type","%.2s",dpx.film.type);
      offset+=ReadBlob(image,sizeof(dpx.film.offset),(unsigned char *)
        dpx.film.offset);
      if (*dpx.film.offset != '\0')
        (void) FormatImageProperty(image,"dpx:film.offset","%.2s",
          dpx.film.offset);
      offset+=ReadBlob(image,sizeof(dpx.film.prefix),(unsigned char *)
        dpx.film.prefix);
      if (*dpx.film.prefix != '\0')
        (void) FormatImageProperty(image,"dpx:film.prefix","%.6s",
          dpx.film.prefix);
      offset+=ReadBlob(image,sizeof(dpx.film.count),(unsigned char *)
        dpx.film.count);
      if (*dpx.film.count != '\0')
        (void) FormatImageProperty(image,"dpx:film.count","%.4s",
          dpx.film.count);
      offset+=ReadBlob(image,sizeof(dpx.film.format),(unsigned char *)
        dpx.film.format);
      if (*dpx.film.format != '\0')
        (void) FormatImageProperty(image,"dpx:film.format","%.4s",
          dpx.film.format);
      dpx.film.frame_position=ReadBlobLong(image);
      offset+=4;
      if (dpx.film.frame_position != ~0U)
        (void) FormatImageProperty(image,"dpx:film.frame_position","%u",
          dpx.film.frame_position);
      dpx.film.sequence_extent=ReadBlobLong(image);
      offset+=4;
      if (dpx.film.sequence_extent != ~0U)
        (void) FormatImageProperty(image,"dpx:film.sequence_extent","%u",
          dpx.film.sequence_extent);
      dpx.film.held_count=ReadBlobLong(image);
      offset+=4;
      if (dpx.film.held_count != ~0U)
        (void) FormatImageProperty(image,"dpx:film.held_count","%u",
          dpx.film.held_count);
      dpx.film.frame_rate=ReadBlobFloat(image);
      offset+=4;
      if (IsFloatDefined(dpx.film.frame_rate) != MagickFalse)
        (void) FormatImageProperty(image,"dpx:film.frame_rate","%g",
          dpx.film.frame_rate);
      dpx.film.shutter_angle=ReadBlobFloat(image);
      offset+=4;
      if (IsFloatDefined(dpx.film.shutter_angle) != MagickFalse)
        (void) FormatImageProperty(image,"dpx:film.shutter_angle","%g",
          dpx.film.shutter_angle);
      offset+=ReadBlob(image,sizeof(dpx.film.frame_id),(unsigned char *)
        dpx.film.frame_id);
      if (*dpx.film.frame_id != '\0')
        (void) FormatImageProperty(image,"dpx:film.frame_id","%.32s",
          dpx.film.frame_id);
      offset+=ReadBlob(image,sizeof(dpx.film.slate),(unsigned char *)
        dpx.film.slate);
      if (*dpx.film.slate != '\0')
        (void) FormatImageProperty(image,"dpx:film.slate","%.100s",
          dpx.film.slate);
      offset+=ReadBlob(image,sizeof(dpx.film.reserve),(unsigned char *)
        dpx.film.reserve);
    }
  if (dpx.file.image_offset >= 2048U)
    {
      /*
        Read DPX television header.
      */
      dpx.television.time_code=(unsigned int) ReadBlobLong(image);
      offset+=4;
      TimeCodeToString(dpx.television.time_code,value);
      (void) SetImageProperty(image,"dpx:television.time.code",value);
      dpx.television.user_bits=(unsigned int) ReadBlobLong(image);
      offset+=4;
      TimeCodeToString(dpx.television.user_bits,value);
      (void) SetImageProperty(image,"dpx:television.user.bits",value);
      dpx.television.interlace=(unsigned char) ReadBlobByte(image);
      offset++;
      if (dpx.television.interlace != 0)
        (void) FormatImageProperty(image,"dpx:television.interlace","%.20g",
          (double) dpx.television.interlace);
      dpx.television.field_number=(unsigned char) ReadBlobByte(image);
      offset++;
      if (dpx.television.field_number != 0)
        (void) FormatImageProperty(image,"dpx:television.field_number","%.20g",
          (double) dpx.television.field_number);
      dpx.television.video_signal=(unsigned char) ReadBlobByte(image);
      offset++;
      if (dpx.television.video_signal != 0)
        (void) FormatImageProperty(image,"dpx:television.video_signal","%.20g",
          (double) dpx.television.video_signal);
      dpx.television.padding=(unsigned char) ReadBlobByte(image);
      offset++;
      if (dpx.television.padding != 0)
        (void) FormatImageProperty(image,"dpx:television.padding","%d",
          dpx.television.padding);
      dpx.television.horizontal_sample_rate=ReadBlobFloat(image);
      offset+=4;
      if (IsFloatDefined(dpx.television.horizontal_sample_rate) != MagickFalse)
        (void) FormatImageProperty(image,
          "dpx:television.horizontal_sample_rate","%g",
          dpx.television.horizontal_sample_rate);
      dpx.television.vertical_sample_rate=ReadBlobFloat(image);
      offset+=4;
      if (IsFloatDefined(dpx.television.vertical_sample_rate) != MagickFalse)
        (void) FormatImageProperty(image,"dpx:television.vertical_sample_rate",
          "%g",dpx.television.vertical_sample_rate);
      dpx.television.frame_rate=ReadBlobFloat(image);
      offset+=4;
      if (IsFloatDefined(dpx.television.frame_rate) != MagickFalse)
        (void) FormatImageProperty(image,"dpx:television.frame_rate","%g",
          dpx.television.frame_rate);
      dpx.television.time_offset=ReadBlobFloat(image);
      offset+=4;
      if (IsFloatDefined(dpx.television.time_offset) != MagickFalse)
        (void) FormatImageProperty(image,"dpx:television.time_offset","%g",
          dpx.television.time_offset);
      dpx.television.gamma=ReadBlobFloat(image);
      offset+=4;
      if (IsFloatDefined(dpx.television.gamma) != MagickFalse)
        (void) FormatImageProperty(image,"dpx:television.gamma","%g",
          dpx.television.gamma);
      dpx.television.black_level=ReadBlobFloat(image);
      offset+=4;
      if (IsFloatDefined(dpx.television.black_level) != MagickFalse)
        (void) FormatImageProperty(image,"dpx:television.black_level","%g",
          dpx.television.black_level);
      dpx.television.black_gain=ReadBlobFloat(image);
      offset+=4;
      if (IsFloatDefined(dpx.television.black_gain) != MagickFalse)
        (void) FormatImageProperty(image,"dpx:television.black_gain","%g",
          dpx.television.black_gain);
      dpx.television.break_point=ReadBlobFloat(image);
      offset+=4;
      if (IsFloatDefined(dpx.television.break_point) != MagickFalse)
        (void) FormatImageProperty(image,"dpx:television.break_point","%g",
          dpx.television.break_point);
      dpx.television.white_level=ReadBlobFloat(image);
      offset+=4;
      if (IsFloatDefined(dpx.television.white_level) != MagickFalse)
        (void) FormatImageProperty(image,"dpx:television.white_level","%g",
          dpx.television.white_level);
      dpx.television.integration_times=ReadBlobFloat(image);
      offset+=4;
      if (IsFloatDefined(dpx.television.integration_times) != MagickFalse)
        (void) FormatImageProperty(image,"dpx:television.integration_times",
          "%g",dpx.television.integration_times);
      offset+=ReadBlob(image,sizeof(dpx.television.reserve),(unsigned char *)
        dpx.television.reserve);
    }
  if (dpx.file.image_offset > 2080U)
    {
      /*
        Read DPX user header.
      */
      offset+=ReadBlob(image,sizeof(dpx.user.id),(unsigned char *) dpx.user.id);
      if (*dpx.user.id != '\0')
        (void) FormatImageProperty(image,"dpx:user.id","%.32s",dpx.user.id);
      if ((dpx.file.user_size != ~0U) &&
          ((size_t) dpx.file.user_size > sizeof(dpx.user.id)))
        {
          StringInfo
            *profile;

           if (dpx.file.user_size > GetBlobSize(image))
             ThrowReaderException(CorruptImageError,
               "InsufficientImageDataInFile");
           profile=BlobToStringInfo((const void *) NULL,
             dpx.file.user_size-sizeof(dpx.user.id));
           if (profile == (StringInfo *) NULL)
             ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
           offset+=ReadBlob(image,GetStringInfoLength(profile),
             GetStringInfoDatum(profile));
           if (EOFBlob(image) != MagickFalse)
             (void) SetImageProfile(image,"dpx:user-data",profile);
           profile=DestroyStringInfo(profile);
        }
    }
  for ( ; offset < (MagickOffsetType) dpx.file.image_offset; offset++)
    if (ReadBlobByte(image) == EOF)
      break;
  if (EOFBlob(image) != MagickFalse)
    ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
      image->filename);
  if (image_info->ping != MagickFalse)
    {
      (void) CloseBlob(image);
      return(GetFirstImageInList(image));
    }
  status=SetImageExtent(image,image->columns,image->rows);
  if (status == MagickFalse)
    {
      InheritException(exception,&image->exception);
      return(DestroyImageList(image));
    }
  status=ResetImagePixels(image,exception);
  if (status == MagickFalse)
    {
      InheritException(exception,&image->exception);
      return(DestroyImageList(image));
    }
  for (n=0; n < (ssize_t) dpx.image.number_elements; n++)
  {
    /*
      Convert DPX raster image to pixel packets.
    */
    if ((dpx.image.image_element[n].data_offset != ~0U) &&
        (dpx.image.image_element[n].data_offset != 0U))
      {
         MagickOffsetType
           data_offset;

         data_offset=(MagickOffsetType) dpx.image.image_element[n].data_offset;
         if (data_offset < offset)
           offset=SeekBlob(image,data_offset,SEEK_SET);
         else
           for ( ; offset < data_offset; offset++)
             if (ReadBlobByte(image) == EOF)
               break;
          if (offset != data_offset)
            ThrowReaderException(CorruptImageError,"UnableToReadImageData");
       }
    SetPrimaryChromaticity((DPXColorimetric)
      dpx.image.image_element[n].colorimetric,&image->chromaticity);
    image->depth=dpx.image.image_element[n].bit_size;
    if ((image->depth == 0) || (image->depth > 32))
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    samples_per_pixel=1;
    quantum_type=GrayQuantum;
    component_type=dpx.image.image_element[n].descriptor;
    switch (component_type)
    {
      case CbYCrY422ComponentType:
      {
        samples_per_pixel=2;
        quantum_type=CbYCrYQuantum;
        break;
      }
      case CbYACrYA4224ComponentType:
      case CbYCr444ComponentType:
      {
        samples_per_pixel=3;
        quantum_type=CbYCrQuantum;
        break;
      }
      case RGBComponentType:
      {
        samples_per_pixel=3;
        quantum_type=RGBQuantum;
        break;
      }
      case ABGRComponentType:
      case RGBAComponentType:
      {
        image->matte=MagickTrue;
        samples_per_pixel=4;
        quantum_type=RGBAQuantum;
        break;
      }
      default:
        break;
    }
    switch (component_type)
    {
      case CbYCrY422ComponentType:
      case CbYACrYA4224ComponentType:
      case CbYCr444ComponentType:
      {
        (void) SetImageColorspace(image,Rec709YCbCrColorspace);
        break;
      }
      case LumaComponentType:
      {
        (void) SetImageColorspace(image,GRAYColorspace);
        break;
      }
      default:
      {
        (void) SetImageColorspace(image,RGBColorspace);
        if (dpx.image.image_element[n].transfer_characteristic == LogarithmicColorimetric)
          (void) SetImageColorspace(image,LogColorspace);
        if (dpx.image.image_element[n].transfer_characteristic == PrintingDensityColorimetric)
          (void) SetImageColorspace(image,LogColorspace);
        break;
      }
    }
    extent=GetBytesPerRow(image->columns,samples_per_pixel,image->depth,
      dpx.image.image_element[n].packing == 0 ? MagickFalse : MagickTrue);
    /*
      DPX any-bit pixel format.
    */
    row=0;
    quantum_info=AcquireQuantumInfo(image_info,image);
    if (quantum_info == (QuantumInfo *) NULL)
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    SetQuantumQuantum(quantum_info,32);
    SetQuantumPack(quantum_info,dpx.image.image_element[n].packing == 0 ?
      MagickTrue : MagickFalse);
    for (y=0; y < (ssize_t) image->rows; y++)
    {
      const unsigned char
        *pixels;

      MagickBooleanType
        sync;

      register PixelPacket
        *q;

      size_t
        length;

      ssize_t
        count,
        offset;

      pixels=(const unsigned char *) ReadBlobStream(image,extent,
        GetQuantumPixels(quantum_info),&count);
      if (count != (ssize_t) extent)
        break;
      if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
          (image->previous == (Image *) NULL))
        {
          MagickBooleanType
            proceed;

          proceed=SetImageProgress(image,LoadImageTag,(MagickOffsetType) row,
            image->rows);
          if (proceed == MagickFalse)
            break;
        }
      offset=row++;
      q=QueueAuthenticPixels(image,0,offset,image->columns,1,exception);
      if (q == (PixelPacket *) NULL)
        break;
      length=ImportQuantumPixels(image,(CacheView *) NULL,quantum_info,
        quantum_type,pixels,exception);
      (void) length;
      sync=SyncAuthenticPixels(image,exception);
      if (sync == MagickFalse)
        break;
    }
    quantum_info=DestroyQuantumInfo(quantum_info);
    if (y < (ssize_t) image->rows)
      ThrowReaderException(CorruptImageError,"UnableToReadImageData");
    SetQuantumImageType(image,quantum_type);
    if (EOFBlob(image) != MagickFalse)
      ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
        image->filename);
  }
  (void) CloseBlob(image);
  return(GetFirstImageInList(image));
}