static MagickBooleanType ReadTIM2ImageData(const ImageInfo *image_info,
  Image *image,TIM2ImageHeader *header,char clut_depth,char bits_per_pixel,
  ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  ssize_t
    x;

  Quantum
    *q;

  unsigned char
    *p;

  size_t
    bits_per_line,
    bytes_per_line;

  ssize_t
    count,
    y;

  unsigned char
    *row_data;

  unsigned int
    word;

  status=SetImageExtent(image,image->columns,image->rows,exception);
  if (status == MagickFalse)
    return(MagickFalse);
  /*
   * User data
   */
  status=DiscardBlobBytes(image,header->header_size-48);
  if (status == MagickFalse)
    return(MagickFalse);
  /*
   * Image data
   */
  bits_per_line=image->columns*bits_per_pixel;
  bytes_per_line=bits_per_line/8 + ((bits_per_line%8==0) ? 0 : 1);
  row_data=(unsigned char*) AcquireQuantumMemory(1,bytes_per_line);
  if (row_data == (unsigned char *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image_info->filename);
  if (clut_depth != 0)
    {
      image->colors=header->clut_color_count;
      if (AcquireImageColormap(image,image->colors,exception) == MagickFalse)
        {
          row_data=(unsigned char *) RelinquishMagickMemory(row_data);
          ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
            image_info->filename);
        }
      switch (bits_per_pixel)
      {
        case 4:
        {
          for (y=0; y<(ssize_t) image->rows; y++)
          {
            q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
            if (q == (Quantum *) NULL)
              break;
            count=ReadBlob(image,bytes_per_line,row_data);
            if (count != (ssize_t) bytes_per_line)
              {
                row_data=(unsigned char *) RelinquishMagickMemory(row_data);
                ThrowBinaryException(CorruptImageError,
                  "InsufficientImageDataInFile",image_info->filename);
              }
            p=row_data;
            for (x=0; x < ((ssize_t) image->columns-1); x+=2)
            {
              SetPixelIndex(image,(*p >> 0) & 0x0F,q);
              q+=GetPixelChannels(image);
              SetPixelIndex(image,(*p >> 4) & 0x0F,q);
              p++;
              q+=GetPixelChannels(image);
            }
            if ((image->columns % 2) != 0)
              {
                SetPixelIndex(image,(*p >> 4) & 0x0F,q);
                p++;
                q+=GetPixelChannels(image);
              }
            if (SyncAuthenticPixels(image,exception) == MagickFalse)
              break;
            if (image->previous == (Image *) NULL)
              {
                status=SetImageProgress(image,LoadImageTag,
                  (MagickOffsetType) y,image->rows);
                if (status == MagickFalse)
                  break;
              }
          }
          break;
        }
        case 8:
        {
          for (y=0;y<(ssize_t) image->rows; y++)
          {
            q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
            if (q == (Quantum *) NULL)
              break;
            count=ReadBlob(image,bytes_per_line,row_data);
            if (count != (ssize_t) bytes_per_line)
              {
                row_data=(unsigned char *) RelinquishMagickMemory(row_data);
                ThrowBinaryException(CorruptImageError,
                  "InsufficientImageDataInFile",image_info->filename);
              }
            p=row_data;
            for (x=0; x < (ssize_t) image->columns; x++)
            {
              SetPixelIndex(image,*p,q);
              p++;
              q+=GetPixelChannels(image);
            }
            if (SyncAuthenticPixels(image,exception) == MagickFalse)
              break;
            if (image->previous == (Image *) NULL)
              {
                status=SetImageProgress(image,LoadImageTag,
                  (MagickOffsetType) y,image->rows);
                if (status == MagickFalse)
                  break;
              }
          }
          break;
        }
        default:
        {
          row_data=(unsigned char *) RelinquishMagickMemory(row_data);
          ThrowBinaryException(CorruptImageError,"ImproperImageHeader",
            image_info->filename);
        }
      }
    }
  else  /* has_clut==false */
    {
      switch (bits_per_pixel)
      {
        case 16:
        {
          for (y=0; y<(ssize_t) image->rows; y++)
          {
            q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
            if (q == (Quantum *) NULL)
              break;
            count=ReadBlob(image,bytes_per_line,row_data);
            if (count != (ssize_t) bytes_per_line)
              {
                row_data=(unsigned char *) RelinquishMagickMemory(row_data);
                ThrowBinaryException(CorruptImageError,
                  "InsufficientImageDataInFile",image_info->filename);
              }
            p=row_data;
            for (x=0; x < (ssize_t) image->columns; x++)
            {
              word = ((unsigned int)* p   )<<0*8 |
                      ((unsigned int)*(p+1))<<1*8;

              SetPixelRed(image,GetChannelValue(word,0,RGBA16),q);
              SetPixelGreen(image,GetChannelValue(word,1,RGBA16),q);
              SetPixelBlue(image,GetChannelValue(word,2,RGBA16),q);
              SetPixelAlpha(image,GetAlpha(word,RGBA16),q);
              q+=GetPixelChannels(image);
              p+=sizeof(unsigned short);
            }
            if (SyncAuthenticPixels(image,exception) == MagickFalse)
              break;
            if (image->previous == (Image *) NULL)
              {
                status=SetImageProgress(image,LoadImageTag,
                  (MagickOffsetType) y,image->rows);
                if (status == MagickFalse)
                  break;
              }
          }
          break;
        }
        case 24:
        {
          for (y = 0; y<(ssize_t) image->rows; y++)
          {
            q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
            if (q == (Quantum *) NULL)
              break;
            count=ReadBlob(image,bytes_per_line,row_data);
            if (count != (ssize_t) bytes_per_line)
              {
                row_data=(unsigned char *) RelinquishMagickMemory(row_data);
                ThrowBinaryException(CorruptImageError,
                  "InsufficientImageDataInFile",image_info->filename);
              }
            p=row_data;
            for (x=0; x < (ssize_t) image->columns; x++)
            {
              word = (unsigned int)(* p   )<<0*8 |
                      (unsigned int)(*(p+1))<<1*8 |
                      (unsigned int)(*(p+2))<<2*8;

              SetPixelRed(image,GetChannelValue(word,0,RGB24),q);
              SetPixelGreen(image,GetChannelValue(word,1,RGB24),q);
              SetPixelBlue(image,GetChannelValue(word,2,RGB24),q);
              q+=GetPixelChannels(image);
              p+=3;
            }
            if (SyncAuthenticPixels(image,exception) == MagickFalse)
              break;
            if (image->previous == (Image *) NULL)
              {
                status=SetImageProgress(image,LoadImageTag,
                  (MagickOffsetType) y,image->rows);
                if (status == MagickFalse)
                  break;
              }
          }
          break;
        }
        case 32:
        {  
          for (y = 0; y<(ssize_t) image->rows; y++)
          {
            q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
            if (q == (Quantum *) NULL)
              break;
            count=ReadBlob(image,bytes_per_line,row_data);
            if (count != (ssize_t) bytes_per_line)
              {
                row_data=(unsigned char *) RelinquishMagickMemory(row_data);
                ThrowBinaryException(CorruptImageError,
                  "InsufficientImageDataInFile",image_info->filename);
              }
            p=row_data;
            for (x=0; x < (ssize_t) image->columns; x++)
            {
              word = ((unsigned int)* p   )<<0*8 |
                      ((unsigned int)*(p+1))<<1*8 |
                      ((unsigned int)*(p+2))<<2*8 |
                      ((unsigned int)*(p+3))<<3*8;

              SetPixelRed(image,GetChannelValue(word,0,RGBA32),q);
              SetPixelGreen(image,GetChannelValue(word,1,RGBA32),q);
              SetPixelBlue(image,GetChannelValue(word,2,RGBA32),q);
              SetPixelAlpha(image,GetAlpha(word,RGBA32),q);
              q+=GetPixelChannels(image);
              p+=4;
            }
            if (SyncAuthenticPixels(image,exception) == MagickFalse)
              break;
            if (image->previous == (Image *) NULL)
              {
                status=SetImageProgress(image,LoadImageTag,
                  (MagickOffsetType) y,image->rows);
                if (status == MagickFalse)
                  break;
              }
          }
          break;
        }
        default:
        {
          row_data=(unsigned char *) RelinquishMagickMemory(row_data);
          ThrowBinaryException(CorruptImageError,"ImproperImageHeader",
            image_info->filename);
        }
      }
    }
  row_data=(unsigned char *) RelinquishMagickMemory(row_data);
  if ((status != MagickFalse) && (clut_depth != 0))
  {
    CSM
      csm;

    ssize_t
      i;

    unsigned char
      *clut_data;

    /*
      * ### Read CLUT Data ###
      */
    clut_data=(unsigned char *) AcquireQuantumMemory(1,header->clut_size);
    if (clut_data == (unsigned char *) NULL)
      ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
        image_info->filename);
    count=ReadBlob(image,header->clut_size,clut_data);
    if (count != (ssize_t) (header->clut_size))
      {
        clut_data=(unsigned char *) RelinquishMagickMemory(clut_data);
        ThrowBinaryException(CorruptImageError,"InsufficientImageDataInFile",
          image_info->filename);
      }
    /*
      * ### Process CLUT Data ###
      */
    p=clut_data;
    switch(clut_depth)
    {
      case 16:
      {
        for (i=0; i < (ssize_t) image->colors; i++)
        {
          word = ((unsigned short)* p   )<<0*8 |
                  ((unsigned short)*(p+1))<<1*8;

          image->colormap[i].red=GetChannelValue(word,0,RGBA16);
          image->colormap[i].green=GetChannelValue(word,1,RGBA16);
          image->colormap[i].blue=GetChannelValue(word,2,RGBA16);
          image->colormap[i].alpha=GetAlpha(word,RGBA16);
          p+=2;
        }
        break;
      }
      case 24:
      {
        for (i=0; i < (ssize_t) image->colors; i++)
        {
          word = ((unsigned int)* p   )<<0*8 |
                  ((unsigned int)*(p+1))<<1*8 |
                  ((unsigned int)*(p+2))<<2*8;

          image->colormap[i].red=GetChannelValue(word,0,RGB24);
          image->colormap[i].green=GetChannelValue(word,1,RGB24);
          image->colormap[i].blue=GetChannelValue(word,2,RGB24);
          p+=3;
        }
        break;
      }
      case 32:
      {
        for (i=0; i < (ssize_t) image->colors; i++)
        {
          word = ((unsigned int)* p   )<<0*8 |
                  ((unsigned int)*(p+1))<<1*8 |
                  ((unsigned int)*(p+2))<<2*8 |
                  ((unsigned int)*(p+3))<<3*8;

          image->colormap[i].red=GetChannelValue(word,0,RGBA32);
          image->colormap[i].green=GetChannelValue(word,1,RGBA32);
          image->colormap[i].blue=GetChannelValue(word,2,RGBA32);
          image->colormap[i].alpha=GetAlpha(word,RGBA32);
          p+=4;
        }
        break;
      }
    }
    clut_data=(unsigned char *) RelinquishMagickMemory(clut_data);
    /* CSM: CLUT Storage Mode */
    switch ((int) header->clut_type>>4)  /* High 4 bits */
    {
      case 0:
        csm=CSM1;
        break;
      case 1:
        csm=CSM2;
        break;
      default:
        ThrowBinaryException(CorruptImageError,"ImproperImageHeader",
          image_info->filename);
        break;
    }
    if (csm==CSM1)
      {
        PixelInfo
          *oldColormap;

        oldColormap=(PixelInfo *) AcquireQuantumMemory((size_t)(image->colors)+1,
          sizeof(*image->colormap));
        if (oldColormap == (PixelInfo *) NULL)
          ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
            image_info->filename);
        deshufflePalette(image,oldColormap);
        RelinquishMagickMemory(oldColormap);
      }
  }
  return(status);
}