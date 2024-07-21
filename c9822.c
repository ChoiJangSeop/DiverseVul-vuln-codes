static Image *ReadPDFImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  char
    command[MagickPathExtent],
    *density,
    filename[MagickPathExtent],
    input_filename[MagickPathExtent],
    message[MagickPathExtent],
    *options,
    postscript_filename[MagickPathExtent];

  const char
    *option;

  const DelegateInfo
    *delegate_info;

  GeometryInfo
    geometry_info;

  Image
    *image,
    *next,
    *pdf_image;

  ImageInfo
    *read_info;

  int
    file;

  MagickBooleanType
    fitPage,
    status;

  MagickStatusType
    flags;

  PDFInfo
    pdf_info;

  PointInfo
    delta;

  RectangleInfo
    page;

  register ssize_t
    i;

  size_t
    scene;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  /*
    Open image file.
  */
  image=AcquireImage(image_info,exception);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  status=AcquireUniqueSymbolicLink(image_info->filename,input_filename);
  if (status == MagickFalse)
    {
      ThrowFileException(exception,FileOpenError,"UnableToCreateTemporaryFile",
        image_info->filename);
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  /*
    Set the page density.
  */
  delta.x=DefaultResolution;
  delta.y=DefaultResolution;
  if ((image->resolution.x == 0.0) || (image->resolution.y == 0.0))
    {
      flags=ParseGeometry(PSDensityGeometry,&geometry_info);
      image->resolution.x=geometry_info.rho;
      image->resolution.y=geometry_info.sigma;
      if ((flags & SigmaValue) == 0)
        image->resolution.y=image->resolution.x;
    }
  if (image_info->density != (char *) NULL)
    {
      flags=ParseGeometry(image_info->density,&geometry_info);
      image->resolution.x=geometry_info.rho;
      image->resolution.y=geometry_info.sigma;
      if ((flags & SigmaValue) == 0)
        image->resolution.y=image->resolution.x;
    }
  (void) memset(&page,0,sizeof(page));
  (void) ParseAbsoluteGeometry(PSPageGeometry,&page);
  if (image_info->page != (char *) NULL)
    (void) ParseAbsoluteGeometry(image_info->page,&page);
  page.width=(size_t) ((ssize_t) ceil((double) (page.width*
    image->resolution.x/delta.x)-0.5));
  page.height=(size_t) ((ssize_t) ceil((double) (page.height*
    image->resolution.y/delta.y)-0.5));
  /*
    Determine page geometry from the PDF media box.
  */
  ReadPDFInfo(image_info,image,&pdf_info,exception);
  (void) CloseBlob(image);
  /*
    Set PDF render geometry.
  */
  if ((fabs(pdf_info.bounds.x2-pdf_info.bounds.x1) >= MagickEpsilon) &&
      (fabs(pdf_info.bounds.y2-pdf_info.bounds.y1) >= MagickEpsilon))
    {
      (void) FormatImageProperty(image,"pdf:HiResBoundingBox",
        "%gx%g%+.15g%+.15g",pdf_info.bounds.x2-pdf_info.bounds.x1,
        pdf_info.bounds.y2-pdf_info.bounds.y1,pdf_info.bounds.x1,
        pdf_info.bounds.y1);
      page.width=(size_t) ((ssize_t) ceil((double) ((pdf_info.bounds.x2-
        pdf_info.bounds.x1)*image->resolution.x/delta.x)-0.5));
      page.height=(size_t) ((ssize_t) ceil((double) ((pdf_info.bounds.y2-
        pdf_info.bounds.y1)*image->resolution.y/delta.y)-0.5));
    }
  fitPage=MagickFalse;
  option=GetImageOption(image_info,"pdf:fit-page");
  if (option != (char *) NULL)
    {
      char
        *page_geometry;

      page_geometry=GetPageGeometry(option);
      flags=ParseMetaGeometry(page_geometry,&page.x,&page.y,&page.width,
        &page.height);
      page_geometry=DestroyString(page_geometry);
      if (flags == NoValue)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),OptionError,
            "InvalidGeometry","`%s'",option);
          CleanupPDFInfo(&pdf_info);
          image=DestroyImage(image);
          return((Image *) NULL);
        }
      page.width=(size_t) ((ssize_t) ceil((double) (page.width*
        image->resolution.x/delta.x)-0.5));
      page.height=(size_t) ((ssize_t) ceil((double) (page.height*
        image->resolution.y/delta.y)-0.5));
      fitPage=MagickTrue;
    }
  if ((fabs(pdf_info.angle) == 90.0) || (fabs(pdf_info.angle) == 270.0))
    {
      size_t
        swap;

      swap=page.width;
      page.width=page.height;
      page.height=swap;
    }
  if (IssRGBCompatibleColorspace(image_info->colorspace) != MagickFalse)
    pdf_info.cmyk=MagickFalse;
  /*
    Create Ghostscript control file.
  */
  file=AcquireUniqueFileResource(postscript_filename);
  if (file == -1)
    {
      ThrowFileException(exception,FileOpenError,"UnableToCreateTemporaryFile",
        image_info->filename);
      CleanupPDFInfo(&pdf_info);
      image=DestroyImage(image);
      return((Image *) NULL);
    }
  if (write(file," ",1) != 1)
    {
      file=close(file)-1;
      (void) RelinquishUniqueFileResource(postscript_filename);
      CleanupPDFInfo(&pdf_info);
      image=DestroyImage(image);
      return((Image *) NULL);
    }
  file=close(file)-1;
  /*
    Render Postscript with the Ghostscript delegate.
  */
  if (image_info->monochrome != MagickFalse)
    delegate_info=GetDelegateInfo("ps:mono",(char *) NULL,exception);
  else
     if (pdf_info.cmyk != MagickFalse)
       delegate_info=GetDelegateInfo("ps:cmyk",(char *) NULL,exception);
     else
       delegate_info=GetDelegateInfo("ps:alpha",(char *) NULL,exception);
  if (delegate_info == (const DelegateInfo *) NULL)
    {
      (void) RelinquishUniqueFileResource(postscript_filename);
      CleanupPDFInfo(&pdf_info);
      image=DestroyImage(image);
      return((Image *) NULL);
    }
  density=AcquireString("");
  options=AcquireString("");
  (void) FormatLocaleString(density,MagickPathExtent,"%gx%g",
    image->resolution.x,image->resolution.y);
  if ((image_info->page != (char *) NULL) || (fitPage != MagickFalse))
    (void) FormatLocaleString(options,MagickPathExtent,"-g%.20gx%.20g ",(double)
      page.width,(double) page.height);
  if (fitPage != MagickFalse)
    (void) ConcatenateMagickString(options,"-dPSFitPage ",MagickPathExtent);
  if (pdf_info.cropbox != MagickFalse)
    (void) ConcatenateMagickString(options,"-dUseCropBox ",MagickPathExtent);
  if (pdf_info.trimbox != MagickFalse)
    (void) ConcatenateMagickString(options,"-dUseTrimBox ",MagickPathExtent);
  option=GetImageOption(image_info,"pdf:stop-on-error");
  if (IsStringTrue(option) != MagickFalse)
    (void) ConcatenateMagickString(options,"-dPDFSTOPONERROR ",
      MagickPathExtent);
  option=GetImageOption(image_info,"pdf:interpolate");
  if (IsStringTrue(option) != MagickFalse)
    (void) ConcatenateMagickString(options,"-dInterpolateControl=-1 ",
      MagickPathExtent);
  option=GetImageOption(image_info,"authenticate");
  if ((option != (char *) NULL) && (strpbrk(option,"&;<>|") == (char *) NULL))
    {
      char
        passphrase[MagickPathExtent];

      (void) FormatLocaleString(passphrase,MagickPathExtent,
        "\"-sPDFPassword=%s\" ",option);
      (void) ConcatenateMagickString(options,passphrase,MagickPathExtent);
    }
  read_info=CloneImageInfo(image_info);
  *read_info->magick='\0';
  if (read_info->number_scenes != 0)
    {
      char
        pages[MagickPathExtent];

      (void) FormatLocaleString(pages,MagickPathExtent,"-dFirstPage=%.20g "
        "-dLastPage=%.20g",(double) read_info->scene+1,(double)
        (read_info->scene+read_info->number_scenes));
      (void) ConcatenateMagickString(options,pages,MagickPathExtent);
      read_info->number_scenes=0;
      if (read_info->scenes != (char *) NULL)
        *read_info->scenes='\0';
    }
  (void) CopyMagickString(filename,read_info->filename,MagickPathExtent);
  (void) AcquireUniqueFilename(filename);
  (void) RelinquishUniqueFileResource(filename);
  (void) ConcatenateMagickString(filename,"%d",MagickPathExtent);
  (void) FormatLocaleString(command,MagickPathExtent,
    GetDelegateCommands(delegate_info),
    read_info->antialias != MagickFalse ? 4 : 1,
    read_info->antialias != MagickFalse ? 4 : 1,density,options,filename,
    postscript_filename,input_filename);
  options=DestroyString(options);
  density=DestroyString(density);
  *message='\0';
  status=InvokeGhostscriptDelegate(read_info->verbose,command,message,
    exception);
  (void) RelinquishUniqueFileResource(postscript_filename);
  (void) RelinquishUniqueFileResource(input_filename);
  pdf_image=(Image *) NULL;
  if (status == MagickFalse)
    for (i=1; ; i++)
    {
      (void) InterpretImageFilename(image_info,image,filename,(int) i,
        read_info->filename,exception);
      if (IsGhostscriptRendered(read_info->filename) == MagickFalse)
        break;
      (void) RelinquishUniqueFileResource(read_info->filename);
    }
  else
    for (i=1; ; i++)
    {
      (void) InterpretImageFilename(image_info,image,filename,(int) i,
        read_info->filename,exception);
      if (IsGhostscriptRendered(read_info->filename) == MagickFalse)
        break;
      read_info->blob=NULL;
      read_info->length=0;
      next=ReadImage(read_info,exception);
      (void) RelinquishUniqueFileResource(read_info->filename);
      if (next == (Image *) NULL)
        break;
      AppendImageToList(&pdf_image,next);
    }
  read_info=DestroyImageInfo(read_info);
  if (pdf_image == (Image *) NULL)
    {
      if (*message != '\0')
        (void) ThrowMagickException(exception,GetMagickModule(),DelegateError,
          "PDFDelegateFailed","`%s'",message);
      CleanupPDFInfo(&pdf_info);
      image=DestroyImage(image);
      return((Image *) NULL);
    }
  if (LocaleCompare(pdf_image->magick,"BMP") == 0)
    {
      Image
        *cmyk_image;

      cmyk_image=ConsolidateCMYKImages(pdf_image,exception);
      if (cmyk_image != (Image *) NULL)
        {
          pdf_image=DestroyImageList(pdf_image);
          pdf_image=cmyk_image;
        }
    }
  if (pdf_info.profile != (StringInfo *) NULL)
    {
      char
        *profile;

      (void) SetImageProfile(image,"xmp",pdf_info.profile,exception);
      profile=(char *) GetStringInfoDatum(pdf_info.profile);
      if (strstr(profile,"Adobe Illustrator") != (char *) NULL)
        (void) CopyMagickString(image->magick,"AI",MagickPathExtent);
    }
  CleanupPDFInfo(&pdf_info);
  if (image_info->number_scenes != 0)
    {
      Image
        *clone_image;

      /*
        Add place holder images to meet the subimage specification requirement.
      */
      for (i=0; i < (ssize_t) image_info->scene; i++)
      {
        clone_image=CloneImage(pdf_image,1,1,MagickTrue,exception);
        if (clone_image != (Image *) NULL)
          PrependImageToList(&pdf_image,clone_image);
      }
    }
  do
  {
    (void) CopyMagickString(pdf_image->filename,filename,MagickPathExtent);
    (void) CopyMagickString(pdf_image->magick,image->magick,MagickPathExtent);
    pdf_image->page=page;
    (void) CloneImageProfiles(pdf_image,image);
    (void) CloneImageProperties(pdf_image,image);
    next=SyncNextImageInList(pdf_image);
    if (next != (Image *) NULL)
      pdf_image=next;
  } while (next != (Image *) NULL);
  image=DestroyImage(image);
  scene=0;
  for (next=GetFirstImageInList(pdf_image); next != (Image *) NULL; )
  {
    next->scene=scene++;
    next=GetNextImageInList(next);
  }
  return(GetFirstImageInList(pdf_image));
}