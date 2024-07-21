MagickExport MagickBooleanType PosterizeImage(Image *image,const size_t levels,
  const DitherMethod dither_method,ExceptionInfo *exception)
{
#define PosterizeImageTag  "Posterize/Image"
#define PosterizePixel(pixel) (Quantum) (QuantumRange*(MagickRound( \
  QuantumScale*pixel*(levels-1)))/MagickMax((ssize_t) levels-1,1))

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  QuantizeInfo
    *quantize_info;

  register ssize_t
    i;

  ssize_t
    y;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (image->storage_class == PseudoClass)
#if defined(MAGICKCORE_OPENMP_SUPPORT)
    #pragma omp parallel for schedule(static) shared(progress,status) \
      magick_number_threads(image,image,image->colors,1)
#endif
    for (i=0; i < (ssize_t) image->colors; i++)
    {
      /*
        Posterize colormap.
      */
      if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
        image->colormap[i].red=(double)
          PosterizePixel(image->colormap[i].red);
      if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
        image->colormap[i].green=(double)
          PosterizePixel(image->colormap[i].green);
      if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
        image->colormap[i].blue=(double)
          PosterizePixel(image->colormap[i].blue);
      if ((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0)
        image->colormap[i].alpha=(double)
          PosterizePixel(image->colormap[i].alpha);
    }
  /*
    Posterize image.
  */
  status=MagickTrue;
  progress=0;
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(image,image,image->rows,1)
#endif
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    register Quantum
      *magick_restrict q;

    register ssize_t
      x;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) image->columns; x++)
    {
      if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
        SetPixelRed(image,PosterizePixel(GetPixelRed(image,q)),q);
      if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
        SetPixelGreen(image,PosterizePixel(GetPixelGreen(image,q)),q);
      if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
        SetPixelBlue(image,PosterizePixel(GetPixelBlue(image,q)),q);
      if (((GetPixelBlackTraits(image) & UpdatePixelTrait) != 0) &&
          (image->colorspace == CMYKColorspace))
        SetPixelBlack(image,PosterizePixel(GetPixelBlack(image,q)),q);
      if (((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0) &&
          (image->alpha_trait == BlendPixelTrait))
        SetPixelAlpha(image,PosterizePixel(GetPixelAlpha(image,q)),q);
      q+=GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(image,PosterizeImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  quantize_info=AcquireQuantizeInfo((ImageInfo *) NULL);
  quantize_info->number_colors=(size_t) MagickMin((ssize_t) levels*levels*
    levels,MaxColormapSize+1);
  quantize_info->dither_method=dither_method;
  quantize_info->tree_depth=MaxTreeDepth;
  status=QuantizeImage(quantize_info,image,exception);
  quantize_info=DestroyQuantizeInfo(quantize_info);
  return(status);
}