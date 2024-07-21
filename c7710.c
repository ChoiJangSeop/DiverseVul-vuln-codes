MagickExport Image *EvaluateImages(const Image *images,
  const MagickEvaluateOperator op,ExceptionInfo *exception)
{
#define EvaluateImageTag  "Evaluate/Image"

  CacheView
    *evaluate_view;

  Image
    *image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  PixelChannels
    **magick_restrict evaluate_pixels;

  RandomInfo
    **magick_restrict random_info;

  size_t
    number_images;

  ssize_t
    y;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
  unsigned long
    key;
#endif

  assert(images != (Image *) NULL);
  assert(images->signature == MagickCoreSignature);
  if (images->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  image=AcquireImageCanvas(images,exception);
  if (image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    {
      image=DestroyImage(image);
      return((Image *) NULL);
    }
  number_images=GetImageListLength(images);
  evaluate_pixels=AcquirePixelThreadSet(images);
  if (evaluate_pixels == (PixelChannels **) NULL)
    {
      image=DestroyImage(image);
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",images->filename);
      return((Image *) NULL);
    }
  /*
    Evaluate image pixels.
  */
  status=MagickTrue;
  progress=0;
  random_info=AcquireRandomInfoThreadSet();
  evaluate_view=AcquireAuthenticCacheView(image,exception);
  if (op == MedianEvaluateOperator)
    {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      key=GetRandomSecretKey(random_info[0]);
      #pragma omp parallel for schedule(static) shared(progress,status) \
        magick_number_threads(image,images,image->rows,key == ~0UL)
#endif
      for (y=0; y < (ssize_t) image->rows; y++)
      {
        CacheView
          *image_view;

        const Image
          *next;

        const int
          id = GetOpenMPThreadId();

        register PixelChannels
          *evaluate_pixel;

        register Quantum
          *magick_restrict q;

        register ssize_t
          x;

        if (status == MagickFalse)
          continue;
        q=QueueCacheViewAuthenticPixels(evaluate_view,0,y,image->columns,1,
          exception);
        if (q == (Quantum *) NULL)
          {
            status=MagickFalse;
            continue;
          }
        evaluate_pixel=evaluate_pixels[id];
        for (x=0; x < (ssize_t) image->columns; x++)
        {
          register ssize_t
            j,
            k;

          for (j=0; j < (ssize_t) number_images; j++)
            for (k=0; k < MaxPixelChannels; k++)
              evaluate_pixel[j].channel[k]=0.0;
          next=images;
          for (j=0; j < (ssize_t) number_images; j++)
          {
            register const Quantum
              *p;

            register ssize_t
              i;

            image_view=AcquireVirtualCacheView(next,exception);
            p=GetCacheViewVirtualPixels(image_view,x,y,1,1,exception);
            if (p == (const Quantum *) NULL)
              {
                image_view=DestroyCacheView(image_view);
                break;
              }
            for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
            {
              PixelChannel channel = GetPixelChannelChannel(image,i);
              PixelTrait evaluate_traits=GetPixelChannelTraits(image,channel);
              PixelTrait traits = GetPixelChannelTraits(next,channel);
              if ((traits == UndefinedPixelTrait) ||
                  (evaluate_traits == UndefinedPixelTrait))
                continue;
              if ((traits & UpdatePixelTrait) == 0)
                continue;
              evaluate_pixel[j].channel[i]=ApplyEvaluateOperator(
                random_info[id],GetPixelChannel(image,channel,p),op,
                evaluate_pixel[j].channel[i]);
            }
            image_view=DestroyCacheView(image_view);
            next=GetNextImageInList(next);
          }
          qsort((void *) evaluate_pixel,number_images,sizeof(*evaluate_pixel),
            IntensityCompare);
          for (k=0; k < (ssize_t) GetPixelChannels(image); k++)
            q[k]=ClampToQuantum(evaluate_pixel[j/2].channel[k]);
          q+=GetPixelChannels(image);
        }
        if (SyncCacheViewAuthenticPixels(evaluate_view,exception) == MagickFalse)
          status=MagickFalse;
        if (images->progress_monitor != (MagickProgressMonitor) NULL)
          {
            MagickBooleanType
              proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
            #pragma omp atomic
#endif
            progress++;
            proceed=SetImageProgress(images,EvaluateImageTag,progress,
              image->rows);
            if (proceed == MagickFalse)
              status=MagickFalse;
          }
      }
    }
  else
    {
#if defined(MAGICKCORE_OPENMP_SUPPORT)
      key=GetRandomSecretKey(random_info[0]);
      #pragma omp parallel for schedule(static) shared(progress,status) \
        magick_number_threads(image,images,image->rows,key == ~0UL)
#endif
      for (y=0; y < (ssize_t) image->rows; y++)
      {
        CacheView
          *image_view;

        const Image
          *next;

        const int
          id = GetOpenMPThreadId();

        register ssize_t
          i,
          x;

        register PixelChannels
          *evaluate_pixel;

        register Quantum
          *magick_restrict q;

        ssize_t
          j;

        if (status == MagickFalse)
          continue;
        q=QueueCacheViewAuthenticPixels(evaluate_view,0,y,image->columns,1,
          exception);
        if (q == (Quantum *) NULL)
          {
            status=MagickFalse;
            continue;
          }
        evaluate_pixel=evaluate_pixels[id];
        for (j=0; j < (ssize_t) image->columns; j++)
          for (i=0; i < MaxPixelChannels; i++)
            evaluate_pixel[j].channel[i]=0.0;
        next=images;
        for (j=0; j < (ssize_t) number_images; j++)
        {
          register const Quantum
            *p;

          image_view=AcquireVirtualCacheView(next,exception);
          p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,
            exception);
          if (p == (const Quantum *) NULL)
            {
              image_view=DestroyCacheView(image_view);
              break;
            }
          for (x=0; x < (ssize_t) image->columns; x++)
          {
            register ssize_t
              i;

            for (i=0; i < (ssize_t) GetPixelChannels(next); i++)
            {
              PixelChannel channel = GetPixelChannelChannel(image,i);
              PixelTrait traits = GetPixelChannelTraits(next,channel);
              PixelTrait evaluate_traits=GetPixelChannelTraits(image,channel);
              if ((traits == UndefinedPixelTrait) ||
                  (evaluate_traits == UndefinedPixelTrait))
                continue;
              if ((traits & UpdatePixelTrait) == 0)
                continue;
              evaluate_pixel[x].channel[i]=ApplyEvaluateOperator(
                random_info[id],GetPixelChannel(image,channel,p),j == 0 ?
                AddEvaluateOperator : op,evaluate_pixel[x].channel[i]);
            }
            p+=GetPixelChannels(next);
          }
          image_view=DestroyCacheView(image_view);
          next=GetNextImageInList(next);
        }
        for (x=0; x < (ssize_t) image->columns; x++)
        {
          register ssize_t
             i;

          switch (op)
          {
            case MeanEvaluateOperator:
            {
              for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
                evaluate_pixel[x].channel[i]/=(double) number_images;
              break;
            }
            case MultiplyEvaluateOperator:
            {
              for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
              {
                register ssize_t
                  j;

                for (j=0; j < (ssize_t) (number_images-1); j++)
                  evaluate_pixel[x].channel[i]*=QuantumScale;
              }
              break;
            }
            case RootMeanSquareEvaluateOperator:
            {
              for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
                evaluate_pixel[x].channel[i]=sqrt(evaluate_pixel[x].channel[i]/
                  number_images);
              break;
            }
            default:
              break;
          }
        }
        for (x=0; x < (ssize_t) image->columns; x++)
        {
          register ssize_t
            i;

          for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
          {
            PixelChannel channel = GetPixelChannelChannel(image,i);
            PixelTrait traits = GetPixelChannelTraits(image,channel);
            if (traits == UndefinedPixelTrait)
              continue;
            if ((traits & UpdatePixelTrait) == 0)
              continue;
            q[i]=ClampToQuantum(evaluate_pixel[x].channel[i]);
          }
          q+=GetPixelChannels(image);
        }
        if (SyncCacheViewAuthenticPixels(evaluate_view,exception) == MagickFalse)
          status=MagickFalse;
        if (images->progress_monitor != (MagickProgressMonitor) NULL)
          {
            MagickBooleanType
              proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
            #pragma omp atomic
#endif
            progress++;
            proceed=SetImageProgress(images,EvaluateImageTag,progress,
              image->rows);
            if (proceed == MagickFalse)
              status=MagickFalse;
          }
      }
    }
  evaluate_view=DestroyCacheView(evaluate_view);
  evaluate_pixels=DestroyPixelThreadSet(images,evaluate_pixels);
  random_info=DestroyRandomInfoThreadSet(random_info);
  if (status == MagickFalse)
    image=DestroyImage(image);
  return(image);
}