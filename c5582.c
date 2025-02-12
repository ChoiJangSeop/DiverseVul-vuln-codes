static Image *ReadVIPSImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  char
    buffer[MagickPathExtent],
    *metadata;

  Image
    *image;

  MagickBooleanType
    status;

  ssize_t
    n;

  unsigned int
    channels,
    marker;

  VIPSBandFormat
    format;

  VIPSCoding
    coding;

  VIPSType
    type;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);

  image=AcquireImage(image_info,exception);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  marker=ReadBlobLSBLong(image);
  if (marker == VIPS_MAGIC_LSB)
    image->endian=LSBEndian;
  else if (marker == VIPS_MAGIC_MSB)
    image->endian=MSBEndian;
  else
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  image->columns=(size_t) ReadBlobLong(image);
  image->rows=(size_t) ReadBlobLong(image);
  status=SetImageExtent(image,image->columns,image->rows,exception);
  if (status == MagickFalse)
    return(DestroyImageList(image));
  channels=ReadBlobLong(image);
  (void) ReadBlobLong(image); /* Legacy */
  format=(VIPSBandFormat) ReadBlobLong(image);
  switch(format)
  {
    case VIPSBandFormatUCHAR:
    case VIPSBandFormatCHAR:
      image->depth=8;
      break;
    case VIPSBandFormatUSHORT:
    case VIPSBandFormatSHORT:
      image->depth=16;
      break;
    case VIPSBandFormatUINT:
    case VIPSBandFormatINT:
    case VIPSBandFormatFLOAT:
      image->depth=32;
      break;
    case VIPSBandFormatDOUBLE:
      image->depth=64;
      break;
    default:
    case VIPSBandFormatCOMPLEX:
    case VIPSBandFormatDPCOMPLEX:
    case VIPSBandFormatNOTSET:
      ThrowReaderException(CoderError,"Unsupported band format");
  }
  coding=(VIPSCoding) ReadBlobLong(image);
  type=(VIPSType) ReadBlobLong(image);
  switch(type)
  {
    case VIPSTypeCMYK:
      SetImageColorspace(image,CMYKColorspace,exception);
      if (channels == 5)
        image->alpha_trait=BlendPixelTrait;
      break;
    case VIPSTypeB_W:
    case VIPSTypeGREY16:
      SetImageColorspace(image,GRAYColorspace,exception);
      if (channels == 2)
        image->alpha_trait=BlendPixelTrait;
      break;
    case VIPSTypeRGB:
    case VIPSTypeRGB16:
      SetImageColorspace(image,RGBColorspace,exception);
      if (channels == 4)
        image->alpha_trait=BlendPixelTrait;
      break;
    case VIPSTypesRGB:
      SetImageColorspace(image,sRGBColorspace,exception);
      if (channels == 4)
        image->alpha_trait=BlendPixelTrait;
      break;
    default:
    case VIPSTypeFOURIER:
    case VIPSTypeHISTOGRAM:
    case VIPSTypeLAB:
    case VIPSTypeLABS:
    case VIPSTypeLABQ:
    case VIPSTypeLCH:
    case VIPSTypeMULTIBAND:
    case VIPSTypeUCS:
    case VIPSTypeXYZ:
    case VIPSTypeYXY:
      ThrowReaderException(CoderError,"Unsupported colorspace");
  }
  image->units=PixelsPerCentimeterResolution;
  image->resolution.x=ReadBlobFloat(image)*10;
  image->resolution.y=ReadBlobFloat(image)*10;
  /*
    Legacy, offsets, future
  */
  (void) ReadBlobLongLong(image);
  (void) ReadBlobLongLong(image);
  (void) ReadBlobLongLong(image);
  if (image_info->ping != MagickFalse)
    return(image);
  if (IsSupportedCombination(format,type) == MagickFalse)
    ThrowReaderException(CoderError,
      "Unsupported combination of band format and colorspace");
  if (channels == 0 || channels > 5)
    ThrowReaderException(CoderError,"Unsupported number of channels");
  if (coding == VIPSCodingNONE)
    status=ReadVIPSPixelsNONE(image,format,type,channels,exception);
  else
    ThrowReaderException(CoderError,"Unsupported coding");
  metadata=(char *) NULL;
  while ((n=ReadBlob(image,MagickPathExtent-1,(unsigned char *) buffer)) != 0)
  {
    buffer[n]='\0';
    if (metadata == (char *) NULL)
      metadata=ConstantString(buffer);
    else
      (void) ConcatenateString(&metadata,buffer);
  }
  if (metadata != (char *) NULL)
    SetImageProperty(image,"vips:metadata",metadata,exception);
  (void) CloseBlob(image);
  if (status == MagickFalse)
    return((Image *) NULL);
  return(image);
}