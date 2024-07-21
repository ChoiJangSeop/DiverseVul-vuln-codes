static Image *ReadWMFImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  char
    filename[MagickPathExtent];

  int
    unique_file;

  FILE
    *file;

  Image
    *image;

  ImageInfo
    *read_info;

  MagickBooleanType
    status;

  size_t
    flags;

  wmfAPI
    *wmf_info;

  wmfAPI_Options
    options;

  wmfD_Rect
    bounding_box;

  wmf_eps_t
    *eps_info;

  wmf_error_t
    wmf_status;

  /*
    Read WMF image.
  */
  image=AcquireImage(image_info,exception);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  wmf_info=(wmfAPI *) NULL;
  flags=0;
  flags|=WMF_OPT_IGNORE_NONFATAL;
  flags|=WMF_OPT_FUNCTION;
  options.function=wmf_eps_function;
  wmf_status=wmf_api_create(&wmf_info,(unsigned long) flags,&options);
  if (wmf_status != wmf_E_None)
    {
      if (wmf_info != (wmfAPI *) NULL)
        wmf_api_destroy(wmf_info);
      ThrowReaderException(DelegateError,"UnableToInitializeWMFLibrary");
    }
  wmf_status=wmf_bbuf_input(wmf_info,WMFReadBlob,WMFSeekBlob,WMFTellBlob,
    (void *) image);
  if (wmf_status != wmf_E_None)
    {
      ipa_device_close(wmf_info);
      wmf_api_destroy(wmf_info);
      ThrowFileException(exception,FileOpenError,"UnableToOpenFile",
        image->filename);
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  wmf_status=wmf_scan(wmf_info,0,&bounding_box);
  if (wmf_status != wmf_E_None)
    {
      ipa_device_close(wmf_info);
      wmf_api_destroy(wmf_info);
      ThrowReaderException(DelegateError,"FailedToScanFile");
    }
  eps_info=WMF_EPS_GetData(wmf_info);
  file=(FILE *) NULL;
  unique_file=AcquireUniqueFileResource(filename);
  if (unique_file != -1)
    file=fdopen(unique_file,"wb");
  if ((unique_file == -1) || (file == (FILE *) NULL))
    {
      ipa_device_close(wmf_info);
      wmf_api_destroy(wmf_info);
      ThrowReaderException(FileOpenError,"UnableToCreateTemporaryFile");
    }
  eps_info->out=wmf_stream_create(wmf_info,file);
  eps_info->bbox=bounding_box;
  wmf_status=wmf_play(wmf_info,0,&bounding_box);
  if (wmf_status != wmf_E_None)
    {
      ipa_device_close(wmf_info);
      wmf_api_destroy(wmf_info);
      ThrowReaderException(DelegateError,"FailedToRenderFile");
    }
  (void) fclose(file);
  wmf_api_destroy(wmf_info);
  (void) CloseBlob(image);
  image=DestroyImage(image);
  /*
    Read EPS image.
  */
  read_info=CloneImageInfo(image_info);
  SetImageInfoBlob(read_info,(void *) NULL,0);
  (void) FormatLocaleString(read_info->filename,MagickPathExtent,"eps:%s",
    filename);
  image=ReadImage(read_info,exception);
  read_info=DestroyImageInfo(read_info);
  if (image != (Image *) NULL)
    {
      (void) CopyMagickString(image->filename,image_info->filename,
        MagickPathExtent);
      (void) CopyMagickString(image->magick_filename,image_info->filename,
        MagickPathExtent);
      (void) CopyMagickString(image->magick,"WMF",MagickPathExtent);
    }
  (void) RelinquishUniqueFileResource(filename);
  return(GetFirstImageInList(image));
}