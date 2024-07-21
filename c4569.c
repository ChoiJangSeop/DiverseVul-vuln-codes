MagickExport Image *PingImages(ImageInfo *image_info,const char *filename,
  ExceptionInfo *exception)
{
  char
    ping_filename[MagickPathExtent];

  Image
    *image,
    *images;

  ImageInfo
    *read_info;

  /*
    Ping image list from a file.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  (void) SetImageOption(image_info,"filename",filename);
  (void) CopyMagickString(image_info->filename,filename,MagickPathExtent);
  (void) InterpretImageFilename(image_info,(Image *) NULL,image_info->filename,
    (int) image_info->scene,ping_filename,exception);
  if (LocaleCompare(ping_filename,image_info->filename) != 0)
    {
      ExceptionInfo
        *sans;

      ssize_t
        extent,
        scene;

      /*
        Images of the form image-%d.png[1-5].
      */
      read_info=CloneImageInfo(image_info);
      sans=AcquireExceptionInfo();
      (void) SetImageInfo(read_info,0,sans);
      sans=DestroyExceptionInfo(sans);
      if (read_info->number_scenes == 0)
        {
          read_info=DestroyImageInfo(read_info);
          return(PingImage(image_info,exception));
        }
      (void) CopyMagickString(ping_filename,read_info->filename,MagickPathExtent);
      images=NewImageList();
      extent=(ssize_t) (read_info->scene+read_info->number_scenes);
      for (scene=(ssize_t) read_info->scene; scene < (ssize_t) extent; scene++)
      {
        (void) InterpretImageFilename(image_info,(Image *) NULL,ping_filename,
          (int) scene,read_info->filename,exception);
        image=PingImage(read_info,exception);
        if (image == (Image *) NULL)
          continue;
        AppendImageToList(&images,image);
      }
      read_info=DestroyImageInfo(read_info);
      return(images);
    }
  return(PingImage(image_info,exception));
}