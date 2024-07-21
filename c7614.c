static Image *ReadYUVImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  Image
    *chroma_image,
    *image,
    *resize_image;

  InterlaceType
    interlace;

  MagickBooleanType
    status;

  register const PixelPacket
    *chroma_pixels;

  register ssize_t
    x;

  register PixelPacket
    *q;

  register unsigned char
    *p;

  ssize_t
    count,
    horizontal_factor,
    vertical_factor,
    y;

  size_t
    length,
    quantum;

  unsigned char
    *scanline;

  /*
    Allocate image structure.
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
  quantum=(size_t) (image->depth <= 8 ? 1 : 2);
  interlace=image_info->interlace;
  horizontal_factor=2;
  vertical_factor=2;
  if (image_info->sampling_factor != (char *) NULL)
    {
      GeometryInfo
        geometry_info;

      MagickStatusType
        flags;

      flags=ParseGeometry(image_info->sampling_factor,&geometry_info);
      horizontal_factor=(ssize_t) geometry_info.rho;
      vertical_factor=(ssize_t) geometry_info.sigma;
      if ((flags & SigmaValue) == 0)
        vertical_factor=horizontal_factor;
      if ((horizontal_factor != 1) && (horizontal_factor != 2) &&
          (vertical_factor != 1) && (vertical_factor != 2))
        ThrowReaderException(CorruptImageError,"UnexpectedSamplingFactor");
    }
  if ((interlace == UndefinedInterlace) ||
      ((interlace == NoInterlace) && (vertical_factor == 2)))
    {
      interlace=NoInterlace;    /* CCIR 4:2:2 */
      if (vertical_factor == 2)
        interlace=PlaneInterlace; /* CCIR 4:1:1 */
    }
  if (interlace != PartitionInterlace)
    {
      /*
        Open image file.
      */
      status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
      if (status == MagickFalse)
        {
          image=DestroyImageList(image);
          return((Image *) NULL);
        }
      if (DiscardBlobBytes(image,(MagickSizeType) image->offset) == MagickFalse)
        ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
          image->filename);
    }
  /*
    Allocate memory for a scanline.
  */
  if (interlace == NoInterlace)
    scanline=(unsigned char *) AcquireQuantumMemory((size_t) 2UL*
      image->columns+2UL,quantum*sizeof(*scanline));
  else
    scanline=(unsigned char *) AcquireQuantumMemory(image->columns,
      quantum*sizeof(*scanline));
  if (scanline == (unsigned char *) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  status=MagickTrue;
  do
  {
    chroma_image=CloneImage(image,(image->columns+horizontal_factor-1)/
      horizontal_factor,(image->rows+vertical_factor-1)/vertical_factor,
      MagickTrue,exception);
    if (chroma_image == (Image *) NULL)
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    /*
      Convert raster image to pixel packets.
    */
    if ((image_info->ping != MagickFalse) && (image_info->number_scenes != 0))
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    status=SetImageExtent(image,image->columns,image->rows);
    if (status == MagickFalse)
      break;
    if (interlace == PartitionInterlace)
      {
        AppendImageFormat("Y",image->filename);
        status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
        if (status == MagickFalse)
          {
            image=DestroyImageList(image);
            return((Image *) NULL);
          }
      }
    for (y=0; y < (ssize_t) image->rows; y++)
    {
      register PixelPacket
        *chroma_pixels;

      if (interlace == NoInterlace)
        {
          if ((y > 0) || (GetPreviousImageInList(image) == (Image *) NULL))
            {
              length=2*quantum*image->columns;
              count=ReadBlob(image,length,scanline);
              if (count != (ssize_t) length)
                {
                  status=MagickFalse;
                  ThrowFileException(exception,CorruptImageError,
                    "UnexpectedEndOfFile",image->filename);
                  break;
                }
            }
          p=scanline;
          q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
          if (q == (PixelPacket *) NULL)
            break;
          chroma_pixels=QueueAuthenticPixels(chroma_image,0,y,
            chroma_image->columns,1,exception);
          if (chroma_pixels == (PixelPacket *) NULL)
            break;
          for (x=0; x < (ssize_t) image->columns; x+=2)
          {
            SetPixelRed(chroma_pixels,0);
            if (quantum == 1)
              SetPixelGreen(chroma_pixels,ScaleCharToQuantum(*p++));
            else
              {
                SetPixelGreen(chroma_pixels,ScaleShortToQuantum(((*p) << 8) |
                  *(p+1)));
                p+=2;
              }
            if (quantum == 1)
              SetPixelRed(q,ScaleCharToQuantum(*p++));
            else
              {
                SetPixelRed(q,ScaleShortToQuantum(((*p) << 8) | *(p+1)));
                p+=2;
              }
            SetPixelGreen(q,0);
            SetPixelBlue(q,0);
            q++;
            SetPixelGreen(q,0);
            SetPixelBlue(q,0);
            if (quantum == 1)
              SetPixelBlue(chroma_pixels,ScaleCharToQuantum(*p++));
            else
              {
                SetPixelBlue(chroma_pixels,ScaleShortToQuantum(((*p) << 8) |
                  *(p+1)));
                p+=2;
              }
            if (quantum == 1)
              SetPixelRed(q,ScaleCharToQuantum(*p++));
            else
              {
                SetPixelRed(q,ScaleShortToQuantum(((*p) << 8) | *(p+1)));
                p+=2;
              }
            chroma_pixels++;
            q++;
          }
        }
      else
        {
          if ((y > 0) || (GetPreviousImageInList(image) == (Image *) NULL))
            {
              length=quantum*image->columns;
              count=ReadBlob(image,length,scanline);
              if (count != (ssize_t) length)
                {
                  status=MagickFalse;
                  ThrowFileException(exception,CorruptImageError,
                    "UnexpectedEndOfFile",image->filename);
                  break;
                }
            }
          p=scanline;
          q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
          if (q == (PixelPacket *) NULL)
            break;
          for (x=0; x < (ssize_t) image->columns; x++)
          {
            if (quantum == 1)
              SetPixelRed(q,ScaleCharToQuantum(*p++));
            else
              {
                SetPixelRed(q,ScaleShortToQuantum(((*p) << 8) | *(p+1)));
                p+=2;
              }
            SetPixelGreen(q,0);
            SetPixelBlue(q,0);
            q++;
          }
        }
      if (SyncAuthenticPixels(image,exception) == MagickFalse)
        break;
      if (interlace == NoInterlace)
        if (SyncAuthenticPixels(chroma_image,exception) == MagickFalse)
          break;
      if (image->previous == (Image *) NULL)
        {
          status=SetImageProgress(image,LoadImageTag,(MagickOffsetType) y,
            image->rows);
          if (status == MagickFalse)
            break;
        }
    }
    if (interlace == PartitionInterlace)
      {
        (void) CloseBlob(image);
        AppendImageFormat("U",image->filename);
        status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
        if (status == MagickFalse)
          {
            image=DestroyImageList(image);
            return((Image *) NULL);
          }
      }
    if (interlace != NoInterlace)
      {
        for (y=0; y < (ssize_t) chroma_image->rows; y++)
        {
          length=quantum*chroma_image->columns;
          count=ReadBlob(image,length,scanline);
          if (count != (ssize_t) length)
            {
              status=MagickFalse;
              ThrowFileException(exception,CorruptImageError,
                "UnexpectedEndOfFile",image->filename);
              break;
            }
          p=scanline;
          q=QueueAuthenticPixels(chroma_image,0,y,chroma_image->columns,1,
            exception);
          if (q == (PixelPacket *) NULL)
            break;
          for (x=0; x < (ssize_t) chroma_image->columns; x++)
          {
            SetPixelRed(q,0);
            if (quantum == 1)
              SetPixelGreen(q,ScaleCharToQuantum(*p++));
            else
              {
                SetPixelGreen(q,ScaleShortToQuantum(((*p) << 8) | *(p+1)));
                p+=2;
              }
            SetPixelBlue(q,0);
            q++;
          }
          if (SyncAuthenticPixels(chroma_image,exception) == MagickFalse)
            break;
        }
      if (interlace == PartitionInterlace)
        {
          (void) CloseBlob(image);
          AppendImageFormat("V",image->filename);
          status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
          if (status == MagickFalse)
            {
              image=DestroyImageList(image);
              return((Image *) NULL);
            }
        }
      for (y=0; y < (ssize_t) chroma_image->rows; y++)
      {
        length=quantum*chroma_image->columns;
        count=ReadBlob(image,length,scanline);
        if (count != (ssize_t) length)
          {
            status=MagickFalse;
            ThrowFileException(exception,CorruptImageError,
              "UnexpectedEndOfFile",image->filename);
            break;
          }
        p=scanline;
        q=GetAuthenticPixels(chroma_image,0,y,chroma_image->columns,1,
          exception);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=0; x < (ssize_t) chroma_image->columns; x++)
        {
          if (quantum == 1)
            SetPixelBlue(q,ScaleCharToQuantum(*p++));
          else
            {
              SetPixelBlue(q,ScaleShortToQuantum(((*p) << 8) | *(p+1)));
              p+=2;
            }
          q++;
        }
        if (SyncAuthenticPixels(chroma_image,exception) == MagickFalse)
          break;
      }
    }
    /*
      Scale image.
    */
    resize_image=ResizeImage(chroma_image,image->columns,image->rows,
      TriangleFilter,1.0,exception);
    chroma_image=DestroyImage(chroma_image);
    if (resize_image == (Image *) NULL)
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    for (y=0; y < (ssize_t) image->rows; y++)
    {
      q=GetAuthenticPixels(image,0,y,image->columns,1,exception);
      chroma_pixels=GetVirtualPixels(resize_image,0,y,resize_image->columns,1,
        &resize_image->exception);
      if ((q == (PixelPacket *) NULL) ||
          (chroma_pixels == (const PixelPacket *) NULL))
        break;
      for (x=0; x < (ssize_t) image->columns; x++)
      {
        SetPixelGreen(q,GetPixelGreen(chroma_pixels));
        SetPixelBlue(q,GetPixelBlue(chroma_pixels));
        chroma_pixels++;
        q++;
      }
      if (SyncAuthenticPixels(image,exception) == MagickFalse)
        break;
    }
    resize_image=DestroyImage(resize_image);
    if (SetImageColorspace(image,YCbCrColorspace) == MagickFalse)
      break;
    if (interlace == PartitionInterlace)
      (void) CopyMagickString(image->filename,image_info->filename,
        MaxTextExtent);
    if (EOFBlob(image) != MagickFalse)
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
    if (interlace == NoInterlace)
      count=ReadBlob(image,(size_t) (2*quantum*image->columns),scanline);
    else
      count=ReadBlob(image,(size_t) quantum*image->columns,scanline);
    if (count != 0)
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
  } while (count != 0);
  scanline=(unsigned char *) RelinquishMagickMemory(scanline);
  InheritException(exception,&image->exception);
  (void) CloseBlob(image);
  if (status == MagickFalse)
    return(DestroyImageList(image));
  return(GetFirstImageInList(image));
}