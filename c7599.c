static Image *ReadRAWImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  const unsigned char
    *pixels;

  Image
    *canvas_image,
    *image;

  MagickBooleanType
    status;

  MagickOffsetType
    scene;

  QuantumInfo
    *quantum_info;

  QuantumType
    quantum_type;

  size_t
    length;

  ssize_t
    count,
    y;

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
  if ((image->columns == 0) || (image->rows == 0))
    ThrowReaderException(OptionError,"MustSpecifyImageSize");
  status=SetImageExtent(image,image->columns,image->rows);
  if (status == MagickFalse)
    {
      InheritException(exception,&image->exception);
      return(DestroyImageList(image));
    }
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  if (DiscardBlobBytes(image,(MagickSizeType) image->offset) == MagickFalse)
    ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
      image->filename);
  /*
    Create virtual canvas to support cropping (i.e. image.gray[100x100+10+20]).
  */
  canvas_image=CloneImage(image,image->extract_info.width,1,MagickFalse,
    exception);
  if (canvas_image == (Image *) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  (void) SetImageVirtualPixelMethod(canvas_image,BlackVirtualPixelMethod);
  quantum_type=GrayQuantum;
  quantum_info=AcquireQuantumInfo(image_info,canvas_image);
  if (quantum_info == (QuantumInfo *) NULL)
    {
      canvas_image=DestroyImage(canvas_image);
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    }
  pixels=(const unsigned char *) NULL;
  if (image_info->number_scenes != 0)
    while (image->scene < image_info->scene)
    {
      /*
        Skip to next image.
      */
      image->scene++;
      length=GetQuantumExtent(canvas_image,quantum_info,quantum_type);
      for (y=0; y < (ssize_t) image->rows; y++)
      {
        pixels=(const unsigned char *) ReadBlobStream(image,length,
          GetQuantumPixels(quantum_info),&count);
        if (count != (ssize_t) length)
          break;
      }
    }
  scene=0;
  count=0;
  length=0;
  status=MagickTrue;
  do
  {
    /*
      Read pixels to virtual canvas image then push to image.
    */
    if ((image_info->ping != MagickFalse) && (image_info->number_scenes != 0))
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    status=SetImageExtent(image,image->columns,image->rows);
    if (status == MagickFalse)
      break;
    if (scene == 0)
      {
        length=GetQuantumExtent(canvas_image,quantum_info,quantum_type);
        pixels=(const unsigned char *) ReadBlobStream(image,length,
          GetQuantumPixels(quantum_info),&count);
        if (count != (ssize_t) length)
          break;
      }
    for (y=0; y < (ssize_t) image->extract_info.height; y++)
    {
      register const PixelPacket
        *magick_restrict p;

      register PixelPacket
        *magick_restrict q;

      register ssize_t
        x;

      if (count != (ssize_t) length)
        {
          status=MagickFalse;
          ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
            image->filename);
          break;
        }
      q=GetAuthenticPixels(canvas_image,0,0,canvas_image->columns,1,exception);
      if (q == (PixelPacket *) NULL)
        break;
      length=ImportQuantumPixels(canvas_image,(CacheView *) NULL,quantum_info,
        quantum_type,pixels,exception);
      if (SyncAuthenticPixels(canvas_image,exception) == MagickFalse)
        break;
      if (((y-image->extract_info.y) >= 0) &&
          ((y-image->extract_info.y) < (ssize_t) image->rows))
        {
          p=GetVirtualPixels(canvas_image,canvas_image->extract_info.x,0,
            image->columns,1,exception);
          q=QueueAuthenticPixels(image,0,y-image->extract_info.y,image->columns,
            1,exception);
          if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
            break;
          for (x=0; x < (ssize_t) image->columns; x++)
          {
            SetPixelRed(q,GetPixelRed(p));
            SetPixelGreen(q,GetPixelGreen(p));
            SetPixelBlue(q,GetPixelBlue(p));
            p++;
            q++;
          }
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
      pixels=(const unsigned char *) ReadBlobStream(image,length,
        GetQuantumPixels(quantum_info),&count);
      if (count != (ssize_t) length)
        break;
    }
    SetQuantumImageType(image,quantum_type);
    /*
      Proceed to next image.
    */
    if (image_info->number_scenes != 0)
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    if (count == (ssize_t) length)
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
        status=SetImageProgress(image,LoadImagesTag,TellBlob(image),
          GetBlobSize(image));
        if (status == MagickFalse)
          break;
      }
    scene++;
  } while (count == (ssize_t) length);
  quantum_info=DestroyQuantumInfo(quantum_info);
  InheritException(exception,&canvas_image->exception);
  InheritException(exception,&image->exception);
  canvas_image=DestroyImage(canvas_image);
  (void) CloseBlob(image);
  if (status == MagickFalse)
    return(DestroyImageList(image));
  return(GetFirstImageInList(image));
}