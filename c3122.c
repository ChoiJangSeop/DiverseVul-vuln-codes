static Image *ReadICONImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  IconFile
    icon_file;

  IconInfo
    icon_info;

  Image
    *image;

  MagickBooleanType
    status;

  register ssize_t
    i,
    x;

  register Quantum
    *q;

  register unsigned char
    *p;

  size_t
    bit,
    byte,
    bytes_per_line,
    one,
    scanline_pad;

  ssize_t
    count,
    offset,
    y;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),"%s",image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  image=AcquireImage(image_info,exception);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  icon_file.reserved=(short) ReadBlobLSBShort(image);
  icon_file.resource_type=(short) ReadBlobLSBShort(image);
  icon_file.count=(short) ReadBlobLSBShort(image);
  if ((icon_file.reserved != 0) ||
      ((icon_file.resource_type != 1) && (icon_file.resource_type != 2)) ||
      (icon_file.count > MaxIcons))
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  for (i=0; i < icon_file.count; i++)
  {
    icon_file.directory[i].width=(unsigned char) ReadBlobByte(image);
    icon_file.directory[i].height=(unsigned char) ReadBlobByte(image);
    icon_file.directory[i].colors=(unsigned char) ReadBlobByte(image);
    icon_file.directory[i].reserved=(unsigned char) ReadBlobByte(image);
    icon_file.directory[i].planes=(unsigned short) ReadBlobLSBShort(image);
    icon_file.directory[i].bits_per_pixel=(unsigned short)
      ReadBlobLSBShort(image);
    icon_file.directory[i].size=ReadBlobLSBLong(image);
    icon_file.directory[i].offset=ReadBlobLSBLong(image);
    if (EOFBlob(image) != MagickFalse)
      {
        ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
          image->filename);
        break;
      }
  }
  one=1;
  for (i=0; i < icon_file.count; i++)
  {
    /*
      Verify Icon identifier.
    */
    offset=(ssize_t) SeekBlob(image,(MagickOffsetType)
      icon_file.directory[i].offset,SEEK_SET);
    if (offset < 0)
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    icon_info.size=ReadBlobLSBLong(image);
    icon_info.width=(unsigned char) ((int) ReadBlobLSBLong(image));
    icon_info.height=(unsigned char) ((int) ReadBlobLSBLong(image)/2);
    icon_info.planes=ReadBlobLSBShort(image);
    icon_info.bits_per_pixel=ReadBlobLSBShort(image);
    if (EOFBlob(image) != MagickFalse)
      {
        ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
          image->filename);
        break;
      }
    if (((icon_info.planes == 18505) && (icon_info.bits_per_pixel == 21060)) || 
        (icon_info.size == 0x474e5089))
      {
        Image
          *icon_image;

        ImageInfo
          *read_info;

        size_t
          length;

        unsigned char
          *png;

        /*
          Icon image encoded as a compressed PNG image.
        */
        length=icon_file.directory[i].size;
        png=(unsigned char *) AcquireQuantumMemory(length+16,sizeof(*png));
        if (png == (unsigned char *) NULL)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        (void) CopyMagickMemory(png,"\211PNG\r\n\032\n\000\000\000\015",12);
        png[12]=(unsigned char) icon_info.planes;
        png[13]=(unsigned char) (icon_info.planes >> 8);
        png[14]=(unsigned char) icon_info.bits_per_pixel;
        png[15]=(unsigned char) (icon_info.bits_per_pixel >> 8);
        count=ReadBlob(image,length-16,png+16);
        icon_image=(Image *) NULL;
        if (count > 0)
          {
            read_info=CloneImageInfo(image_info);
            (void) CopyMagickString(read_info->magick,"PNG",MagickPathExtent);
            icon_image=BlobToImage(read_info,png,length+16,exception);
            read_info=DestroyImageInfo(read_info);
          }
        png=(unsigned char *) RelinquishMagickMemory(png);
        if (icon_image == (Image *) NULL)
          {
            if (count != (ssize_t) (length-16))
              ThrowReaderException(CorruptImageError,
                "InsufficientImageDataInFile");
            image=DestroyImageList(image);
            return((Image *) NULL);
          }
        DestroyBlob(icon_image);
        icon_image->blob=ReferenceBlob(image->blob);
        ReplaceImageInList(&image,icon_image);
      }
    else
      {
        if (icon_info.bits_per_pixel > 32)
          ThrowReaderException(CorruptImageError,"ImproperImageHeader");
        icon_info.compression=ReadBlobLSBLong(image);
        icon_info.image_size=ReadBlobLSBLong(image);
        icon_info.x_pixels=ReadBlobLSBLong(image);
        icon_info.y_pixels=ReadBlobLSBLong(image);
        icon_info.number_colors=ReadBlobLSBLong(image);
        icon_info.colors_important=ReadBlobLSBLong(image);
        image->alpha_trait=BlendPixelTrait;
        image->columns=(size_t) icon_file.directory[i].width;
        if ((ssize_t) image->columns > icon_info.width)
          image->columns=(size_t) icon_info.width;
        if (image->columns == 0)
          image->columns=256;
        image->rows=(size_t) icon_file.directory[i].height;
        if ((ssize_t) image->rows > icon_info.height)
          image->rows=(size_t) icon_info.height;
        if (image->rows == 0)
          image->rows=256;
        image->depth=icon_info.bits_per_pixel;
        if (image->debug != MagickFalse)
          {
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              " scene    = %.20g",(double) i);
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "   size   = %.20g",(double) icon_info.size);
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "   width  = %.20g",(double) icon_file.directory[i].width);
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "   height = %.20g",(double) icon_file.directory[i].height);
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "   colors = %.20g",(double ) icon_info.number_colors);
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "   planes = %.20g",(double) icon_info.planes);
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "   bpp    = %.20g",(double) icon_info.bits_per_pixel);
          }
      if ((icon_info.number_colors != 0) || (icon_info.bits_per_pixel <= 16U))
        {
          image->storage_class=PseudoClass;
          image->colors=icon_info.number_colors;
          if (image->colors == 0)
            image->colors=one << icon_info.bits_per_pixel;
        }
      if (image->storage_class == PseudoClass)
        {
          register ssize_t
            i;

          unsigned char
            *icon_colormap;

          /*
            Read Icon raster colormap.
          */
          if (AcquireImageColormap(image,image->colors,exception) ==
              MagickFalse)
            ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
          icon_colormap=(unsigned char *) AcquireQuantumMemory((size_t)
            image->colors,4UL*sizeof(*icon_colormap));
          if (icon_colormap == (unsigned char *) NULL)
            ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
          count=ReadBlob(image,(size_t) (4*image->colors),icon_colormap);
          if (count != (ssize_t) (4*image->colors))
            ThrowReaderException(CorruptImageError,
              "InsufficientImageDataInFile");
          p=icon_colormap;
          for (i=0; i < (ssize_t) image->colors; i++)
          {
            image->colormap[i].blue=(Quantum) ScaleCharToQuantum(*p++);
            image->colormap[i].green=(Quantum) ScaleCharToQuantum(*p++);
            image->colormap[i].red=(Quantum) ScaleCharToQuantum(*p++);
            p++;
          }
          icon_colormap=(unsigned char *) RelinquishMagickMemory(icon_colormap);
        }
        /*
          Convert Icon raster image to pixel packets.
        */
        if ((image_info->ping != MagickFalse) &&
            (image_info->number_scenes != 0))
          if (image->scene >= (image_info->scene+image_info->number_scenes-1))
            break;
        status=SetImageExtent(image,image->columns,image->rows,exception);
        if (status == MagickFalse)
          return(DestroyImageList(image));
        bytes_per_line=(((image->columns*icon_info.bits_per_pixel)+31) &
          ~31) >> 3;
        (void) bytes_per_line;
        scanline_pad=((((image->columns*icon_info.bits_per_pixel)+31) & ~31)-
          (image->columns*icon_info.bits_per_pixel)) >> 3;
        switch (icon_info.bits_per_pixel)
        {
          case 1:
          {
            /*
              Convert bitmap scanline.
            */
            for (y=(ssize_t) image->rows-1; y >= 0; y--)
            {
              q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
              if (q == (Quantum *) NULL)
                break;
              for (x=0; x < (ssize_t) (image->columns-7); x+=8)
              {
                byte=(size_t) ReadBlobByte(image);
                for (bit=0; bit < 8; bit++)
                {
                  SetPixelIndex(image,((byte & (0x80 >> bit)) != 0 ? 0x01 :
                    0x00),q);
                  q+=GetPixelChannels(image);
                }
              }
              if ((image->columns % 8) != 0)
                {
                  byte=(size_t) ReadBlobByte(image);
                  for (bit=0; bit < (image->columns % 8); bit++)
                  {
                    SetPixelIndex(image,((byte & (0x80 >> bit)) != 0 ? 0x01 :
                      0x00),q);
                    q+=GetPixelChannels(image);
                  }
                }
              for (x=0; x < (ssize_t) scanline_pad; x++)
                (void) ReadBlobByte(image);
              if (SyncAuthenticPixels(image,exception) == MagickFalse)
                break;
              if (image->previous == (Image *) NULL)
                {
                  status=SetImageProgress(image,LoadImageTag,image->rows-y-1,
                    image->rows);
                  if (status == MagickFalse)
                    break;
                }
            }
            break;
          }
          case 4:
          {
            /*
              Read 4-bit Icon scanline.
            */
            for (y=(ssize_t) image->rows-1; y >= 0; y--)
            {
              q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
              if (q == (Quantum *) NULL)
                break;
              for (x=0; x < ((ssize_t) image->columns-1); x+=2)
              {
                byte=(size_t) ReadBlobByte(image);
                SetPixelIndex(image,((byte >> 4) & 0xf),q);
                q+=GetPixelChannels(image);
                SetPixelIndex(image,((byte) & 0xf),q);
                q+=GetPixelChannels(image);
              }
              if ((image->columns % 2) != 0)
                {
                  byte=(size_t) ReadBlobByte(image);
                  SetPixelIndex(image,((byte >> 4) & 0xf),q);
                  q+=GetPixelChannels(image);
                }
              for (x=0; x < (ssize_t) scanline_pad; x++)
                (void) ReadBlobByte(image);
              if (SyncAuthenticPixels(image,exception) == MagickFalse)
                break;
              if (image->previous == (Image *) NULL)
                {
                  status=SetImageProgress(image,LoadImageTag,image->rows-y-1,
                    image->rows);
                  if (status == MagickFalse)
                    break;
                }
            }
            break;
          }
          case 8:
          {
            /*
              Convert PseudoColor scanline.
            */
            for (y=(ssize_t) image->rows-1; y >= 0; y--)
            {
              q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
              if (q == (Quantum *) NULL)
                break;
              for (x=0; x < (ssize_t) image->columns; x++)
              {
                byte=(size_t) ReadBlobByte(image);
                SetPixelIndex(image,byte,q);
                q+=GetPixelChannels(image);
              }
              for (x=0; x < (ssize_t) scanline_pad; x++)
                (void) ReadBlobByte(image);
              if (SyncAuthenticPixels(image,exception) == MagickFalse)
                break;
              if (image->previous == (Image *) NULL)
                {
                  status=SetImageProgress(image,LoadImageTag,image->rows-y-1,
                    image->rows);
                  if (status == MagickFalse)
                    break;
                }
            }
            break;
          }
          case 16:
          {
            /*
              Convert PseudoColor scanline.
            */
            for (y=(ssize_t) image->rows-1; y >= 0; y--)
            {
              q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
              if (q == (Quantum *) NULL)
                break;
              for (x=0; x < (ssize_t) image->columns; x++)
              {
                byte=(size_t) ReadBlobByte(image);
                byte|=(size_t) (ReadBlobByte(image) << 8);
                SetPixelIndex(image,byte,q);
                q+=GetPixelChannels(image);
              }
              for (x=0; x < (ssize_t) scanline_pad; x++)
                (void) ReadBlobByte(image);
              if (SyncAuthenticPixels(image,exception) == MagickFalse)
                break;
              if (image->previous == (Image *) NULL)
                {
                  status=SetImageProgress(image,LoadImageTag,image->rows-y-1,
                    image->rows);
                  if (status == MagickFalse)
                    break;
                }
            }
            break;
          }
          case 24:
          case 32:
          {
            /*
              Convert DirectColor scanline.
            */
            for (y=(ssize_t) image->rows-1; y >= 0; y--)
            {
              q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
              if (q == (Quantum *) NULL)
                break;
              for (x=0; x < (ssize_t) image->columns; x++)
              {
                SetPixelBlue(image,ScaleCharToQuantum((unsigned char)
                  ReadBlobByte(image)),q);
                SetPixelGreen(image,ScaleCharToQuantum((unsigned char)
                  ReadBlobByte(image)),q);
                SetPixelRed(image,ScaleCharToQuantum((unsigned char)
                  ReadBlobByte(image)),q);
                if (icon_info.bits_per_pixel == 32)
                  SetPixelAlpha(image,ScaleCharToQuantum((unsigned char)
                    ReadBlobByte(image)),q);
                q+=GetPixelChannels(image);
              }
              if (icon_info.bits_per_pixel == 24)
                for (x=0; x < (ssize_t) scanline_pad; x++)
                  (void) ReadBlobByte(image);
              if (SyncAuthenticPixels(image,exception) == MagickFalse)
                break;
              if (image->previous == (Image *) NULL)
                {
                  status=SetImageProgress(image,LoadImageTag,image->rows-y-1,
                    image->rows);
                  if (status == MagickFalse)
                    break;
                }
            }
            break;
          }
          default:
            ThrowReaderException(CorruptImageError,"ImproperImageHeader");
        }
        if (image_info->ping == MagickFalse)
          (void) SyncImage(image,exception);
        if (icon_info.bits_per_pixel != 32)
          {
            /*
              Read the ICON alpha mask.
            */
            image->storage_class=DirectClass;
            for (y=(ssize_t) image->rows-1; y >= 0; y--)
            {
              q=GetAuthenticPixels(image,0,y,image->columns,1,exception);
              if (q == (Quantum *) NULL)
                break;
              for (x=0; x < ((ssize_t) image->columns-7); x+=8)
              {
                byte=(size_t) ReadBlobByte(image);
                for (bit=0; bit < 8; bit++)
                {
                  SetPixelAlpha(image,(((byte & (0x80 >> bit)) != 0) ?
                    TransparentAlpha : OpaqueAlpha),q);
                  q+=GetPixelChannels(image);
                }
              }
              if ((image->columns % 8) != 0)
                {
                  byte=(size_t) ReadBlobByte(image);
                  for (bit=0; bit < (image->columns % 8); bit++)
                  {
                    SetPixelAlpha(image,(((byte & (0x80 >> bit)) != 0) ?
                      TransparentAlpha : OpaqueAlpha),q);
                    q+=GetPixelChannels(image);
                  }
                }
              if ((image->columns % 32) != 0)
                for (x=0; x < (ssize_t) ((32-(image->columns % 32))/8); x++)
                  (void) ReadBlobByte(image);
              if (SyncAuthenticPixels(image,exception) == MagickFalse)
                break;
            }
          }
        if (EOFBlob(image) != MagickFalse)
          {
            ThrowFileException(exception,CorruptImageError,
              "UnexpectedEndOfFile",image->filename);
            break;
          }
      }
    /*
      Proceed to next image.
    */
    if (image_info->number_scenes != 0)
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    if (i < (ssize_t) (icon_file.count-1))
      {
        /*
          Allocate next image structure.
        */
        AcquireNextImage(image_info,image,exception);
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
  }
  (void) CloseBlob(image);
  return(GetFirstImageInList(image));
}