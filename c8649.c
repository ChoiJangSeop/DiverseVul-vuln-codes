static MagickBooleanType WritePS2Image(const ImageInfo *image_info,Image *image,
  ExceptionInfo *exception)
{
  static const char
    *const PostscriptProlog[]=
    {
      "%%%%BeginProlog",
      "%%",
      "%% Display a color image.  The image is displayed in color on",
      "%% Postscript viewers or printers that support color, otherwise",
      "%% it is displayed as grayscale.",
      "%%",
      "/DirectClassImage",
      "{",
      "  %%",
      "  %% Display a DirectClass image.",
      "  %%",
      "  colorspace 0 eq",
      "  {",
      "    /DeviceRGB setcolorspace",
      "    <<",
      "      /ImageType 1",
      "      /Width columns",
      "      /Height rows",
      "      /BitsPerComponent 8",
      "      /Decode [0 1 0 1 0 1]",
      "      /ImageMatrix [columns 0 0 rows neg 0 rows]",
      "      compression 0 gt",
      "      { /DataSource pixel_stream %s }",
      "      { /DataSource pixel_stream %s } ifelse",
      "    >> image",
      "  }",
      "  {",
      "    /DeviceCMYK setcolorspace",
      "    <<",
      "      /ImageType 1",
      "      /Width columns",
      "      /Height rows",
      "      /BitsPerComponent 8",
      "      /Decode [1 0 1 0 1 0 1 0]",
      "      /ImageMatrix [columns 0 0 rows neg 0 rows]",
      "      compression 0 gt",
      "      { /DataSource pixel_stream %s }",
      "      { /DataSource pixel_stream %s } ifelse",
      "    >> image",
      "  } ifelse",
      "} bind def",
      "",
      "/PseudoClassImage",
      "{",
      "  %%",
      "  %% Display a PseudoClass image.",
      "  %%",
      "  %% Parameters:",
      "  %%   colors: number of colors in the colormap.",
      "  %%",
      "  currentfile buffer readline pop",
      "  token pop /colors exch def pop",
      "  colors 0 eq",
      "  {",
      "    %%",
      "    %% Image is grayscale.",
      "    %%",
      "    currentfile buffer readline pop",
      "    token pop /bits exch def pop",
      "    /DeviceGray setcolorspace",
      "    <<",
      "      /ImageType 1",
      "      /Width columns",
      "      /Height rows",
      "      /BitsPerComponent bits",
      "      /Decode [0 1]",
      "      /ImageMatrix [columns 0 0 rows neg 0 rows]",
      "      compression 0 gt",
      "      { /DataSource pixel_stream %s }",
      "      {",
      "        /DataSource pixel_stream %s",
      "        <<",
      "           /K " CCITTParam,
      "           /Columns columns",
      "           /Rows rows",
      "        >> /CCITTFaxDecode filter",
      "      } ifelse",
      "    >> image",
      "  }",
      "  {",
      "    %%",
      "    %% Parameters:",
      "    %%   colormap: red, green, blue color packets.",
      "    %%",
      "    /colormap colors 3 mul string def",
      "    currentfile colormap readhexstring pop pop",
      "    currentfile buffer readline pop",
      "    [ /Indexed /DeviceRGB colors 1 sub colormap ] setcolorspace",
      "    <<",
      "      /ImageType 1",
      "      /Width columns",
      "      /Height rows",
      "      /BitsPerComponent 8",
      "      /Decode [0 255]",
      "      /ImageMatrix [columns 0 0 rows neg 0 rows]",
      "      compression 0 gt",
      "      { /DataSource pixel_stream %s }",
      "      { /DataSource pixel_stream %s } ifelse",
      "    >> image",
      "  } ifelse",
      "} bind def",
      "",
      "/DisplayImage",
      "{",
      "  %%",
      "  %% Display a DirectClass or PseudoClass image.",
      "  %%",
      "  %% Parameters:",
      "  %%   x & y translation.",
      "  %%   x & y scale.",
      "  %%   label pointsize.",
      "  %%   image label.",
      "  %%   image columns & rows.",
      "  %%   class: 0-DirectClass or 1-PseudoClass.",
      "  %%   colorspace: 0-RGB or 1-CMYK.",
      "  %%   compression: 0-RLECompression or 1-NoCompression.",
      "  %%   hex color packets.",
      "  %%",
      "  gsave",
      "  /buffer 512 string def",
      "  /pixel_stream currentfile def",
      "",
      "  currentfile buffer readline pop",
      "  token pop /x exch def",
      "  token pop /y exch def pop",
      "  x y translate",
      "  currentfile buffer readline pop",
      "  token pop /x exch def",
      "  token pop /y exch def pop",
      "  currentfile buffer readline pop",
      "  token pop /pointsize exch def pop",
      (const char *) NULL
    },
    *const PostscriptEpilog[]=
    {
      "  x y scale",
      "  currentfile buffer readline pop",
      "  token pop /columns exch def",
      "  token pop /rows exch def pop",
      "  currentfile buffer readline pop",
      "  token pop /class exch def pop",
      "  currentfile buffer readline pop",
      "  token pop /colorspace exch def pop",
      "  currentfile buffer readline pop",
      "  token pop /compression exch def pop",
      "  class 0 gt { PseudoClassImage } { DirectClassImage } ifelse",
      "  grestore",
      (const char *) NULL
    };

  char
    buffer[MagickPathExtent],
    date[MagickPathExtent],
    page_geometry[MagickPathExtent],
    **labels;

  CompressionType
    compression;

  const char
    *const *q,
    *value;

  double
    pointsize;

  GeometryInfo
    geometry_info;

  MagickOffsetType
    scene,
    start,
    stop;

  MagickBooleanType
    progress,
    status;

  MagickOffsetType
    offset;

  MagickSizeType
    number_pixels;

  MagickStatusType
    flags;

  PointInfo
    delta,
    resolution,
    scale;

  RectangleInfo
    geometry,
    media_info,
    page_info;

  register const Quantum
    *p;

  register ssize_t
    x;

  register ssize_t
    i;

  SegmentInfo
    bounds;

  size_t
    imageListLength,
    length,
    page,
    text_size;

  ssize_t
    j,
    y;

  time_t
    timer;

  unsigned char
    *pixels;

  /*
    Open output image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,exception);
  if (status == MagickFalse)
    return(status);
  compression=image->compression;
  if (image_info->compression != UndefinedCompression)
    compression=image_info->compression;
  switch (compression)
  {
#if !defined(MAGICKCORE_JPEG_DELEGATE)
    case JPEGCompression:
    {
      compression=RLECompression;
      (void) ThrowMagickException(exception,GetMagickModule(),
        MissingDelegateError,"DelegateLibrarySupportNotBuiltIn","`%s' (JPEG)",
        image->filename);
      break;
    }
#endif
    default:
      break;
  }
  (void) memset(&bounds,0,sizeof(bounds));
  page=1;
  scene=0;
  imageListLength=GetImageListLength(image);
  do
  {
    /*
      Scale relative to dots-per-inch.
    */
    delta.x=DefaultResolution;
    delta.y=DefaultResolution;
    resolution.x=image->resolution.x;
    resolution.y=image->resolution.y;
    if ((resolution.x == 0.0) || (resolution.y == 0.0))
      {
        flags=ParseGeometry(PSDensityGeometry,&geometry_info);
        resolution.x=geometry_info.rho;
        resolution.y=geometry_info.sigma;
        if ((flags & SigmaValue) == 0)
          resolution.y=resolution.x;
      }
    if (image_info->density != (char *) NULL)
      {
        flags=ParseGeometry(image_info->density,&geometry_info);
        resolution.x=geometry_info.rho;
        resolution.y=geometry_info.sigma;
        if ((flags & SigmaValue) == 0)
          resolution.y=resolution.x;
      }
    if (image->units == PixelsPerCentimeterResolution)
      {
        resolution.x=(size_t) (100.0*2.54*resolution.x+0.5)/100.0;
        resolution.y=(size_t) (100.0*2.54*resolution.y+0.5)/100.0;
      }
    SetGeometry(image,&geometry);
    (void) FormatLocaleString(page_geometry,MagickPathExtent,"%.20gx%.20g",
      (double) image->columns,(double) image->rows);
    if (image_info->page != (char *) NULL)
      (void) CopyMagickString(page_geometry,image_info->page,MagickPathExtent);
    else
      if ((image->page.width != 0) && (image->page.height != 0))
        (void) FormatLocaleString(page_geometry,MagickPathExtent,
          "%.20gx%.20g%+.20g%+.20g",(double) image->page.width,(double)
          image->page.height,(double) image->page.x,(double) image->page.y);
      else
        if ((image->gravity != UndefinedGravity) &&
            (LocaleCompare(image_info->magick,"PS") == 0))
          (void) CopyMagickString(page_geometry,PSPageGeometry,MagickPathExtent);
    (void) ConcatenateMagickString(page_geometry,">",MagickPathExtent);
    (void) ParseMetaGeometry(page_geometry,&geometry.x,&geometry.y,
      &geometry.width,&geometry.height);
    scale.x=(double) (geometry.width*delta.x)/resolution.x;
    geometry.width=(size_t) floor(scale.x+0.5);
    scale.y=(double) (geometry.height*delta.y)/resolution.y;
    geometry.height=(size_t) floor(scale.y+0.5);
    (void) ParseAbsoluteGeometry(page_geometry,&media_info);
    (void) ParseGravityGeometry(image,page_geometry,&page_info,exception);
    if (image->gravity != UndefinedGravity)
      {
        geometry.x=(-page_info.x);
        geometry.y=(ssize_t) (media_info.height+page_info.y-image->rows);
      }
    pointsize=12.0;
    if (image_info->pointsize != 0.0)
      pointsize=image_info->pointsize;
    text_size=0;
    value=GetImageProperty(image,"label",exception);
    if (value != (const char *) NULL)
      text_size=(size_t) (MultilineCensus(value)*pointsize+12);
    if (page == 1)
      {
        /*
          Output Postscript header.
        */
        if (LocaleCompare(image_info->magick,"PS2") == 0)
          (void) CopyMagickString(buffer,"%!PS-Adobe-3.0\n",MagickPathExtent);
        else
          (void) CopyMagickString(buffer,"%!PS-Adobe-3.0 EPSF-3.0\n",
            MagickPathExtent);
        (void) WriteBlobString(image,buffer);
        (void) WriteBlobString(image,"%%Creator: (ImageMagick)\n");
        (void) FormatLocaleString(buffer,MagickPathExtent,"%%%%Title: (%s)\n",
          image->filename);
        (void) WriteBlobString(image,buffer);
        timer=time((time_t *) NULL);
        (void) FormatMagickTime(timer,MagickPathExtent,date);
        (void) FormatLocaleString(buffer,MagickPathExtent,
          "%%%%CreationDate: (%s)\n",date);
        (void) WriteBlobString(image,buffer);
        bounds.x1=(double) geometry.x;
        bounds.y1=(double) geometry.y;
        bounds.x2=(double) geometry.x+geometry.width;
        bounds.y2=(double) geometry.y+geometry.height+text_size;
        if ((image_info->adjoin != MagickFalse) &&
            (GetNextImageInList(image) != (Image *) NULL))
          (void) CopyMagickString(buffer,"%%BoundingBox: (atend)\n",
            MagickPathExtent);
        else
          {
            (void) FormatLocaleString(buffer,MagickPathExtent,
              "%%%%BoundingBox: %.20g %.20g %.20g %.20g\n",ceil(bounds.x1-0.5),
              ceil(bounds.y1-0.5),floor(bounds.x2+0.5),floor(bounds.y2+0.5));
            (void) WriteBlobString(image,buffer);
            (void) FormatLocaleString(buffer,MagickPathExtent,
              "%%%%HiResBoundingBox: %g %g %g %g\n",bounds.x1,
              bounds.y1,bounds.x2,bounds.y2);
          }
        (void) WriteBlobString(image,buffer);
        value=GetImageProperty(image,"label",exception);
        if (value != (const char *) NULL)
          (void) WriteBlobString(image,
            "%%DocumentNeededResources: font Helvetica\n");
        (void) WriteBlobString(image,"%%LanguageLevel: 2\n");
        if (LocaleCompare(image_info->magick,"PS2") != 0)
          (void) WriteBlobString(image,"%%Pages: 1\n");
        else
          {
            (void) WriteBlobString(image,"%%Orientation: Portrait\n");
            (void) WriteBlobString(image,"%%PageOrder: Ascend\n");
            if (image_info->adjoin == MagickFalse)
              (void) CopyMagickString(buffer,"%%Pages: 1\n",MagickPathExtent);
            else
              (void) FormatLocaleString(buffer,MagickPathExtent,
                "%%%%Pages: %.20g\n",(double) imageListLength);
            (void) WriteBlobString(image,buffer);
          }
        if (image->colorspace == CMYKColorspace)
          (void) WriteBlobString(image,
            "%%DocumentProcessColors: Cyan Magenta Yellow Black\n");
        (void) WriteBlobString(image,"%%EndComments\n");
        (void) WriteBlobString(image,"\n%%BeginDefaults\n");
        (void) WriteBlobString(image,"%%EndDefaults\n\n");
        /*
          Output Postscript commands.
        */
        for (q=PostscriptProlog; *q; q++)
        {
          switch (compression)
          {
            case NoCompression:
            {
              (void) FormatLocaleString(buffer,MagickPathExtent,*q,
                "/ASCII85Decode filter");
              break;
            }
            case JPEGCompression:
            {
              (void) FormatLocaleString(buffer,MagickPathExtent,*q,
                "/DCTDecode filter");
              break;
            }
            case LZWCompression:
            {
              (void) FormatLocaleString(buffer,MagickPathExtent,*q,
                "/LZWDecode filter");
              break;
            }
            case FaxCompression:
            case Group4Compression:
            {
              (void) FormatLocaleString(buffer,MagickPathExtent,*q," ");
              break;
            }
            default:
            {
              (void) FormatLocaleString(buffer,MagickPathExtent,*q,
                "/RunLengthDecode filter");
              break;
            }
          }
          (void) WriteBlobString(image,buffer);
          (void) WriteBlobByte(image,'\n');
        }
        value=GetImageProperty(image,"label",exception);
        if (value != (const char *) NULL)
          {
            (void) WriteBlobString(image,
              "  /Helvetica findfont pointsize scalefont setfont\n");
            for (j=(ssize_t) MultilineCensus(value)-1; j >= 0; j--)
            {
              (void) WriteBlobString(image,"  /label 512 string def\n");
              (void) WriteBlobString(image,
                "  currentfile label readline pop\n");
              (void) FormatLocaleString(buffer,MagickPathExtent,
                "  0 y %g add moveto label show pop\n",j*pointsize+12);
              (void) WriteBlobString(image,buffer);
            }
          }
        for (q=PostscriptEpilog; *q; q++)
        {
          (void) FormatLocaleString(buffer,MagickPathExtent,"%s\n",*q);
          (void) WriteBlobString(image,buffer);
        }
        if (LocaleCompare(image_info->magick,"PS2") == 0)
          (void) WriteBlobString(image,"  showpage\n");
        (void) WriteBlobString(image,"} bind def\n");
        (void) WriteBlobString(image,"%%EndProlog\n");
      }
    (void) FormatLocaleString(buffer,MagickPathExtent,"%%%%Page:  1 %.20g\n",
      (double) page++);
    (void) WriteBlobString(image,buffer);
    (void) FormatLocaleString(buffer,MagickPathExtent,
      "%%%%PageBoundingBox: %.20g %.20g %.20g %.20g\n",(double) geometry.x,
      (double) geometry.y,geometry.x+(double) geometry.width,geometry.y+(double)
      (geometry.height+text_size));
    (void) WriteBlobString(image,buffer);
    if ((double) geometry.x < bounds.x1)
      bounds.x1=(double) geometry.x;
    if ((double) geometry.y < bounds.y1)
      bounds.y1=(double) geometry.y;
    if ((double) (geometry.x+geometry.width-1) > bounds.x2)
      bounds.x2=(double) geometry.x+geometry.width-1;
    if ((double) (geometry.y+(geometry.height+text_size)-1) > bounds.y2)
      bounds.y2=(double) geometry.y+(geometry.height+text_size)-1;
    value=GetImageProperty(image,"label",exception);
    if (value != (const char *) NULL)
      (void) WriteBlobString(image,"%%PageResources: font Helvetica\n");
    if (LocaleCompare(image_info->magick,"PS2") != 0)
      (void) WriteBlobString(image,"userdict begin\n");
    start=TellBlob(image);
    (void) FormatLocaleString(buffer,MagickPathExtent,
      "%%%%BeginData:%13ld %s Bytes\n",0L,
      compression == NoCompression ? "ASCII" : "Binary");
    (void) WriteBlobString(image,buffer);
    stop=TellBlob(image);
    (void) WriteBlobString(image,"DisplayImage\n");
    /*
      Output image data.
    */
    (void) FormatLocaleString(buffer,MagickPathExtent,
      "%.20g %.20g\n%g %g\n%g\n",(double) geometry.x,(double) geometry.y,
      scale.x,scale.y,pointsize);
    (void) WriteBlobString(image,buffer);
    labels=(char **) NULL;
    value=GetImageProperty(image,"label",exception);
    if (value != (const char *) NULL)
      labels=StringToList(value);
    if (labels != (char **) NULL)
      {
        for (i=0; labels[i] != (char *) NULL; i++)
        {
          (void) FormatLocaleString(buffer,MagickPathExtent,"%s \n",
            labels[i]);
          (void) WriteBlobString(image,buffer);
          labels[i]=DestroyString(labels[i]);
        }
        labels=(char **) RelinquishMagickMemory(labels);
      }
    number_pixels=(MagickSizeType) image->columns*image->rows;
    if (number_pixels != (MagickSizeType) ((size_t) number_pixels))
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
    if ((compression == FaxCompression) || (compression == Group4Compression) ||
        ((image_info->type != TrueColorType) &&
         (SetImageGray(image,exception) != MagickFalse)))
      {
        (void) FormatLocaleString(buffer,MagickPathExtent,
          "%.20g %.20g\n1\n%d\n",(double) image->columns,(double) image->rows,
          (int) (image->colorspace == CMYKColorspace));
        (void) WriteBlobString(image,buffer);
        (void) FormatLocaleString(buffer,MagickPathExtent,"%d\n",(int)
          ((compression != FaxCompression) &&
           (compression != Group4Compression)));
        (void) WriteBlobString(image,buffer);
        (void) WriteBlobString(image,"0\n");
        (void) FormatLocaleString(buffer,MagickPathExtent,"%d\n",
           (compression == FaxCompression) ||
           (compression == Group4Compression) ? 1 : 8);
        (void) WriteBlobString(image,buffer);
        switch (compression)
        {
          case FaxCompression:
          case Group4Compression:
          {
            if (LocaleCompare(CCITTParam,"0") == 0)
              {
                (void) HuffmanEncodeImage(image_info,image,image,exception);
                break;
              }
            (void) Huffman2DEncodeImage(image_info,image,image,exception);
            break;
          }
          case JPEGCompression:
          {
            status=InjectImageBlob(image_info,image,image,"jpeg",exception);
            if (status == MagickFalse)
              {
                (void) CloseBlob(image);
                return(MagickFalse);
              }
            break;
          }
          case RLECompression:
          default:
          {
            MemoryInfo
              *pixel_info;

            register unsigned char
              *q;

            /*
              Allocate pixel array.
            */
            length=(size_t) number_pixels;
            pixel_info=AcquireVirtualMemory(length,sizeof(*pixels));
            if (pixel_info == (MemoryInfo *) NULL)
              ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
            pixels=(unsigned char *) GetVirtualMemoryBlob(pixel_info);
            /*
              Dump runlength encoded pixels.
            */
            q=pixels;
            for (y=0; y < (ssize_t) image->rows; y++)
            {
              p=GetVirtualPixels(image,0,y,image->columns,1,exception);
              if (p == (const Quantum *) NULL)
                break;
              for (x=0; x < (ssize_t) image->columns; x++)
              {
                *q++=ScaleQuantumToChar(ClampToQuantum(GetPixelLuma(image,p)));
                p+=GetPixelChannels(image);
              }
              progress=SetImageProgress(image,SaveImageTag,(MagickOffsetType) y,
                image->rows);
              if (progress == MagickFalse)
                break;
            }
            length=(size_t) (q-pixels);
            if (compression == LZWCompression)
              status=LZWEncodeImage(image,length,pixels,exception);
            else
              status=PackbitsEncodeImage(image,length,pixels,exception);
            pixel_info=RelinquishVirtualMemory(pixel_info);
            if (status == MagickFalse)
              {
                (void) CloseBlob(image);
                return(MagickFalse);
              }
            break;
          }
          case NoCompression:
          {
            /*
              Dump uncompressed PseudoColor packets.
            */
            Ascii85Initialize(image);
            for (y=0; y < (ssize_t) image->rows; y++)
            {
              p=GetVirtualPixels(image,0,y,image->columns,1,exception);
              if (p == (const Quantum *) NULL)
                break;
              for (x=0; x < (ssize_t) image->columns; x++)
              {
                Ascii85Encode(image,ScaleQuantumToChar(ClampToQuantum(
                  GetPixelLuma(image,p))));
                p+=GetPixelChannels(image);
              }
              progress=SetImageProgress(image,SaveImageTag,(MagickOffsetType)
                y,image->rows);
              if (progress == MagickFalse)
                break;
            }
            Ascii85Flush(image);
            break;
          }
        }
      }
    else
      if ((image->storage_class == DirectClass) || (image->colors > 256) ||
          (compression == JPEGCompression) || (image->alpha_trait != UndefinedPixelTrait))
        {
          (void) FormatLocaleString(buffer,MagickPathExtent,
            "%.20g %.20g\n0\n%d\n",(double) image->columns,(double) image->rows,
            (int) (image->colorspace == CMYKColorspace));
          (void) WriteBlobString(image,buffer);
          (void) FormatLocaleString(buffer,MagickPathExtent,"%d\n",
            (int) (compression == NoCompression));
          (void) WriteBlobString(image,buffer);
          switch (compression)
          {
            case JPEGCompression:
            {
              status=InjectImageBlob(image_info,image,image,"jpeg",exception);
              if (status == MagickFalse)
                {
                  (void) CloseBlob(image);
                  return(MagickFalse);
                }
              break;
            }
            case RLECompression:
            default:
            {
              MemoryInfo
                *pixel_info;

              register unsigned char
                *q;

              /*
                Allocate pixel array.
              */
              length=(size_t) number_pixels;
              pixel_info=AcquireVirtualMemory(length,4*sizeof(*pixels));
              if (pixel_info == (MemoryInfo *) NULL)
                ThrowWriterException(ResourceLimitError,
                  "MemoryAllocationFailed");
              pixels=(unsigned char *) GetVirtualMemoryBlob(pixel_info);
              /*
                Dump runlength encoded pixels.
              */
              q=pixels;
              for (y=0; y < (ssize_t) image->rows; y++)
              {
                p=GetVirtualPixels(image,0,y,image->columns,1,exception);
                if (p == (const Quantum *) NULL)
                  break;
                for (x=0; x < (ssize_t) image->columns; x++)
                {
                  if ((image->alpha_trait != UndefinedPixelTrait) &&
                      (GetPixelAlpha(image,p) == (Quantum) TransparentAlpha))
                    {
                      *q++=ScaleQuantumToChar(QuantumRange);
                      *q++=ScaleQuantumToChar(QuantumRange);
                      *q++=ScaleQuantumToChar(QuantumRange);
                    }
                  else
                    if (image->colorspace != CMYKColorspace)
                      {
                        *q++=ScaleQuantumToChar(GetPixelRed(image,p));
                        *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
                        *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
                      }
                    else
                      {
                        *q++=ScaleQuantumToChar(GetPixelRed(image,p));
                        *q++=ScaleQuantumToChar(GetPixelGreen(image,p));
                        *q++=ScaleQuantumToChar(GetPixelBlue(image,p));
                        *q++=ScaleQuantumToChar(GetPixelBlack(image,p));
                      }
                  p+=GetPixelChannels(image);
                }
                progress=SetImageProgress(image,SaveImageTag,(MagickOffsetType)
                  y,image->rows);
                if (progress == MagickFalse)
                  break;
              }
              length=(size_t) (q-pixels);
              if (compression == LZWCompression)
                status=LZWEncodeImage(image,length,pixels,exception);
              else
                status=PackbitsEncodeImage(image,length,pixels,exception);
              if (status == MagickFalse)
                {
                  (void) CloseBlob(image);
                  return(MagickFalse);
                }
              pixel_info=RelinquishVirtualMemory(pixel_info);
              break;
            }
            case NoCompression:
            {
              /*
                Dump uncompressed DirectColor packets.
              */
              Ascii85Initialize(image);
              for (y=0; y < (ssize_t) image->rows; y++)
              {
                p=GetVirtualPixels(image,0,y,image->columns,1,exception);
                if (p == (const Quantum *) NULL)
                  break;
                for (x=0; x < (ssize_t) image->columns; x++)
                {
                  if ((image->alpha_trait != UndefinedPixelTrait) &&
                      (GetPixelAlpha(image,p) == (Quantum) TransparentAlpha))
                    {
                      Ascii85Encode(image,ScaleQuantumToChar((Quantum)
                        QuantumRange));
                      Ascii85Encode(image,ScaleQuantumToChar((Quantum)
                        QuantumRange));
                      Ascii85Encode(image,ScaleQuantumToChar((Quantum)
                        QuantumRange));
                    }
                  else
                    if (image->colorspace != CMYKColorspace)
                      {
                        Ascii85Encode(image,ScaleQuantumToChar(
                          GetPixelRed(image,p)));
                        Ascii85Encode(image,ScaleQuantumToChar(
                          GetPixelGreen(image,p)));
                        Ascii85Encode(image,ScaleQuantumToChar(
                          GetPixelBlue(image,p)));
                      }
                    else
                      {
                        Ascii85Encode(image,ScaleQuantumToChar(
                          GetPixelRed(image,p)));
                        Ascii85Encode(image,ScaleQuantumToChar(
                          GetPixelGreen(image,p)));
                        Ascii85Encode(image,ScaleQuantumToChar(
                          GetPixelBlue(image,p)));
                        Ascii85Encode(image,ScaleQuantumToChar(
                          GetPixelBlack(image,p)));
                      }
                  p+=GetPixelChannels(image);
                }
                progress=SetImageProgress(image,SaveImageTag,(MagickOffsetType)
                  y,image->rows);
                if (progress == MagickFalse)
                  break;
              }
              Ascii85Flush(image);
              break;
            }
          }
        }
      else
        {
          /*
            Dump number of colors and colormap.
          */
          (void) FormatLocaleString(buffer,MagickPathExtent,
            "%.20g %.20g\n1\n%d\n",(double) image->columns,(double) image->rows,
            (int) (image->colorspace == CMYKColorspace));
          (void) WriteBlobString(image,buffer);
          (void) FormatLocaleString(buffer,MagickPathExtent,"%d\n",
            (int) (compression == NoCompression));
          (void) WriteBlobString(image,buffer);
          (void) FormatLocaleString(buffer,MagickPathExtent,"%.20g\n",(double)
            image->colors);
          (void) WriteBlobString(image,buffer);
          for (i=0; i < (ssize_t) image->colors; i++)
          {
            (void) FormatLocaleString(buffer,MagickPathExtent,"%02X%02X%02X\n",
              ScaleQuantumToChar(image->colormap[i].red),
              ScaleQuantumToChar(image->colormap[i].green),
              ScaleQuantumToChar(image->colormap[i].blue));
            (void) WriteBlobString(image,buffer);
          }
          switch (compression)
          {
            case RLECompression:
            default:
            {
              MemoryInfo
                *pixel_info;

              register unsigned char
                *q;

              /*
                Allocate pixel array.
              */
              length=(size_t) number_pixels;
              pixel_info=AcquireVirtualMemory(length,sizeof(*pixels));
              if (pixel_info == (MemoryInfo *) NULL)
                ThrowWriterException(ResourceLimitError,
                  "MemoryAllocationFailed");
              pixels=(unsigned char *) GetVirtualMemoryBlob(pixel_info);
              /*
                Dump runlength encoded pixels.
              */
              q=pixels;
              for (y=0; y < (ssize_t) image->rows; y++)
              {
                p=GetVirtualPixels(image,0,y,image->columns,1,exception);
                if (p == (const Quantum *) NULL)
                  break;
                for (x=0; x < (ssize_t) image->columns; x++)
                {
                  *q++=(unsigned char) GetPixelIndex(image,p);
                  p+=GetPixelChannels(image);
                }
                progress=SetImageProgress(image,SaveImageTag,(MagickOffsetType)
                  y,image->rows);
                if (progress == MagickFalse)
                  break;
              }
              length=(size_t) (q-pixels);
              if (compression == LZWCompression)
                status=LZWEncodeImage(image,length,pixels,exception);
              else
                status=PackbitsEncodeImage(image,length,pixels,exception);
              pixel_info=RelinquishVirtualMemory(pixel_info);
              if (status == MagickFalse)
                {
                  (void) CloseBlob(image);
                  return(MagickFalse);
                }
              break;
            }
            case NoCompression:
            {
              /*
                Dump uncompressed PseudoColor packets.
              */
              Ascii85Initialize(image);
              for (y=0; y < (ssize_t) image->rows; y++)
              {
                p=GetVirtualPixels(image,0,y,image->columns,1,exception);
                if (p == (const Quantum *) NULL)
                  break;
                for (x=0; x < (ssize_t) image->columns; x++)
                {
                  Ascii85Encode(image,(unsigned char) GetPixelIndex(image,p));
                  p+=GetPixelChannels(image);
                }
                progress=SetImageProgress(image,SaveImageTag,(MagickOffsetType)
                  y,image->rows);
                if (progress == MagickFalse)
                  break;
              }
              Ascii85Flush(image);
              break;
            }
          }
        }
    (void) WriteBlobByte(image,'\n');
    length=(size_t) (TellBlob(image)-stop);
    stop=TellBlob(image);
    offset=SeekBlob(image,start,SEEK_SET);
    if (offset < 0)
      ThrowWriterException(CorruptImageError,"ImproperImageHeader");
    (void) FormatLocaleString(buffer,MagickPathExtent,
      "%%%%BeginData:%13ld %s Bytes\n",(long) length,
      compression == NoCompression ? "ASCII" : "Binary");
    (void) WriteBlobString(image,buffer);
    offset=SeekBlob(image,stop,SEEK_SET);
    (void) WriteBlobString(image,"%%EndData\n");
    if (LocaleCompare(image_info->magick,"PS2") != 0)
      (void) WriteBlobString(image,"end\n");
    (void) WriteBlobString(image,"%%PageTrailer\n");
    if (GetNextImageInList(image) == (Image *) NULL)
      break;
    image=SyncNextImageInList(image);
    status=SetImageProgress(image,SaveImagesTag,scene++,imageListLength);
    if (status == MagickFalse)
      break;
  } while (image_info->adjoin != MagickFalse);
  (void) WriteBlobString(image,"%%Trailer\n");
  if (page > 1)
    {
      (void) FormatLocaleString(buffer,MagickPathExtent,
        "%%%%BoundingBox: %.20g %.20g %.20g %.20g\n",ceil(bounds.x1-0.5),
        ceil(bounds.y1-0.5),floor(bounds.x2+0.5),floor(bounds.y2+0.5));
      (void) WriteBlobString(image,buffer);
      (void) FormatLocaleString(buffer,MagickPathExtent,
        "%%%%HiResBoundingBox: %g %g %g %g\n",bounds.x1,bounds.y1,
        bounds.x2,bounds.y2);
      (void) WriteBlobString(image,buffer);
    }
  (void) WriteBlobString(image,"%%EOF\n");
  (void) CloseBlob(image);
  return(MagickTrue);
}