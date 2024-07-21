static Image *ReadFLIFImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  FLIF_DECODER
    *flifdec;

  FLIF_IMAGE
    *flifimage;

  Image
    *image;

  MagickBooleanType
    status;

  register PixelPacket
    *q;

  register ssize_t
    x;

  register unsigned short
    *p;

  size_t
    count,
    image_count,
    length;

  ssize_t
    y;

  unsigned char
    *stream;

  unsigned short
    *pixels;

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
  length=(size_t) GetBlobSize(image);
  stream=(unsigned char *) AcquireQuantumMemory(length,sizeof(*stream));
  if (stream == (unsigned char *) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  count=ReadBlob(image,length,stream);
  if (count != length)
    {
      stream=(unsigned char *) RelinquishMagickMemory(stream);
      ThrowReaderException(CorruptImageError,"InsufficientImageDataInFile");
    }
  flifdec=flif_create_decoder();
  if (image_info->quality != UndefinedCompressionQuality)
    flif_decoder_set_quality(flifdec,image_info->quality);
  if (!flif_decoder_decode_memory(flifdec,stream,length))
    {
      flif_destroy_decoder(flifdec);
      ThrowReaderException(CorruptImageError,"CorruptImage");
    }
  image_count=flif_decoder_num_images(flifdec);
  flifimage=flif_decoder_get_image(flifdec,0);
  length=sizeof(unsigned short)*4*flif_image_get_width(flifimage);
  pixels=(unsigned short *) AcquireMagickMemory(length);
  if (pixels == (unsigned short *) NULL)
    {
      flif_destroy_decoder(flifdec);
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    }

  for (count=0; count < image_count; count++)
  {
    if (count > 0)
      {
        /*
          Allocate next image structure.
        */
        AcquireNextImage(image_info,image);
        if (GetNextImageInList(image) == (Image *) NULL)
          {
            image=DestroyImageList(image);
            flif_destroy_decoder(flifdec);
            pixels=(unsigned short *) RelinquishMagickMemory(pixels);
            return((Image *) NULL);
          }
        image=SyncNextImageInList(image);
      }
    flifimage=flif_decoder_get_image(flifdec,count);
    image->columns=(size_t) flif_image_get_width(flifimage);
    image->rows=(size_t) flif_image_get_height(flifimage);
    image->depth=flif_image_get_depth(flifimage);
    image->matte=(flif_image_get_nb_channels(flifimage) > 3 ?
      MagickTrue : MagickFalse);
    image->delay=flif_image_get_frame_delay(flifimage);
    image->ticks_per_second=1000;
    image->scene=count;
    image->dispose=BackgroundDispose;
    for (y=0; y < (ssize_t) image->rows; y++)
    {
      flif_image_read_row_RGBA16(flifimage,y,pixels,length);
      p=pixels;
      q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
      if (q == (PixelPacket *) NULL)
        break;
      for (x=0; x < (ssize_t) image->columns; x++)
      {
        SetPixelRed(q,ScaleShortToQuantum(*p++));
        SetPixelGreen(q,ScaleShortToQuantum(*p++));
        SetPixelBlue(q,ScaleShortToQuantum(*p++));
        SetPixelAlpha(q,ScaleShortToQuantum(*p++));
        q++;
      }
      if (SyncAuthenticPixels(image,exception) == MagickFalse)
        break;
      status=SetImageProgress(image,LoadImageTag,(MagickOffsetType) y,
        image->rows);
      if (status == MagickFalse)
        break;
    }
  }
  flif_destroy_decoder(flifdec);
  pixels=(unsigned short *) RelinquishMagickMemory(pixels);
  return(image);
}