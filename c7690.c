MagickExport Image *ComplexImages(const Image *images,const ComplexOperator op,
  ExceptionInfo *exception)
{
#define ComplexImageTag  "Complex/Image"

  CacheView
    *Ai_view,
    *Ar_view,
    *Bi_view,
    *Br_view,
    *Ci_view,
    *Cr_view;

  const char
    *artifact;

  const Image
    *Ai_image,
    *Ar_image,
    *Bi_image,
    *Br_image;

  double
    snr;

  Image
    *Ci_image,
    *complex_images,
    *Cr_image,
    *image;

  MagickBooleanType
    status;

  MagickOffsetType
    progress;

  ssize_t
    y;

  assert(images != (Image *) NULL);
  assert(images->signature == MagickCoreSignature);
  if (images->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  if (images->next == (Image *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ImageError,
        "ImageSequenceRequired","`%s'",images->filename);
      return((Image *) NULL);
    }
  image=CloneImage(images,0,0,MagickTrue,exception);
  if (image == (Image *) NULL)
    return((Image *) NULL);
  if (SetImageStorageClass(image,DirectClass,exception) == MagickFalse)
    {
      image=DestroyImageList(image);
      return(image);
    }
  image->depth=32UL;
  complex_images=NewImageList();
  AppendImageToList(&complex_images,image);
  image=CloneImage(images,0,0,MagickTrue,exception);
  if (image == (Image *) NULL)
    {
      complex_images=DestroyImageList(complex_images);
      return(complex_images);
    }
  AppendImageToList(&complex_images,image);
  /*
    Apply complex mathematics to image pixels.
  */
  artifact=GetImageArtifact(image,"complex:snr");
  snr=0.0;
  if (artifact != (const char *) NULL)
    snr=StringToDouble(artifact,(char **) NULL);
  Ar_image=images;
  Ai_image=images->next;
  Br_image=images;
  Bi_image=images->next;
  if ((images->next->next != (Image *) NULL) &&
      (images->next->next->next != (Image *) NULL))
    {
      Br_image=images->next->next;
      Bi_image=images->next->next->next;
    }
  Cr_image=complex_images;
  Ci_image=complex_images->next;
  Ar_view=AcquireVirtualCacheView(Ar_image,exception);
  Ai_view=AcquireVirtualCacheView(Ai_image,exception);
  Br_view=AcquireVirtualCacheView(Br_image,exception);
  Bi_view=AcquireVirtualCacheView(Bi_image,exception);
  Cr_view=AcquireAuthenticCacheView(Cr_image,exception);
  Ci_view=AcquireAuthenticCacheView(Ci_image,exception);
  status=MagickTrue;
  progress=0;
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(progress,status) \
    magick_number_threads(images,complex_images,images->rows,1L)
#endif
  for (y=0; y < (ssize_t) images->rows; y++)
  {
    register const Quantum
      *magick_restrict Ai,
      *magick_restrict Ar,
      *magick_restrict Bi,
      *magick_restrict Br;

    register Quantum
      *magick_restrict Ci,
      *magick_restrict Cr;

    register ssize_t
      x;

    if (status == MagickFalse)
      continue;
    Ar=GetCacheViewVirtualPixels(Ar_view,0,y,
      MagickMax(Ar_image->columns,Cr_image->columns),1,exception);
    Ai=GetCacheViewVirtualPixels(Ai_view,0,y,
      MagickMax(Ai_image->columns,Ci_image->columns),1,exception);
    Br=GetCacheViewVirtualPixels(Br_view,0,y,
      MagickMax(Br_image->columns,Cr_image->columns),1,exception);
    Bi=GetCacheViewVirtualPixels(Bi_view,0,y,
      MagickMax(Bi_image->columns,Ci_image->columns),1,exception);
    Cr=QueueCacheViewAuthenticPixels(Cr_view,0,y,Cr_image->columns,1,exception);
    Ci=QueueCacheViewAuthenticPixels(Ci_view,0,y,Ci_image->columns,1,exception);
    if ((Ar == (const Quantum *) NULL) || (Ai == (const Quantum *) NULL) || 
        (Br == (const Quantum *) NULL) || (Bi == (const Quantum *) NULL) ||
        (Cr == (Quantum *) NULL) || (Ci == (Quantum *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    for (x=0; x < (ssize_t) images->columns; x++)
    {
      register ssize_t
        i;

      for (i=0; i < (ssize_t) GetPixelChannels(images); i++)
      {
        switch (op)
        {
          case AddComplexOperator:
          {
            Cr[i]=Ar[i]+Br[i];
            Ci[i]=Ai[i]+Bi[i];
            break;
          }
          case ConjugateComplexOperator:
          default:
          {
            Cr[i]=Ar[i];
            Ci[i]=(-Bi[i]);
            break;
          }
          case DivideComplexOperator:
          {
            double
              gamma;

            gamma=PerceptibleReciprocal(Br[i]*Br[i]+Bi[i]*Bi[i]+snr);
            Cr[i]=gamma*(Ar[i]*Br[i]+Ai[i]*Bi[i]);
            Ci[i]=gamma*(Ai[i]*Br[i]-Ar[i]*Bi[i]);
            break;
          }
          case MagnitudePhaseComplexOperator:
          {
            Cr[i]=sqrt(Ar[i]*Ar[i]+Ai[i]*Ai[i]);
            Ci[i]=atan2(Ai[i],Ar[i])/(2.0*MagickPI)+0.5;
            break;
          }
          case MultiplyComplexOperator:
          {
            Cr[i]=QuantumScale*(Ar[i]*Br[i]-Ai[i]*Bi[i]);
            Ci[i]=QuantumScale*(Ai[i]*Br[i]+Ar[i]*Bi[i]);
            break;
          }
          case RealImaginaryComplexOperator:
          {
            Cr[i]=Ar[i]*cos(2.0*MagickPI*(Ai[i]-0.5));
            Ci[i]=Ar[i]*sin(2.0*MagickPI*(Ai[i]-0.5));
            break;
          }
          case SubtractComplexOperator:
          {
            Cr[i]=Ar[i]-Br[i];
            Ci[i]=Ai[i]-Bi[i];
            break;
          }
        }
      }
      Ar+=GetPixelChannels(Ar_image);
      Ai+=GetPixelChannels(Ai_image);
      Br+=GetPixelChannels(Br_image);
      Bi+=GetPixelChannels(Bi_image);
      Cr+=GetPixelChannels(Cr_image);
      Ci+=GetPixelChannels(Ci_image);
    }
    if (SyncCacheViewAuthenticPixels(Ci_view,exception) == MagickFalse)
      status=MagickFalse;
    if (SyncCacheViewAuthenticPixels(Cr_view,exception) == MagickFalse)
      status=MagickFalse;
    if (images->progress_monitor != (MagickProgressMonitor) NULL)
      {
        MagickBooleanType
          proceed;

#if defined(MAGICKCORE_OPENMP_SUPPORT)
        #pragma omp atomic
#endif
        progress++;
        proceed=SetImageProgress(images,ComplexImageTag,progress,images->rows);
        if (proceed == MagickFalse)
          status=MagickFalse;
      }
  }
  Cr_view=DestroyCacheView(Cr_view);
  Ci_view=DestroyCacheView(Ci_view);
  Br_view=DestroyCacheView(Br_view);
  Bi_view=DestroyCacheView(Bi_view);
  Ar_view=DestroyCacheView(Ar_view);
  Ai_view=DestroyCacheView(Ai_view);
  if (status == MagickFalse)
    complex_images=DestroyImageList(complex_images);
  return(complex_images);
}