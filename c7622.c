static Image *ReadSGIImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  Image
    *image;

  MagickBooleanType
    status;

  MagickSizeType
    number_pixels;

  MemoryInfo
    *pixel_info;

  register IndexPacket
    *indexes;

  register PixelPacket
    *q;

  register ssize_t
    i,
    x;

  register unsigned char
    *p;

  SGIInfo
    iris_info;

  size_t
    bytes_per_pixel,
    quantum;

  ssize_t
    count,
    y,
    z;

  unsigned char
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
  /*
    Read SGI raster header.
  */
  (void) memset(&iris_info,0,sizeof(iris_info));
  iris_info.magic=ReadBlobMSBShort(image);
  do
  {
    /*
      Verify SGI identifier.
    */
    if (iris_info.magic != 0x01DA)
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    iris_info.storage=(unsigned char) ReadBlobByte(image);
    switch (iris_info.storage)
    {
      case 0x00: image->compression=NoCompression; break;
      case 0x01: image->compression=RLECompression; break;
      default:
        ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    }
    iris_info.bytes_per_pixel=(unsigned char) ReadBlobByte(image);
    if ((iris_info.bytes_per_pixel == 0) || (iris_info.bytes_per_pixel > 2))
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    iris_info.dimension=ReadBlobMSBShort(image);
    if ((iris_info.dimension == 0) || (iris_info.dimension > 3))
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    iris_info.columns=ReadBlobMSBShort(image);
    iris_info.rows=ReadBlobMSBShort(image);
    iris_info.depth=ReadBlobMSBShort(image);
    if ((iris_info.depth == 0) || (iris_info.depth > 4))
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    iris_info.minimum_value=ReadBlobMSBLong(image);
    iris_info.maximum_value=ReadBlobMSBLong(image);
    iris_info.sans=ReadBlobMSBLong(image);
    count=ReadBlob(image,sizeof(iris_info.name),(unsigned char *)
      iris_info.name);
    if ((size_t) count != sizeof(iris_info.name))
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    iris_info.name[sizeof(iris_info.name)-1]='\0';
    if (*iris_info.name != '\0')
      (void) SetImageProperty(image,"label",iris_info.name);
    iris_info.pixel_format=ReadBlobMSBLong(image);
    if (iris_info.pixel_format != 0)
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    count=ReadBlob(image,sizeof(iris_info.filler),iris_info.filler);
    if ((size_t) count != sizeof(iris_info.filler))
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    image->columns=iris_info.columns;
    image->rows=iris_info.rows;
    image->matte=iris_info.depth == 4 ? MagickTrue : MagickFalse;
    image->depth=(size_t) MagickMin(iris_info.depth,MAGICKCORE_QUANTUM_DEPTH);
    if (iris_info.pixel_format == 0)
      image->depth=(size_t) MagickMin((size_t) 8*iris_info.bytes_per_pixel,
        MAGICKCORE_QUANTUM_DEPTH);
    if (iris_info.depth < 3)
      {
        image->storage_class=PseudoClass;
        image->colors=(size_t) (iris_info.bytes_per_pixel > 1 ? 65535 : 256);
      }
    if ((image_info->ping != MagickFalse) && (image_info->number_scenes != 0))
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    status=SetImageExtent(image,image->columns,image->rows);
    if (status != MagickFalse)
      status=ResetImagePixels(image,&image->exception);
    if (status == MagickFalse)
      {
        InheritException(exception,&image->exception);
        return(DestroyImageList(image));
      }
    /*
      Allocate SGI pixels.
    */
    bytes_per_pixel=(size_t) iris_info.bytes_per_pixel;
    number_pixels=(MagickSizeType) iris_info.columns*iris_info.rows;
    if ((4*bytes_per_pixel*number_pixels) != ((MagickSizeType) (size_t)
        (4*bytes_per_pixel*number_pixels)))
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    pixel_info=AcquireVirtualMemory(iris_info.columns,iris_info.rows*4*
      bytes_per_pixel*sizeof(*pixels));
    if (pixel_info == (MemoryInfo *) NULL)
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    pixels=(unsigned char *) GetVirtualMemoryBlob(pixel_info);
    (void) memset(pixels,0,iris_info.columns*iris_info.rows*4*
      bytes_per_pixel*sizeof(*pixels));
    if ((int) iris_info.storage != 0x01)
      {
        unsigned char
          *scanline;

        /*
          Read standard image format.
        */
        scanline=(unsigned char *) AcquireQuantumMemory(iris_info.columns,
          bytes_per_pixel*sizeof(*scanline));
        if (scanline == (unsigned char *) NULL)
          {
            pixel_info=RelinquishVirtualMemory(pixel_info);
            ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
          }
        for (z=0; z < (ssize_t) iris_info.depth; z++)
        {
          p=pixels+bytes_per_pixel*z;
          for (y=0; y < (ssize_t) iris_info.rows; y++)
          {
            count=ReadBlob(image,bytes_per_pixel*iris_info.columns,scanline);
            if (count != (bytes_per_pixel*iris_info.columns))
              break;
            if (bytes_per_pixel == 2)
              for (x=0; x < (ssize_t) iris_info.columns; x++)
              {
                *p=scanline[2*x];
                *(p+1)=scanline[2*x+1];
                p+=8;
              }
            else
              for (x=0; x < (ssize_t) iris_info.columns; x++)
              {
                *p=scanline[x];
                p+=4;
              }
          }
          if (y < (ssize_t) iris_info.rows)
            break;
        }
        scanline=(unsigned char *) RelinquishMagickMemory(scanline);
      }
    else
      {
        MemoryInfo
          *packet_info;

        size_t
          *runlength;

        ssize_t
          offset,
          *offsets;

        unsigned char
          *packets;

        unsigned int
          data_order;

        /*
          Read runlength-encoded image format.
        */
        offsets=(ssize_t *) AcquireQuantumMemory((size_t) iris_info.rows,
          iris_info.depth*sizeof(*offsets));
        runlength=(size_t *) AcquireQuantumMemory(iris_info.rows,
          iris_info.depth*sizeof(*runlength));
        packet_info=AcquireVirtualMemory((size_t) iris_info.columns+10UL,4UL*
          sizeof(*packets));
        if ((offsets == (ssize_t *) NULL) ||
            (runlength == (size_t *) NULL) ||
            (packet_info == (MemoryInfo *) NULL))
          {
            offsets=(ssize_t *) RelinquishMagickMemory(offsets);
            runlength=(size_t *) RelinquishMagickMemory(runlength);
            if (packet_info != (MemoryInfo *) NULL)
              packet_info=RelinquishVirtualMemory(packet_info);
            pixel_info=RelinquishVirtualMemory(pixel_info);
            ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
          }
        packets=(unsigned char *) GetVirtualMemoryBlob(packet_info);
        for (i=0; i < (ssize_t) (iris_info.rows*iris_info.depth); i++)
          offsets[i]=(ssize_t) ReadBlobMSBSignedLong(image);
        for (i=0; i < (ssize_t) (iris_info.rows*iris_info.depth); i++)
        {
          runlength[i]=ReadBlobMSBLong(image);
          if (runlength[i] > (4*(size_t) iris_info.columns+10))
            {
              offsets=(ssize_t *) RelinquishMagickMemory(offsets);
              runlength=(size_t *) RelinquishMagickMemory(runlength);
              packet_info=RelinquishVirtualMemory(packet_info);
              pixel_info=RelinquishVirtualMemory(pixel_info);
              ThrowReaderException(CorruptImageError,"ImproperImageHeader");
            }
        }
        /*
          Check data order.
        */
        offset=0;
        data_order=0;
        for (y=0; ((y < (ssize_t) iris_info.rows) && (data_order == 0)); y++)
          for (z=0; ((z < (ssize_t) iris_info.depth) && (data_order == 0)); z++)
          {
            if (offsets[y+z*iris_info.rows] < offset)
              data_order=1;
            offset=offsets[y+z*iris_info.rows];
          }
        offset=(ssize_t) TellBlob(image);
        if (data_order == 1)
          {
            for (z=0; z < (ssize_t) iris_info.depth; z++)
            {
              p=pixels;
              for (y=0; y < (ssize_t) iris_info.rows; y++)
              {
                if (offset != offsets[y+z*iris_info.rows])
                  {
                    offset=offsets[y+z*iris_info.rows];
                    offset=(ssize_t) SeekBlob(image,(MagickOffsetType) offset,
                      SEEK_SET);
                  }
                (void) ReadBlob(image,(size_t) runlength[y+z*iris_info.rows],
                  packets);
                if (count != runlength[y+z*iris_info.rows])
                  break;
                offset+=(ssize_t) runlength[y+z*iris_info.rows];
                status=SGIDecode(bytes_per_pixel,(ssize_t)
                  (runlength[y+z*iris_info.rows]/bytes_per_pixel),packets,
                  (ssize_t) iris_info.columns,p+bytes_per_pixel*z);
                if (status == MagickFalse)
                  {
                    offsets=(ssize_t *) RelinquishMagickMemory(offsets);
                    runlength=(size_t *) RelinquishMagickMemory(runlength);
                    packet_info=RelinquishVirtualMemory(packet_info);
                    pixel_info=RelinquishVirtualMemory(pixel_info);
                    ThrowReaderException(CorruptImageError,
                      "ImproperImageHeader");
                  }
                p+=(iris_info.columns*4*bytes_per_pixel);
              }
              if (y < (ssize_t) iris_info.rows)
                break;
            }
          }
        else
          {
            MagickOffsetType
              position;

            position=TellBlob(image);
            p=pixels;
            for (y=0; y < (ssize_t) iris_info.rows; y++)
            {
              for (z=0; z < (ssize_t) iris_info.depth; z++)
              {
                if (offset != offsets[y+z*iris_info.rows])
                  {
                    offset=offsets[y+z*iris_info.rows];
                    offset=(ssize_t) SeekBlob(image,(MagickOffsetType) offset,
                      SEEK_SET);
                  }
                count=ReadBlob(image,(size_t) runlength[y+z*iris_info.rows],
                  packets);
                if (count != runlength[y+z*iris_info.rows])
                  break;
                offset+=(ssize_t) runlength[y+z*iris_info.rows];
                status=SGIDecode(bytes_per_pixel,(ssize_t)
                  (runlength[y+z*iris_info.rows]/bytes_per_pixel),packets,
                  (ssize_t) iris_info.columns,p+bytes_per_pixel*z);
                if (status == MagickFalse)
                  {
                    offsets=(ssize_t *) RelinquishMagickMemory(offsets);
                    runlength=(size_t *) RelinquishMagickMemory(runlength);
                    packet_info=RelinquishVirtualMemory(packet_info);
                    pixel_info=RelinquishVirtualMemory(pixel_info);
                    ThrowReaderException(CorruptImageError,
                      "ImproperImageHeader");
                  }
              }
              if (z < (ssize_t) iris_info.depth)
                break;
              p+=(iris_info.columns*4*bytes_per_pixel);
            }
            offset=(ssize_t) SeekBlob(image,position,SEEK_SET);
          }
        packet_info=RelinquishVirtualMemory(packet_info);
        runlength=(size_t *) RelinquishMagickMemory(runlength);
        offsets=(ssize_t *) RelinquishMagickMemory(offsets);
      }
    /*
      Convert SGI raster image to pixel packets.
    */
    if (image->storage_class == DirectClass)
      {
        /*
          Convert SGI image to DirectClass pixel packets.
        */
        if (bytes_per_pixel == 2)
          {
            for (y=0; y < (ssize_t) image->rows; y++)
            {
              p=pixels+(image->rows-y-1)*8*image->columns;
              q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
              if (q == (PixelPacket *) NULL)
                break;
              for (x=0; x < (ssize_t) image->columns; x++)
              {
                SetPixelRed(q,ScaleShortToQuantum((unsigned short)
                  ((*(p+0) << 8) | (*(p+1)))));
                SetPixelGreen(q,ScaleShortToQuantum((unsigned short)
                  ((*(p+2) << 8) | (*(p+3)))));
                SetPixelBlue(q,ScaleShortToQuantum((unsigned short)
                  ((*(p+4) << 8) | (*(p+5)))));
                SetPixelOpacity(q,OpaqueOpacity);
                if (image->matte != MagickFalse)
                  SetPixelAlpha(q,ScaleShortToQuantum((unsigned short)
                    ((*(p+6) << 8) | (*(p+7)))));
                p+=8;
                q++;
              }
              if (SyncAuthenticPixels(image,exception) == MagickFalse)
                break;
              if (image->previous == (Image *) NULL)
                {
                  status=SetImageProgress(image,LoadImageTag,(MagickOffsetType)
                    y,image->rows);
                  if (status == MagickFalse)
                    break;
                }
            }
          }
        else
          for (y=0; y < (ssize_t) image->rows; y++)
          {
            p=pixels+(image->rows-y-1)*4*image->columns;
            q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (ssize_t) image->columns; x++)
            {
              SetPixelRed(q,ScaleCharToQuantum(*p));
              q->green=ScaleCharToQuantum(*(p+1));
              q->blue=ScaleCharToQuantum(*(p+2));
              SetPixelOpacity(q,OpaqueOpacity);
              if (image->matte != MagickFalse)
                SetPixelAlpha(q,ScaleCharToQuantum(*(p+3)));
              p+=4;
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
      }
    else
      {
        /*
          Create grayscale map.
        */
        if (AcquireImageColormap(image,image->colors) == MagickFalse)
          {
            pixel_info=RelinquishVirtualMemory(pixel_info);
            ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
          }
        /*
          Convert SGI image to PseudoClass pixel packets.
        */
        if (bytes_per_pixel == 2)
          {
            for (y=0; y < (ssize_t) image->rows; y++)
            {
              p=pixels+(image->rows-y-1)*8*image->columns;
              q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
              if (q == (PixelPacket *) NULL)
                break;
              indexes=GetAuthenticIndexQueue(image);
              for (x=0; x < (ssize_t) image->columns; x++)
              {
                quantum=(*p << 8);
                quantum|=(*(p+1));
                SetPixelIndex(indexes+x,quantum);
                p+=8;
                q++;
              }
              if (SyncAuthenticPixels(image,exception) == MagickFalse)
                break;
              if (image->previous == (Image *) NULL)
                {
                  status=SetImageProgress(image,LoadImageTag,(MagickOffsetType)
                    y,image->rows);
                  if (status == MagickFalse)
                    break;
                }
            }
          }
        else
          for (y=0; y < (ssize_t) image->rows; y++)
          {
            p=pixels+(image->rows-y-1)*4*image->columns;
            q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
            if (q == (PixelPacket *) NULL)
              break;
            indexes=GetAuthenticIndexQueue(image);
            for (x=0; x < (ssize_t) image->columns; x++)
            {
              SetPixelIndex(indexes+x,*p);
              p+=4;
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
        (void) SyncImage(image);
      }
    pixel_info=RelinquishVirtualMemory(pixel_info);
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
    iris_info.magic=ReadBlobMSBShort(image);
    if (iris_info.magic == 0x01DA)
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
  } while (iris_info.magic == 0x01DA);
  (void) CloseBlob(image);
  return(GetFirstImageInList(image));
}