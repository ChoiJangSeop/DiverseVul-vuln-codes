static Image *ReadDCMImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
#define ThrowDCMException(exception,message) \
{ \
  RelinquishDCMMemory(&info,&map,stream_info,stack,data); \
  if (info_copy != (DCMInfo *) NULL) \
    info_copy=(DCMInfo *) RelinquishDCMInfo(info_copy); \
  ThrowReaderException((exception),(message)); \
}

  char
    explicit_vr[MagickPathExtent],
    implicit_vr[MagickPathExtent],
    magick[MagickPathExtent],
    photometric[MagickPathExtent];

  DCMInfo
    info,
    *info_copy = (DCMInfo *) NULL;

  DCMMap
    map;

  DCMStreamInfo
    *stream_info;

  Image
    *image;

  int
    datum;

  LinkedListInfo
    *stack;

  MagickBooleanType
    explicit_file,
    explicit_retry,
    use_explicit;

  MagickOffsetType
    blob_size,
    offset;

  unsigned char
    *p;

  ssize_t
    i;

  size_t
    colors,
    length,
    number_scenes,
    quantum,
    status;

  ssize_t
    count,
    scene,
    sequence_depth;

  unsigned char
    *data;

  unsigned short
    group,
    element;

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
  image=AcquireImage(image_info,exception);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  image->depth=8UL;
  image->endian=LSBEndian;
  /*
    Read DCM preamble.
  */
  (void) memset(&info,0,sizeof(info));
  (void) memset(&map,0,sizeof(map));
  data=(unsigned char *) NULL;
  stream_info=(DCMStreamInfo *) AcquireMagickMemory(sizeof(*stream_info));
  sequence_depth=0;
  stack=NewLinkedList(256);
  if (stream_info == (DCMStreamInfo *) NULL)
    ThrowDCMException(ResourceLimitError,"MemoryAllocationFailed")
  (void) memset(stream_info,0,sizeof(*stream_info));
  count=ReadBlob(image,128,(unsigned char *) magick);
  if (count != 128)
    ThrowDCMException(CorruptImageError,"ImproperImageHeader")
  count=ReadBlob(image,4,(unsigned char *) magick);
  if ((count != 4) || (LocaleNCompare(magick,"DICM",4) != 0))
    {
      offset=SeekBlob(image,0L,SEEK_SET);
      if (offset < 0)
        ThrowDCMException(CorruptImageError,"ImproperImageHeader")
    }
  /*
    Read DCM Medical image.
  */
  (void) CopyMagickString(photometric,"MONOCHROME1 ",MagickPathExtent);
  info.bits_allocated=8;
  info.bytes_per_pixel=1;
  info.depth=8;
  info.mask=0xffff;
  info.max_value=255UL;
  info.samples_per_pixel=1;
  info.signed_data=(~0UL);
  info.rescale_slope=1.0;
  element=0;
  explicit_vr[2]='\0';
  explicit_file=MagickFalse;
  colors=0;
  number_scenes=1;
  use_explicit=MagickFalse;
  explicit_retry=MagickFalse;
  blob_size=(MagickOffsetType) GetBlobSize(image);
  while (TellBlob(image) < blob_size)
  {
    for (group=0; (group != 0x7FE0) || (element != 0x0010) ; )
    {
      /*
        Read a group.
      */
      image->offset=(ssize_t) TellBlob(image);
      group=ReadBlobLSBShort(image);
      element=ReadBlobLSBShort(image);
      if ((group == 0xfffc) && (element == 0xfffc))
        break;
      if ((group != 0x0002) && (image->endian == MSBEndian))
        {
          group=(unsigned short) ((group << 8) | ((group >> 8) & 0xFF));
          element=(unsigned short) ((element << 8) | ((element >> 8) & 0xFF));
        }
      quantum=0;
      /*
        Find corresponding VR for this group and element.
      */
      for (i=0; dicom_info[i].group < 0xffff; i++)
        if ((group == dicom_info[i].group) &&
            (element == dicom_info[i].element))
          break;
      (void) CopyMagickString(implicit_vr,dicom_info[i].vr,MagickPathExtent);
      count=ReadBlob(image,2,(unsigned char *) explicit_vr);
      if (count != 2)
        ThrowDCMException(CorruptImageError,"ImproperImageHeader")
      /*
        Check for "explicitness", but meta-file headers always explicit.
      */
      if ((explicit_file == MagickFalse) && (group != 0x0002))
        explicit_file=(isupper((int) ((unsigned char) *explicit_vr)) != 0) &&
          (isupper((int) ((unsigned char) *(explicit_vr+1))) != 0) ?
          MagickTrue : MagickFalse;
      use_explicit=((group == 0x0002) && (explicit_retry == MagickFalse)) ||
        (explicit_file != MagickFalse) ? MagickTrue : MagickFalse;
      if ((use_explicit != MagickFalse) && (strncmp(implicit_vr,"xs",2) == 0))
        (void) CopyMagickString(implicit_vr,explicit_vr,MagickPathExtent);
      if ((use_explicit == MagickFalse) || (strncmp(implicit_vr,"!!",2) == 0))
        {
          offset=SeekBlob(image,(MagickOffsetType) -2,SEEK_CUR);
          if (offset < 0)
            ThrowDCMException(CorruptImageError,"ImproperImageHeader")
          quantum=4;
        }
      else
        {
          /*
            Assume explicit type.
          */
          quantum=2;
          if ((strcmp(explicit_vr,"OB") == 0) ||
              (strcmp(explicit_vr,"OW") == 0) ||
              (strcmp(explicit_vr,"OF") == 0) ||
              (strcmp(explicit_vr,"SQ") == 0) ||
              (strcmp(explicit_vr,"UN") == 0) ||
              (strcmp(explicit_vr,"UT") == 0))
            {
              (void) ReadBlobLSBShort(image);
              quantum=4;
            }
        }
      if ((group == 0xFFFE) && (element == 0xE0DD))
        {
          /*
            If we're exiting a sequence, restore the previous image parameters,
            effectively undoing any parameter changes that happened inside the
            sequence.
          */
          sequence_depth--;
          info_copy=(DCMInfo *) RemoveLastElementFromLinkedList(stack);
          if (info_copy == (DCMInfo *)NULL)
            {
              /*
                The sequence's entry and exit points don't line up (tried to
                exit one more sequence than we entered).
              */
              ThrowDCMException(CorruptImageError,"ImproperImageHeader")
            }
          if (info.scale != (Quantum *) NULL)
            info.scale=(Quantum *) RelinquishMagickMemory(info.scale);
          (void) memcpy(&info,info_copy,sizeof(info));
          info_copy=(DCMInfo *) RelinquishMagickMemory(info_copy);
        }
      if (strcmp(explicit_vr,"SQ") == 0)
        {
          /*
            If we're entering a sequence, push the current image parameters
            onto the stack, so we can restore them at the end of the sequence.
          */
          info_copy=(DCMInfo *) AcquireMagickMemory(sizeof(info));
          if (info_copy == (DCMInfo *) NULL)
            ThrowDCMException(ResourceLimitError,"MemoryAllocationFailed")
          (void) memcpy(info_copy,&info,sizeof(info));
          info_copy->scale=(Quantum *) AcquireQuantumMemory(
            info_copy->scale_size+1,sizeof(*info_copy->scale));
          if (info_copy->scale == (Quantum *) NULL)
            ThrowDCMException(ResourceLimitError,"MemoryAllocationFailed")
          (void) memcpy(info_copy->scale,info.scale,info_copy->scale_size*
            sizeof(*info_copy->scale));
          AppendValueToLinkedList(stack,info_copy);
          sequence_depth++;
        }
      datum=0;
      if (quantum == 4)
        {
          if (group == 0x0002)
            datum=ReadBlobLSBSignedLong(image);
          else
            datum=ReadBlobSignedLong(image);
        }
      else
        if (quantum == 2)
          {
            if (group == 0x0002)
              datum=ReadBlobLSBSignedShort(image);
            else
              datum=ReadBlobSignedShort(image);
          }
      quantum=0;
      length=1;
      if (datum != 0)
        {
          if ((strncmp(implicit_vr,"OW",2) == 0) ||
              (strncmp(implicit_vr,"SS",2) == 0) ||
              (strncmp(implicit_vr,"US",2) == 0))
            quantum=2;
          else
            if ((strncmp(implicit_vr,"FL",2) == 0) ||
                (strncmp(implicit_vr,"OF",2) == 0) ||
                (strncmp(implicit_vr,"SL",2) == 0) ||
                (strncmp(implicit_vr,"UL",2) == 0))
              quantum=4;
            else
              if (strncmp(implicit_vr,"FD",2) == 0)
                quantum=8;
              else
                quantum=1;
          if (datum != ~0)
            length=(size_t) datum/quantum;
          else
            {
              /*
                Sequence and item of undefined length.
              */
              quantum=0;
              length=0;
            }
        }
      if (image_info->verbose != MagickFalse)
        {
          /*
            Display Dicom info.
          */
          if (use_explicit == MagickFalse)
            explicit_vr[0]='\0';
          for (i=0; dicom_info[i].description != (char *) NULL; i++)
            if ((group == dicom_info[i].group) &&
                (element == dicom_info[i].element))
              break;
          (void) FormatLocaleFile(stdout,
            "0x%04lX %4ld S%ld %s-%s (0x%04lx,0x%04lx)",
            (unsigned long) image->offset,(long) length,(long) sequence_depth,
            implicit_vr,explicit_vr,(unsigned long) group,
            (unsigned long) element);
          if (dicom_info[i].description != (char *) NULL)
            (void) FormatLocaleFile(stdout," %s",dicom_info[i].description);
          (void) FormatLocaleFile(stdout,": ");
        }
      if ((group == 0x7FE0) && (element == 0x0010))
        {
          if (image_info->verbose != MagickFalse)
            (void) FormatLocaleFile(stdout,"\n");
          break;
        }
      /*
        Allocate space and read an array.
      */
      data=(unsigned char *) NULL;
      if ((length == 1) && (quantum == 1))
        datum=ReadBlobByte(image);
      else
        if ((length == 1) && (quantum == 2))
          {
            if (group == 0x0002)
              datum=ReadBlobLSBSignedShort(image);
            else
              datum=ReadBlobSignedShort(image);
          }
        else
          if ((length == 1) && (quantum == 4))
            {
              if (group == 0x0002)
                datum=ReadBlobLSBSignedLong(image);
              else
                datum=ReadBlobSignedLong(image);
            }
          else
            if ((quantum != 0) && (length != 0))
              {
                if (length > (size_t) GetBlobSize(image))
                  ThrowDCMException(CorruptImageError,
                    "InsufficientImageDataInFile")
                if (~length >= 1)
                  data=(unsigned char *) AcquireQuantumMemory(length+1,quantum*
                    sizeof(*data));
                if (data == (unsigned char *) NULL)
                  ThrowDCMException(ResourceLimitError,
                    "MemoryAllocationFailed")
                count=ReadBlob(image,(size_t) quantum*length,data);
                if (count != (ssize_t) (quantum*length))
                  {
                    if (image_info->verbose != MagickFalse)
                      (void) FormatLocaleFile(stdout,"count=%d quantum=%d "
                        "length=%d group=%d\n",(int) count,(int) quantum,(int)
                        length,(int) group);
                     ThrowDCMException(CorruptImageError,
                       "InsufficientImageDataInFile")
                  }
                data[length*quantum]='\0';
              }
      if ((((unsigned int) group << 16) | element) == 0xFFFEE0DD)
        {
          if (data != (unsigned char *) NULL)
            data=(unsigned char *) RelinquishMagickMemory(data);
          continue;
        }
      switch (group)
      {
        case 0x0002:
        {
          switch (element)
          {
            case 0x0010:
            {
              char
                transfer_syntax[MagickPathExtent];

              /*
                Transfer Syntax.
              */
              if ((datum == 0) && (explicit_retry == MagickFalse))
                {
                  explicit_retry=MagickTrue;
                  (void) SeekBlob(image,(MagickOffsetType) 0,SEEK_SET);
                  group=0;
                  element=0;
                  if (image_info->verbose != MagickFalse)
                    (void) FormatLocaleFile(stdout,
                      "Corrupted image - trying explicit format\n");
                  break;
                }
              *transfer_syntax='\0';
              if (data != (unsigned char *) NULL)
                (void) CopyMagickString(transfer_syntax,(char *) data,
                  MagickPathExtent);
              if (image_info->verbose != MagickFalse)
                (void) FormatLocaleFile(stdout,"transfer_syntax=%s\n",
                  (const char *) transfer_syntax);
              if (strncmp(transfer_syntax,"1.2.840.10008.1.2",17) == 0)
                {
                  int
                    subtype,
                    type;

                  type=1;
                  subtype=0;
                  if (strlen(transfer_syntax) > 17)
                    {
                      count=(ssize_t) sscanf(transfer_syntax+17,".%d.%d",&type,
                        &subtype);
                      if (count < 1)
                        ThrowDCMException(CorruptImageError,
                          "ImproperImageHeader")
                    }
                  switch (type)
                  {
                    case 1:
                    {
                      image->endian=LSBEndian;
                      break;
                    }
                    case 2:
                    {
                      image->endian=MSBEndian;
                      break;
                    }
                    case 4:
                    {
                      if ((subtype >= 80) && (subtype <= 81))
                        image->compression=JPEGCompression;
                      else
                        if ((subtype >= 90) && (subtype <= 93))
                          image->compression=JPEG2000Compression;
                        else
                          image->compression=JPEGCompression;
                      break;
                    }
                    case 5:
                    {
                      image->compression=RLECompression;
                      break;
                    }
                  }
                }
              break;
            }
            default:
              break;
          }
          break;
        }
        case 0x0028:
        {
          switch (element)
          {
            case 0x0002:
            {
              /*
                Samples per pixel.
              */
              info.samples_per_pixel=(size_t) datum;
              if ((info.samples_per_pixel == 0) || (info.samples_per_pixel > 4))
                ThrowDCMException(CorruptImageError,"ImproperImageHeader")
              break;
            }
            case 0x0004:
            {
              /*
                Photometric interpretation.
              */
              if (data == (unsigned char *) NULL)
                break;
              for (i=0; i < (ssize_t) MagickMin(length,MagickPathExtent-1); i++)
                photometric[i]=(char) data[i];
              photometric[i]='\0';
              info.polarity=LocaleCompare(photometric,"MONOCHROME1 ") == 0 ?
                MagickTrue : MagickFalse;
              break;
            }
            case 0x0006:
            {
              /*
                Planar configuration.
              */
              if (datum == 1)
                image->interlace=PlaneInterlace;
              break;
            }
            case 0x0008:
            {
              /*
                Number of frames.
              */
              if (data == (unsigned char *) NULL)
                break;
              number_scenes=StringToUnsignedLong((char *) data);
              break;
            }
            case 0x0010:
            {
              /*
                Image rows.
              */
              info.height=(size_t) datum;
              break;
            }
            case 0x0011:
            {
              /*
                Image columns.
              */
              info.width=(size_t) datum;
              break;
            }
            case 0x0100:
            {
              /*
                Bits allocated.
              */
              info.bits_allocated=(size_t) datum;
              info.bytes_per_pixel=1;
              if (datum > 8)
                info.bytes_per_pixel=2;
              info.depth=info.bits_allocated;
              if ((info.depth == 0) || (info.depth > 32))
                ThrowDCMException(CorruptImageError,"ImproperImageHeader")
              info.max_value=(1UL << info.bits_allocated)-1;
              image->depth=info.depth;
              break;
            }
            case 0x0101:
            {
              /*
                Bits stored.
              */
              info.significant_bits=(size_t) datum;
              info.bytes_per_pixel=1;
              if (info.significant_bits > 8)
                info.bytes_per_pixel=2;
              info.depth=info.significant_bits;
              if ((info.depth == 0) || (info.depth > 16))
                ThrowDCMException(CorruptImageError,"ImproperImageHeader")
              info.max_value=(1UL << info.significant_bits)-1;
              info.mask=(size_t) GetQuantumRange(info.significant_bits);
              image->depth=info.depth;
              break;
            }
            case 0x0102:
            {
              /*
                High bit.
              */
              break;
            }
            case 0x0103:
            {
              /*
                Pixel representation.
              */
              info.signed_data=(size_t) datum;
              break;
            }
            case 0x1050:
            {
              /*
                Visible pixel range: center.
              */
              if (data != (unsigned char *) NULL)
                info.window_center=StringToDouble((char *) data,(char **) NULL);
              break;
            }
            case 0x1051:
            {
              /*
                Visible pixel range: width.
              */
              if (data != (unsigned char *) NULL)
                info.window_width=StringToDouble((char *) data,(char **) NULL);
              break;
            }
            case 0x1052:
            {
              /*
                Rescale intercept
              */
              if (data != (unsigned char *) NULL)
                info.rescale_intercept=StringToDouble((char *) data,
                  (char **) NULL);
              break;
            }
            case 0x1053:
            {
              /*
                Rescale slope
              */
              if (data != (unsigned char *) NULL)
                info.rescale_slope=StringToDouble((char *) data,(char **) NULL);
              break;
            }
            case 0x1200:
            case 0x3006:
            {
              /*
                Populate graymap.
              */
              if (data == (unsigned char *) NULL)
                break;
              colors=(size_t) (length/info.bytes_per_pixel);
              datum=(int) colors;
              if (map.gray != (int *) NULL)
                map.gray=(int *) RelinquishMagickMemory(map.gray);
              map.gray=(int *) AcquireQuantumMemory(MagickMax(colors,65536),
                sizeof(*map.gray));
              if (map.gray == (int *) NULL)
                ThrowDCMException(ResourceLimitError,"MemoryAllocationFailed")
              (void) memset(map.gray,0,MagickMax(colors,65536)*
                sizeof(*map.gray));
              for (i=0; i < (ssize_t) colors; i++)
                if (info.bytes_per_pixel == 1)
                  map.gray[i]=(int) data[i];
                else
                  map.gray[i]=(int) ((short *) data)[i];
              break;
            }
            case 0x1201:
            {
              unsigned short
                index;

              /*
                Populate redmap.
              */
              if (data == (unsigned char *) NULL)
                break;
              colors=(size_t) (length/info.bytes_per_pixel);
              datum=(int) colors;
              if (map.red != (int *) NULL)
                map.red=(int *) RelinquishMagickMemory(map.red);
              map.red=(int *) AcquireQuantumMemory(MagickMax(colors,65536),
                sizeof(*map.red));
              if (map.red == (int *) NULL)
                ThrowDCMException(ResourceLimitError,"MemoryAllocationFailed")
              (void) memset(map.red,0,MagickMax(colors,65536)*
                sizeof(*map.red));
              p=data;
              for (i=0; i < (ssize_t) colors; i++)
              {
                if (image->endian == MSBEndian)
                  index=(unsigned short) ((*p << 8) | *(p+1));
                else
                  index=(unsigned short) (*p | (*(p+1) << 8));
                map.red[i]=(int) index;
                p+=2;
              }
              break;
            }
            case 0x1202:
            {
              unsigned short
                index;

              /*
                Populate greenmap.
              */
              if (data == (unsigned char *) NULL)
                break;
              colors=(size_t) (length/info.bytes_per_pixel);
              datum=(int) colors;
              if (map.green != (int *) NULL)
                map.green=(int *) RelinquishMagickMemory(map.green);
              map.green=(int *) AcquireQuantumMemory(MagickMax(colors,65536),
                sizeof(*map.green));
              if (map.green == (int *) NULL)
                ThrowDCMException(ResourceLimitError,"MemoryAllocationFailed")
              (void) memset(map.green,0,MagickMax(colors,65536)*
                sizeof(*map.green));
              p=data;
              for (i=0; i < (ssize_t) colors; i++)
              {
                if (image->endian == MSBEndian)
                  index=(unsigned short) ((*p << 8) | *(p+1));
                else
                  index=(unsigned short) (*p | (*(p+1) << 8));
                map.green[i]=(int) index;
                p+=2;
              }
              break;
            }
            case 0x1203:
            {
              unsigned short
                index;

              /*
                Populate bluemap.
              */
              if (data == (unsigned char *) NULL)
                break;
              colors=(size_t) (length/info.bytes_per_pixel);
              datum=(int) colors;
              if (map.blue != (int *) NULL)
                map.blue=(int *) RelinquishMagickMemory(map.blue);
              map.blue=(int *) AcquireQuantumMemory(MagickMax(colors,65536),
                sizeof(*map.blue));
              if (map.blue == (int *) NULL)
                ThrowDCMException(ResourceLimitError,"MemoryAllocationFailed")
              (void) memset(map.blue,0,MagickMax(colors,65536)*
                sizeof(*map.blue));
              p=data;
              for (i=0; i < (ssize_t) colors; i++)
              {
                if (image->endian == MSBEndian)
                  index=(unsigned short) ((*p << 8) | *(p+1));
                else
                  index=(unsigned short) (*p | (*(p+1) << 8));
                map.blue[i]=(int) index;
                p+=2;
              }
              break;
            }
            default:
              break;
          }
          break;
        }
        case 0x2050:
        {
          switch (element)
          {
            case 0x0020:
            {
              if ((data != (unsigned char *) NULL) &&
                  (strncmp((char *) data,"INVERSE",7) == 0))
                info.polarity=MagickTrue;
              break;
            }
            default:
              break;
          }
          break;
        }
        default:
          break;
      }
      if (data != (unsigned char *) NULL)
        {
          char
            *attribute;

          for (i=0; dicom_info[i].description != (char *) NULL; i++)
            if ((group == dicom_info[i].group) &&
                (element == dicom_info[i].element))
              break;
          if (dicom_info[i].description != (char *) NULL)
            {
              attribute=AcquireString("dcm:");
              (void) ConcatenateString(&attribute,dicom_info[i].description);
              for (i=0; i < (ssize_t) MagickMax(length,4); i++)
                if (isprint((int) data[i]) == 0)
                  break;
              if ((i == (ssize_t) length) || (length > 4))
                {
                  (void) SubstituteString(&attribute," ","");
                  (void) SetImageProperty(image,attribute,(char *) data,
                    exception);
                }
              attribute=DestroyString(attribute);
            }
        }
      if (image_info->verbose != MagickFalse)
        {
          if (data == (unsigned char *) NULL)
            (void) FormatLocaleFile(stdout,"%d\n",datum);
          else
            {
              /*
                Display group data.
              */
              for (i=0; i < (ssize_t) MagickMax(length,4); i++)
                if (isprint((int) data[i]) == 0)
                  break;
              if ((i != (ssize_t) length) && (length <= 4))
                {
                  ssize_t
                    j;

                  datum=0;
                  for (j=(ssize_t) length-1; j >= 0; j--)
                    datum=(256*datum+data[j]);
                  (void) FormatLocaleFile(stdout,"%d",datum);
                }
              else
                for (i=0; i < (ssize_t) length; i++)
                  if (isprint((int) data[i]) != 0)
                    (void) FormatLocaleFile(stdout,"%c",data[i]);
                  else
                    (void) FormatLocaleFile(stdout,"%c",'.');
              (void) FormatLocaleFile(stdout,"\n");
            }
        }
      if (data != (unsigned char *) NULL)
        data=(unsigned char *) RelinquishMagickMemory(data);
      if (EOFBlob(image) != MagickFalse)
        {
          ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
            image->filename);
          group=0xfffc;
          break;
        }
    }
    if ((group == 0xfffc) && (element == 0xfffc))
      {
        Image
          *last;

        last=RemoveLastImageFromList(&image);
        if (last != (Image *) NULL)
          last=DestroyImage(last);
        break;
      }
    if ((info.width == 0) || (info.height == 0))
      ThrowDCMException(CorruptImageError,"ImproperImageHeader")
    image->columns=info.width;
    image->rows=info.height;
    if (info.signed_data == 0xffff)
      info.signed_data=(size_t) (info.significant_bits == 16 ? 1 : 0);
    if ((image->compression == JPEGCompression) ||
        (image->compression == JPEG2000Compression))
      {
        Image
          *images;

        ImageInfo
          *read_info;

        int
          c;

        /*
          Read offset table.
        */
        for (i=0; i < (ssize_t) stream_info->remaining; i++)
          if (ReadBlobByte(image) == EOF)
            break;
        (void) (((ssize_t) ReadBlobLSBShort(image) << 16) |
          ReadBlobLSBShort(image));
        length=(size_t) ReadBlobLSBLong(image);
        if (length > (size_t) GetBlobSize(image))
          ThrowDCMException(CorruptImageError,"InsufficientImageDataInFile")
        stream_info->offset_count=length >> 2;
        if (stream_info->offset_count != 0)
          {
            if (stream_info->offsets != (ssize_t *) NULL)
              stream_info->offsets=(ssize_t *) RelinquishMagickMemory(
                stream_info->offsets);
            stream_info->offsets=(ssize_t *) AcquireQuantumMemory(
              stream_info->offset_count,sizeof(*stream_info->offsets));
            if (stream_info->offsets == (ssize_t *) NULL)
              ThrowDCMException(ResourceLimitError,"MemoryAllocationFailed")
            for (i=0; i < (ssize_t) stream_info->offset_count; i++)
              stream_info->offsets[i]=(ssize_t) ReadBlobLSBSignedLong(image);
            offset=TellBlob(image);
            for (i=0; i < (ssize_t) stream_info->offset_count; i++)
              stream_info->offsets[i]+=offset;
          }
        /*
          Handle non-native image formats.
        */
        read_info=CloneImageInfo(image_info);
        SetImageInfoBlob(read_info,(void *) NULL,0);
        images=NewImageList();
        for (scene=0; scene < (ssize_t) number_scenes; scene++)
        {
          char
            filename[MagickPathExtent];

          const char
            *property;

          FILE
            *file;

          Image
            *jpeg_image;

          int
            unique_file;

          unsigned int
            tag;

          tag=((unsigned int) ReadBlobLSBShort(image) << 16) |
            ReadBlobLSBShort(image);
          length=(size_t) ReadBlobLSBLong(image);
          if (length > (size_t) GetBlobSize(image))
            {
              images=DestroyImageList(images);
              read_info=DestroyImageInfo(read_info);
              ThrowDCMException(CorruptImageError,"InsufficientImageDataInFile")
            }
          if (EOFBlob(image) != MagickFalse)
            {
              status=MagickFalse;
              break;
            }
          if (tag == 0xFFFEE0DD)
            break; /* sequence delimiter tag */
          if (tag != 0xFFFEE000)
            {
              status=MagickFalse;
              break;
            }
          file=(FILE *) NULL;
          unique_file=AcquireUniqueFileResource(filename);
          if (unique_file != -1)
            file=fdopen(unique_file,"wb");
          if (file == (FILE *) NULL)
            {
              (void) RelinquishUniqueFileResource(filename);
              ThrowFileException(exception,FileOpenError,
                "UnableToCreateTemporaryFile",filename);
              break;
            }
          for (c=EOF; length != 0; length--)
          {
            c=ReadBlobByte(image);
            if (c == EOF)
              {
                ThrowFileException(exception,CorruptImageError,
                  "UnexpectedEndOfFile",image->filename);
                break;
              }
            if (fputc(c,file) != c)
              break;
          }
          (void) fclose(file);
          if (c == EOF)
            break;
          (void) FormatLocaleString(read_info->filename,MagickPathExtent,
            "jpeg:%s",filename);
          if (image->compression == JPEG2000Compression)
            (void) FormatLocaleString(read_info->filename,MagickPathExtent,
              "j2k:%s",filename);
          jpeg_image=ReadImage(read_info,exception);
          if (jpeg_image != (Image *) NULL)
            {
              ResetImagePropertyIterator(image);
              property=GetNextImageProperty(image);
              while (property != (const char *) NULL)
              {
                (void) SetImageProperty(jpeg_image,property,
                  GetImageProperty(image,property,exception),exception);
                property=GetNextImageProperty(image);
              }
              AppendImageToList(&images,jpeg_image);
            }
          (void) RelinquishUniqueFileResource(filename);
        }
        read_info=DestroyImageInfo(read_info);
        image=DestroyImageList(image);
        if ((status == MagickFalse) && (exception->severity < ErrorException))
          {
            images=DestroyImageList(images);
            ThrowDCMException(CorruptImageError,"CorruptImageError")
          }
        else
          RelinquishDCMMemory(&info,&map,stream_info,stack,data);
        return(GetFirstImageInList(images));
      }
    if (info.depth != (1UL*MAGICKCORE_QUANTUM_DEPTH))
      {
        QuantumAny
          range;

        /*
          Compute pixel scaling table.
        */
        length=(size_t) (GetQuantumRange(info.depth)+1);
        if (length > (size_t) GetBlobSize(image))
          ThrowDCMException(CorruptImageError,"InsufficientImageDataInFile")
        if (info.scale != (Quantum *) NULL)
          info.scale=(Quantum *) RelinquishMagickMemory(info.scale);
        info.scale_size=MagickMax(length,MaxMap)+1;
        info.scale=(Quantum *) AcquireQuantumMemory(info.scale_size,
          sizeof(*info.scale));
        if (info.scale == (Quantum *) NULL)
          ThrowDCMException(ResourceLimitError,"MemoryAllocationFailed")
        (void) memset(info.scale,0,(MagickMax(length,MaxMap)+1)*
          sizeof(*info.scale));
        range=GetQuantumRange(info.depth);
        for (i=0; i <= (ssize_t) GetQuantumRange(info.depth); i++)
          info.scale[i]=ScaleAnyToQuantum((size_t) i,range);
      }
    if (image->compression == RLECompression)
      {
        unsigned int
          tag;

        /*
          Read RLE offset table.
        */
        for (i=0; i < (ssize_t) stream_info->remaining; i++)
        {
          int
            c;

          c=ReadBlobByte(image);
          if (c == EOF)
            break;
        }
        tag=((unsigned int) ReadBlobLSBShort(image) << 16) |
          ReadBlobLSBShort(image);
        (void) tag;
        length=(size_t) ReadBlobLSBLong(image);
        if (length > (size_t) GetBlobSize(image))
          ThrowDCMException(CorruptImageError,"InsufficientImageDataInFile")
        stream_info->offset_count=length >> 2;
        if (stream_info->offset_count != 0)
          {
            if (stream_info->offsets != (ssize_t *) NULL)
              stream_info->offsets=(ssize_t *)
                RelinquishMagickMemory(stream_info->offsets);
            stream_info->offsets=(ssize_t *) AcquireQuantumMemory(
              stream_info->offset_count,sizeof(*stream_info->offsets));
            if (stream_info->offsets == (ssize_t *) NULL)
              ThrowDCMException(ResourceLimitError,"MemoryAllocationFailed")
            for (i=0; i < (ssize_t) stream_info->offset_count; i++)
            {
              offset=(MagickOffsetType) ReadBlobLSBSignedLong(image);
              if (offset > (MagickOffsetType) GetBlobSize(image))
                ThrowDCMException(CorruptImageError,
                  "InsufficientImageDataInFile")
              stream_info->offsets[i]=(ssize_t) offset;
              if (EOFBlob(image) != MagickFalse)
                break;
            }
            offset=TellBlob(image)+8;
            for (i=0; i < (ssize_t) stream_info->offset_count; i++)
              stream_info->offsets[i]+=offset;
          }
      }
    for (scene=0; scene < (ssize_t) number_scenes; scene++)
    {
      image->columns=info.width;
      image->rows=info.height;
      image->depth=info.depth;
      status=SetImageExtent(image,image->columns,image->rows,exception);
      if (status == MagickFalse)
        break;
      image->colorspace=RGBColorspace;
      (void) SetImageBackgroundColor(image,exception);
      if ((image->colormap == (PixelInfo *) NULL) &&
          (info.samples_per_pixel == 1))
        {
          int
            index;

          size_t
            one;

          one=1;
          if (colors == 0)
            colors=one << info.depth;
          if (AcquireImageColormap(image,colors,exception) == MagickFalse)
            ThrowDCMException(ResourceLimitError,"MemoryAllocationFailed")
          if (map.red != (int *) NULL)
            for (i=0; i < (ssize_t) colors; i++)
            {
              index=map.red[i];
              if ((info.scale != (Quantum *) NULL) && (index >= 0) &&
                  (index <= (int) info.max_value))
                index=(int) info.scale[index];
              image->colormap[i].red=(MagickRealType) index;
            }
          if (map.green != (int *) NULL)
            for (i=0; i < (ssize_t) colors; i++)
            {
              index=map.green[i];
              if ((info.scale != (Quantum *) NULL) && (index >= 0) &&
                  (index <= (int) info.max_value))
                index=(int) info.scale[index];
              image->colormap[i].green=(MagickRealType) index;
            }
          if (map.blue != (int *) NULL)
            for (i=0; i < (ssize_t) colors; i++)
            {
              index=map.blue[i];
              if ((info.scale != (Quantum *) NULL) && (index >= 0) &&
                  (index <= (int) info.max_value))
                index=(int) info.scale[index];
              image->colormap[i].blue=(MagickRealType) index;
            }
          if (map.gray != (int *) NULL)
            for (i=0; i < (ssize_t) colors; i++)
            {
              index=map.gray[i];
              if ((info.scale != (Quantum *) NULL) && (index >= 0) &&
                  (index <= (int) info.max_value))
                index=(int) info.scale[index];
              image->colormap[i].red=(MagickRealType) index;
              image->colormap[i].green=(MagickRealType) index;
              image->colormap[i].blue=(MagickRealType) index;
            }
        }
      if (image->compression == RLECompression)
        {
          unsigned int
            tag;

          /*
            Read RLE segment table.
          */
          for (i=0; i < (ssize_t) stream_info->remaining; i++)
          {
            int
              c;

            c=ReadBlobByte(image);
            if (c == EOF)
              break;
          }
          tag=((unsigned int) ReadBlobLSBShort(image) << 16) |
            ReadBlobLSBShort(image);
          stream_info->remaining=(size_t) ReadBlobLSBLong(image);
          if ((tag != 0xFFFEE000) || (stream_info->remaining <= 64) ||
              (EOFBlob(image) != MagickFalse))
            {
              if (stream_info->offsets != (ssize_t *) NULL)
                stream_info->offsets=(ssize_t *)
                  RelinquishMagickMemory(stream_info->offsets);
              ThrowDCMException(CorruptImageError,"ImproperImageHeader")
            }
          stream_info->count=0;
          stream_info->segment_count=ReadBlobLSBLong(image);
          for (i=0; i < 15; i++)
            stream_info->segments[i]=(ssize_t) ReadBlobLSBSignedLong(image);
          stream_info->remaining-=64;
          if (stream_info->segment_count > 1)
            {
              info.bytes_per_pixel=1;
              info.depth=8;
              if (stream_info->offset_count > 0)
                (void) SeekBlob(image,(MagickOffsetType)
                  stream_info->offsets[0]+stream_info->segments[0],SEEK_SET);
            }
        }
      if ((info.samples_per_pixel > 1) && (image->interlace == PlaneInterlace))
        {
          Quantum
            *q;

          ssize_t
            x,
            y;

          /*
            Convert Planar RGB DCM Medical image to pixel packets.
          */
          for (i=0; i < (ssize_t) info.samples_per_pixel; i++)
          {
            for (y=0; y < (ssize_t) image->rows; y++)
            {
              q=GetAuthenticPixels(image,0,y,image->columns,1,exception);
              if (q == (Quantum *) NULL)
                break;
              for (x=0; x < (ssize_t) image->columns; x++)
              {
                switch ((int) i)
                {
                  case 0:
                  {
                    SetPixelRed(image,ScaleCharToQuantum((unsigned char)
                      ReadDCMByte(stream_info,image)),q);
                    break;
                  }
                  case 1:
                  {
                    SetPixelGreen(image,ScaleCharToQuantum((unsigned char)
                      ReadDCMByte(stream_info,image)),q);
                    break;
                  }
                  case 2:
                  {
                    SetPixelBlue(image,ScaleCharToQuantum((unsigned char)
                      ReadDCMByte(stream_info,image)),q);
                    break;
                  }
                  case 3:
                  {
                    SetPixelAlpha(image,ScaleCharToQuantum((unsigned char)
                      ReadDCMByte(stream_info,image)),q);
                    break;
                  }
                  default:
                    break;
                }
                q+=GetPixelChannels(image);
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
        }
      else
        {
          const char
            *option;

          /*
            Convert DCM Medical image to pixel packets.
          */
          option=GetImageOption(image_info,"dcm:display-range");
          if (option != (const char *) NULL)
            {
              if (LocaleCompare(option,"reset") == 0)
                info.window_width=0;
            }
          option=GetImageOption(image_info,"dcm:window");
          if (option != (char *) NULL)
            {
              GeometryInfo
                geometry_info;

              MagickStatusType
                flags;

              flags=ParseGeometry(option,&geometry_info);
              if (flags & RhoValue)
                info.window_center=geometry_info.rho;
              if (flags & SigmaValue)
                info.window_width=geometry_info.sigma;
              info.rescale=MagickTrue;
            }
          option=GetImageOption(image_info,"dcm:rescale");
          if (option != (char *) NULL)
            info.rescale=IsStringTrue(option);
          if ((info.window_center != 0) && (info.window_width == 0))
            info.window_width=info.window_center;
          status=ReadDCMPixels(image,&info,stream_info,MagickTrue,exception);
          if ((status != MagickFalse) && (stream_info->segment_count > 1))
            {
              if (stream_info->offset_count > 0)
                (void) SeekBlob(image,(MagickOffsetType)
                  stream_info->offsets[0]+stream_info->segments[1],SEEK_SET);
              (void) ReadDCMPixels(image,&info,stream_info,MagickFalse,
                exception);
            }
        }
      if (IdentifyImageCoderGray(image,exception) != MagickFalse)
        (void) SetImageColorspace(image,GRAYColorspace,exception);
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
      if (scene < (ssize_t) (number_scenes-1))
        {
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
          status=SetImageProgress(image,LoadImagesTag,TellBlob(image),
            GetBlobSize(image));
          if (status == MagickFalse)
            break;
        }
    }
    if (TellBlob(image) < (MagickOffsetType) GetBlobSize(image))
      {
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
        status=SetImageProgress(image,LoadImagesTag,TellBlob(image),
          GetBlobSize(image));
        if (status == MagickFalse)
          break;
     }
  }
  /*
    Free resources.
  */
  RelinquishDCMMemory(&info,&map,stream_info,stack,data);
  if (image == (Image *) NULL)
    return(image);
  (void) CloseBlob(image);
  if (status == MagickFalse)
    return(DestroyImageList(image));
  return(GetFirstImageInList(image));
}