static MagickBooleanType WritePSDImage(const ImageInfo *image_info,
  Image *image)
{
  char
    layer_name[MaxTextExtent];

  const char
    *property;

  const StringInfo
    *icc_profile,
    *info;

  Image
    *base_image,
    *next_image;

  MagickBooleanType
    status;

  MagickOffsetType
    *layer_size_offsets,
    size_offset;

  PSDInfo
    psd_info;

  register ssize_t
    i;

  size_t
    layer_count,
    layer_index,
    length,
    name_length,
    num_channels,
    packet_size,
    rounded_size,
    size;

  StringInfo
    *bim_profile;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,&image->exception);
  if (status == MagickFalse)
    return(status);
  packet_size=(size_t) (image->depth > 8 ? 6 : 3);
  if (image->matte != MagickFalse)
    packet_size+=image->depth > 8 ? 2 : 1;
  psd_info.version=1;
  if ((LocaleCompare(image_info->magick,"PSB") == 0) ||
      (image->columns > 30000) || (image->rows > 30000))
    psd_info.version=2;
  (void) WriteBlob(image,4,(const unsigned char *) "8BPS");
  (void) WriteBlobMSBShort(image,psd_info.version);  /* version */
  for (i=1; i <= 6; i++)
    (void) WriteBlobByte(image, 0);  /* 6 bytes of reserved */
  /* When the image has a color profile it won't be converted to gray scale */
  if ((GetImageProfile(image,"icc") == (StringInfo *) NULL) &&
      (SetImageGray(image,&image->exception) != MagickFalse))
    num_channels=(image->matte != MagickFalse ? 2UL : 1UL);
  else
    if ((image_info->type != TrueColorType) && (image_info->type !=
         TrueColorMatteType) && (image->storage_class == PseudoClass))
      num_channels=(image->matte != MagickFalse ? 2UL : 1UL);
    else
      {
        if (image->storage_class == PseudoClass)
          (void) SetImageStorageClass(image,DirectClass);
        if (image->colorspace != CMYKColorspace)
          num_channels=(image->matte != MagickFalse ? 4UL : 3UL);
        else
          num_channels=(image->matte != MagickFalse ? 5UL : 4UL);
      }
  (void) WriteBlobMSBShort(image,(unsigned short) num_channels);
  (void) WriteBlobMSBLong(image,(unsigned int) image->rows);
  (void) WriteBlobMSBLong(image,(unsigned int) image->columns);
  if (IsGrayImage(image,&image->exception) != MagickFalse)
    {
      MagickBooleanType
        monochrome;

      /*
        Write depth & mode.
      */
      monochrome=IsMonochromeImage(image,&image->exception) &&
        (image->depth == 1) ? MagickTrue : MagickFalse;
      (void) WriteBlobMSBShort(image,(unsigned short)
        (monochrome != MagickFalse ? 1 : image->depth > 8 ? 16 : 8));
      (void) WriteBlobMSBShort(image,(unsigned short)
        (monochrome != MagickFalse ? BitmapMode : GrayscaleMode));
    }
  else
    {
      (void) WriteBlobMSBShort(image,(unsigned short) (image->storage_class ==
        PseudoClass ? 8 : image->depth > 8 ? 16 : 8));
      if (((image_info->colorspace != UndefinedColorspace) ||
           (image->colorspace != CMYKColorspace)) &&
          (image_info->colorspace != CMYKColorspace))
        {
          (void) TransformImageColorspace(image,sRGBColorspace);
          (void) WriteBlobMSBShort(image,(unsigned short)
            (image->storage_class == PseudoClass ? IndexedMode : RGBMode));
        }
      else
        {
          if (image->colorspace != CMYKColorspace)
            (void) TransformImageColorspace(image,CMYKColorspace);
          (void) WriteBlobMSBShort(image,CMYKMode);
        }
    }
  if ((IsGrayImage(image,&image->exception) != MagickFalse) ||
      (image->storage_class == DirectClass) || (image->colors > 256))
    (void) WriteBlobMSBLong(image,0);
  else
    {
      /*
        Write PSD raster colormap.
      */
      (void) WriteBlobMSBLong(image,768);
      for (i=0; i < (ssize_t) image->colors; i++)
        (void) WriteBlobByte(image,ScaleQuantumToChar(image->colormap[i].red));
      for ( ; i < 256; i++)
        (void) WriteBlobByte(image,0);
      for (i=0; i < (ssize_t) image->colors; i++)
        (void) WriteBlobByte(image,ScaleQuantumToChar(
          image->colormap[i].green));
      for ( ; i < 256; i++)
        (void) WriteBlobByte(image,0);
      for (i=0; i < (ssize_t) image->colors; i++)
        (void) WriteBlobByte(image,ScaleQuantumToChar(image->colormap[i].blue));
      for ( ; i < 256; i++)
        (void) WriteBlobByte(image,0);
    }
  /*
    Image resource block.
  */
  length=28; /* 0x03EB */
  bim_profile=(StringInfo *) GetImageProfile(image,"8bim");
  icc_profile=GetImageProfile(image,"icc");
  if (bim_profile != (StringInfo *) NULL)
    {
      bim_profile=CloneStringInfo(bim_profile);
      if (icc_profile != (StringInfo *) NULL)
        RemoveICCProfileFromResourceBlock(bim_profile);
      RemoveResolutionFromResourceBlock(bim_profile);
      length+=PSDQuantum(GetStringInfoLength(bim_profile));
    }
  if (icc_profile != (const StringInfo *) NULL)
    length+=PSDQuantum(GetStringInfoLength(icc_profile))+12;
  (void) WriteBlobMSBLong(image,(unsigned int) length);
  WriteResolutionResourceBlock(image);
  if (bim_profile != (StringInfo *) NULL)
    {
      (void) WriteBlob(image,GetStringInfoLength(bim_profile),
        GetStringInfoDatum(bim_profile));
      bim_profile=DestroyStringInfo(bim_profile);
    }
  if (icc_profile != (StringInfo *) NULL)
    {
      (void) WriteBlob(image,4,(const unsigned char *) "8BIM");
      (void) WriteBlobMSBShort(image,0x0000040F);
      (void) WriteBlobMSBShort(image,0);
      (void) WriteBlobMSBLong(image,(unsigned int) GetStringInfoLength(
        icc_profile));
      (void) WriteBlob(image,GetStringInfoLength(icc_profile),
        GetStringInfoDatum(icc_profile));
      if ((MagickOffsetType) GetStringInfoLength(icc_profile) !=
          PSDQuantum(GetStringInfoLength(icc_profile)))
        (void) WriteBlobByte(image,0);
    }
  base_image=GetNextImageInList(image);
  if (base_image == (Image *)NULL)
    base_image=image;
  size=0;
  size_offset=TellBlob(image);
  SetPSDSize(&psd_info,image,0);
  SetPSDSize(&psd_info,image,0);
  layer_count=0;
  for (next_image=base_image; next_image != NULL; )
  {
    layer_count++;
    next_image=GetNextImageInList(next_image);
  }
  if (image->matte != MagickFalse)
    size+=WriteBlobMSBShort(image,-(unsigned short) layer_count);
  else
    size+=WriteBlobMSBShort(image,(unsigned short) layer_count);
  layer_size_offsets=(MagickOffsetType *) AcquireQuantumMemory(
    (size_t) layer_count,sizeof(MagickOffsetType));
  if (layer_size_offsets == (MagickOffsetType *) NULL)
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
  layer_index=0;
  for (next_image=base_image; next_image != NULL; )
  {
    Image
      *mask;

    unsigned char
      default_color;

    unsigned short
      channels,
      total_channels;

    mask=(Image *) NULL;
    property=GetImageArtifact(next_image,"psd:opacity-mask");
    default_color=0;
    if (property != (const char *) NULL)
      {
        mask=(Image *) GetImageRegistry(ImageRegistryType,property,
          &image->exception);
        default_color=strlen(property) == 9 ? 255 : 0;
      }
    size+=WriteBlobMSBLong(image,(unsigned int) next_image->page.y);
    size+=WriteBlobMSBLong(image,(unsigned int) next_image->page.x);
    size+=WriteBlobMSBLong(image,(unsigned int) (next_image->page.y+
      next_image->rows));
    size+=WriteBlobMSBLong(image,(unsigned int) (next_image->page.x+
      next_image->columns));
    channels=1U;
    if ((next_image->storage_class != PseudoClass) &&
        (IsGrayImage(next_image,&next_image->exception) == MagickFalse))
      channels=next_image->colorspace == CMYKColorspace ? 4U : 3U;
    total_channels=channels;
    if (next_image->matte != MagickFalse)
      total_channels++;
    if (mask != (Image *) NULL)
      total_channels++;
    size+=WriteBlobMSBShort(image,total_channels);
    layer_size_offsets[layer_index++]=TellBlob(image);
    for (i=0; i < (ssize_t) channels; i++)
      size+=WriteChannelSize(&psd_info,image,(signed short) i);
    if (next_image->matte != MagickFalse)
      size+=WriteChannelSize(&psd_info,image,-1);
    if (mask != (Image *) NULL)
      size+=WriteChannelSize(&psd_info,image,-2);
    size+=WriteBlob(image,4,(const unsigned char *) "8BIM");
    size+=WriteBlob(image,4,(const unsigned char *)
      CompositeOperatorToPSDBlendMode(next_image->compose));
    property=GetImageArtifact(next_image,"psd:layer.opacity");
    if (property != (const char *) NULL)
      {
        Quantum
          opacity;

        opacity=(Quantum) StringToInteger(property);
        size+=WriteBlobByte(image,ScaleQuantumToChar(opacity));
        (void) ApplyPSDLayerOpacity(next_image,opacity,MagickTrue,
          &image->exception);
      }
    else
      size+=WriteBlobByte(image,255);
    size+=WriteBlobByte(image,0);
    size+=WriteBlobByte(image,next_image->compose==NoCompositeOp ?
      1 << 0x02 : 1); /* layer properties - visible, etc. */
    size+=WriteBlobByte(image,0);
    info=GetAdditionalInformation(image_info,next_image);
    property=(const char *) GetImageProperty(next_image,"label");
    if (property == (const char *) NULL)
      {
        (void) FormatLocaleString(layer_name,MaxTextExtent,"L%.20g",
          (double) layer_index);
        property=layer_name;
      }
    name_length=strlen(property)+1;
    if ((name_length % 4) != 0)
      name_length+=(4-(name_length % 4));
    if (info != (const StringInfo *) NULL)
      name_length+=GetStringInfoLength(info);
    name_length+=8;
    if (mask != (Image *) NULL)
      name_length+=20;
    size+=WriteBlobMSBLong(image,(unsigned int) name_length);
    if (mask == (Image *) NULL)
      size+=WriteBlobMSBLong(image,0);
    else
      {
        if (mask->compose != NoCompositeOp)
          (void) ApplyPSDOpacityMask(next_image,mask,ScaleCharToQuantum(
            default_color),MagickTrue,&image->exception);
        mask->page.y+=image->page.y;
        mask->page.x+=image->page.x;
        size+=WriteBlobMSBLong(image,20);
        size+=WriteBlobMSBSignedLong(image,mask->page.y);
        size+=WriteBlobMSBSignedLong(image,mask->page.x);
        size+=WriteBlobMSBSignedLong(image,(const signed int) mask->rows+
          mask->page.y);
        size+=WriteBlobMSBSignedLong(image,(const signed int) mask->columns+
          mask->page.x);
        size+=WriteBlobByte(image,default_color);
        size+=WriteBlobByte(image,mask->compose == NoCompositeOp ? 2 : 0);
        size+=WriteBlobMSBShort(image,0);
      }
    size+=WriteBlobMSBLong(image,0);
    size+=WritePascalString(image,property,4);
    if (info != (const StringInfo *) NULL)
      size+=WriteBlob(image,GetStringInfoLength(info),
        GetStringInfoDatum(info));
    next_image=GetNextImageInList(next_image);
  }
  /*
    Now the image data!
  */
  next_image=base_image;
  layer_index=0;
  while (next_image != NULL)
  {
    length=WritePSDChannels(&psd_info,image_info,image,next_image,
      layer_size_offsets[layer_index++],MagickTrue);
    if (length == 0)
      {
        status=MagickFalse;
        break;
      }
    size+=length;
    next_image=GetNextImageInList(next_image);
  }
  (void) WriteBlobMSBLong(image,0);  /* user mask data */
  /*
    Remove the opacity mask from the registry
  */
  next_image=base_image;
  while (next_image != (Image *) NULL)
  {
    property=GetImageArtifact(next_image,"psd:opacity-mask");
    if (property != (const char *) NULL)
      DeleteImageRegistry(property);
    next_image=GetNextImageInList(next_image);
  }
  /*
    Write the total size
  */
  size_offset+=WritePSDSize(&psd_info,image,size+
    (psd_info.version == 1 ? 8 : 12),size_offset);
  if ((size/2) != ((size+1)/2))
    rounded_size=size+1;
  else
    rounded_size=size;
  (void) WritePSDSize(&psd_info,image,rounded_size,size_offset);
  layer_size_offsets=(MagickOffsetType *) RelinquishMagickMemory(
    layer_size_offsets);
  /*
    Write composite image.
  */
  if (status != MagickFalse)
    {
      CompressionType
        compression;

      compression=image->compression;
      if (image->compression == ZipCompression)
        image->compression=RLECompression;
      if (WritePSDChannels(&psd_info,image_info,image,image,0,
          MagickFalse) == 0)
        status=MagickFalse;
      image->compression=compression;
    }
  (void) CloseBlob(image);
  return(status);
}