MagickExport MagickBooleanType DrawClipPath(Image *image,
  const DrawInfo *draw_info,const char *name,ExceptionInfo *exception)
{
  char
    filename[MagickPathExtent];

  Image
    *clip_mask;

  const char
    *value;

  DrawInfo
    *clone_info;

  MagickStatusType
    status;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(draw_info != (const DrawInfo *) NULL);
  (void) FormatLocaleString(filename,MagickPathExtent,"%s",name);
  value=GetImageArtifact(image,filename);
  if (value == (const char *) NULL)
    return(MagickFalse);
  clip_mask=CloneImage(image,image->columns,image->rows,MagickTrue,exception);
  if (clip_mask == (Image *) NULL)
    return(MagickFalse);
  (void) QueryColorCompliance("#0000",AllCompliance,
    &clip_mask->background_color,exception);
  clip_mask->background_color.alpha=(Quantum) TransparentAlpha;
  (void) SetImageBackgroundColor(clip_mask,exception);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"\nbegin clip-path %s",
      draw_info->clip_mask);
  clone_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
  (void) CloneString(&clone_info->primitive,value);
  (void) QueryColorCompliance("#ffffff",AllCompliance,&clone_info->fill,
    exception);
  clone_info->clip_mask=(char *) NULL;
  status=NegateImage(clip_mask,MagickFalse,exception);
  (void) SetImageMask(image,ReadPixelMask,clip_mask,exception);
  clip_mask=DestroyImage(clip_mask);
  status&=DrawImage(image,clone_info,exception);
  clone_info=DestroyDrawInfo(clone_info);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"end clip-path");
  return(status != 0 ? MagickTrue : MagickFalse);
}