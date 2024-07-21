MagickExport MagickBooleanType GammaImage(Image *image,const double gamma,
  ExceptionInfo *exception)
{
#define GammaImageTag  "Gamma/Image"

  CacheView
    *image_view;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  Quantum
    *gamma_map;

  register ssize_t
    i;

  ssize_t
    y;

  /*
    Allocate and initialize gamma maps.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (gamma == 1.0)
    return(MagickTrue);
  gamma_map=(Quantum *) AcquireQuantumMemory(MaxMap+1UL,sizeof(*gamma_map));
  if (gamma_map == (Quantum *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  (void) memset(gamma_map,0,(MaxMap+1)*sizeof(*gamma_map));
  if (gamma != 0.0)
    for (i=0; i <= (ssize_t) MaxMap; i++)
      gamma_map[i]=ScaleMapToQuantum((double) (MaxMap*pow((double) i/
        MaxMap,1.0/gamma)));
  if (image->storage_class == PseudoClass)
    for (i=0; i < (ssize_t) image->colors; i++)
    {
      /*
        Gamma-correct colormap.
      */
      if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)
        image->colormap[i].red=(double) gamma_map[ScaleQuantumToMap(
          ClampToQuantum(image->colormap[i].red))];
      if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)
        image->colormap[i].green=(double) gamma_map[ScaleQuantumToMap(
          ClampToQuantum(image->colormap[i].green))];
      if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)
        image->colormap[i].blue=(double) gamma_map[ScaleQuantumToMap(
          ClampToQuantum(image->colormap[i].blue))];
      if ((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0)
        image->colormap[i].alpha=(double) gamma_map[ScaleQuantumToMap(
          ClampToQuantum(image->colormap[i].alpha))];
    }
  /*
    Gamma-correct image.
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
      register ssize_t
        j;

      for (j=0; j < (ssize_t) GetPixelChannels(image); j++)
      {
        PixelChannel channel = GetPixelChannelChannel(image,j);
        PixelTrait traits = GetPixelChannelTraits(image,channel);
        if ((traits & UpdatePixelTrait) == 0)
          continue;
        q[j]=gamma_map[ScaleQuantumToMap(ClampToQuantum((MagickRealType)
          q[j]))];
      }
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
        proceed=SetImageProgress(image,GammaImageTag,progress,image->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  image_view=DestroyCacheView(image_view);
  gamma_map=(Quantum *) RelinquishMagickMemory(gamma_map);
  if (image->gamma != 0.0)
    image->gamma*=gamma;
  return(status);
}