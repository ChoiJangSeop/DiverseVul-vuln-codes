static Image *ReadTIM2Image(const ImageInfo *image_info,ExceptionInfo *exception)
{
  Image
    *image;

  MagickBooleanType
    status;

  ssize_t
    str_read;

  TIM2FileHeader
    file_header;

  /*
   * Open image file.
   */
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
  /*
   * Verify TIM2 magic number.
   */
  file_header.magic_num=ReadBlobMSBLong(image);
  if (file_header.magic_num != 0x54494D32) /* "TIM2" */
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  /*
   * #### Read File Header ####
   */
  file_header.format_vers=ReadBlobByte(image);
  if (file_header.format_vers != 0x04)
    ThrowReaderException(CoderError,"ImageTypeNotSupported");
  file_header.format_type=ReadBlobByte(image);
  file_header.image_count=ReadBlobLSBShort(image);
  ReadBlobStream(image,8,&(file_header.reserved),&str_read);
  /*
   * Jump to first image header
   */
  switch(file_header.format_type)
  {
    case 0x00:
      if (DiscardBlobBytes(image,16) == MagickFalse)
        ThrowReaderException(CorruptImageError,"InsufficientImageDataInFile");
      break;
    case 0x01:
      if (DiscardBlobBytes(image,128) == MagickFalse)
        ThrowReaderException(CorruptImageError,"InsufficientImageDataInFile");
      break;
    default:
      ThrowReaderException(CoderError,"ImageTypeNotSupported");
  }
  /*
   * Process each image. Only one image supported for now
   */
  if (file_header.image_count != 1)
    ThrowReaderException(CoderError,"NumberOfImagesIsNotSupported");
  for (int i=0; i < file_header.image_count; ++i)
  {
    char
      clut_depth,
      bits_per_pixel;

    TIM2ImageHeader
      image_header;

    ReadTIM2ImageHeader(image,&image_header);
    if (image_header.mipmap_count != 1)
      ThrowReaderException(CoderError,"NumberOfImagesIsNotSupported");
    if (image_header.header_size < 48)
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    if ((MagickSizeType) image_header.image_size > GetBlobSize(image))
      ThrowReaderException(CorruptImageError,"InsufficientImageDataInFile");
    if ((MagickSizeType) image_header.clut_size > GetBlobSize(image))
      ThrowReaderException(CorruptImageError,"InsufficientImageDataInFile");
    image->columns=image_header.width;
    image->rows=image_header.height;
    clut_depth=0;
    if (image_header.clut_type !=0)
      {
        switch((int) image_header.clut_type&0x0F)  /* Low 4 bits */
        {
          case 1:
            clut_depth=16;
            break;
          case 2:
            clut_depth=24;
            break;
          case 3:
            clut_depth=32;
            break;
          default:
            ThrowReaderException(CorruptImageError,"ImproperImageHeader");
            break;
        }
      }
    switch ((int) image_header.bpp_type)
    {
      case 1:
        bits_per_pixel=16;
        break;
      case 2:
        bits_per_pixel=24;
        break;
      case 3:
        bits_per_pixel=32;
        break;
      case 4:
        bits_per_pixel=4;  /* Implies CLUT */
        break;
      case 5:
        bits_per_pixel=8;  /* Implies CLUT */
        break;
      default:
        ThrowReaderException(CorruptImageError,"ImproperImageHeader");
        break;
    }
    image->depth=(clut_depth != 0) ? clut_depth : bits_per_pixel;
    if ((image->depth == 16) || (image->depth == 32))
      image->alpha_trait=BlendPixelTrait;
    if (image->ping != MagickFalse)
      {
        status=ReadTIM2ImageData(image_info,image,&image_header,clut_depth,
          bits_per_pixel,exception);
        if (status==MagickFalse)
          break;
      }
    if ((image_info->ping != MagickFalse) && (image_info->number_scenes != 0))
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    if ((image->storage_class == PseudoClass) && (EOFBlob(image) != MagickFalse))
      {
        ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
          image->filename);
        break;
      }
    /*
      Proceed to next image.
    */
    if (image_info->number_scenes != 0)
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    /*
      Allocate next image structure.
    */
    AcquireNextImage(image_info,image,exception);
    if (GetNextImageInList(image) == (Image *) NULL)
      {
        status=MagickFalse;
        break;
      }
    image=SyncNextImageInList(image);
    status=SetImageProgress(image,LoadImagesTag,image->scene-1,
      image->scene);
    if (status == MagickFalse)
      break;
  }
  (void) CloseBlob(image);
  if (status == MagickFalse)
    return(DestroyImageList(image));
  return(GetFirstImageInList(image));
}