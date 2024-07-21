MagickExport MagickBooleanType DrawGradientImage(Image *image,
  const DrawInfo *draw_info,ExceptionInfo *exception)
{
  CacheView
    *image_view;

  const GradientInfo
    *gradient;

  const SegmentInfo
    *gradient_vector;

  double
    length;

  MagickBooleanType
    status;

  PixelInfo
    zero;

  PointInfo
    point;

  RectangleInfo
    bounding_box;

  ssize_t
    y;

  /*
    Draw linear or radial gradient on image.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(draw_info != (const DrawInfo *) NULL);
  gradient=(&draw_info->gradient);
  qsort(gradient->stops,gradient->number_stops,sizeof(StopInfo),
    StopInfoCompare);
  gradient_vector=(&gradient->gradient_vector);
  point.x=gradient_vector->x2-gradient_vector->x1;
  point.y=gradient_vector->y2-gradient_vector->y1;
  length=sqrt(point.x*point.x+point.y*point.y);
  bounding_box=gradient->bounding_box;
  status=MagickTrue;
  GetPixelInfo(image,&zero);
  image_view=AcquireAuthenticCacheView(image,exception);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(static) shared(status) \
    magick_number_threads(image,image,bounding_box.height-bounding_box.y,1)
#endif
  for (y=bounding_box.y; y < (ssize_t) bounding_box.height; y++)
  {
    double
      alpha,
      offset;

    PixelInfo
      composite,
      pixel;

    Quantum
      *magick_restrict q;

    ssize_t
      i,
      x;

    ssize_t
      j;

    if (status == MagickFalse)
      continue;
    q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,exception);
    if (q == (Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    pixel=zero;
    composite=zero;
    offset=GetStopColorOffset(gradient,0,y);
    if (gradient->type != RadialGradient)
      offset*=PerceptibleReciprocal(length);
    for (x=bounding_box.x; x < (ssize_t) bounding_box.width; x++)
    {
      GetPixelInfoPixel(image,q,&pixel);
      switch (gradient->spread)
      {
        case UndefinedSpread:
        case PadSpread:
        {
          if ((x != CastDoubleToLong(ceil(gradient_vector->x1-0.5))) ||
              (y != CastDoubleToLong(ceil(gradient_vector->y1-0.5))))
            {
              offset=GetStopColorOffset(gradient,x,y);
              if (gradient->type != RadialGradient)
                offset*=PerceptibleReciprocal(length);
            }
          for (i=0; i < (ssize_t) gradient->number_stops; i++)
            if (offset < gradient->stops[i].offset)
              break;
          if ((offset < 0.0) || (i == 0))
            composite=gradient->stops[0].color;
          else
            if ((offset > 1.0) || (i == (ssize_t) gradient->number_stops))
              composite=gradient->stops[gradient->number_stops-1].color;
            else
              {
                j=i;
                i--;
                alpha=(offset-gradient->stops[i].offset)/
                  (gradient->stops[j].offset-gradient->stops[i].offset);
                CompositePixelInfoBlend(&gradient->stops[i].color,1.0-alpha,
                  &gradient->stops[j].color,alpha,&composite);
              }
          break;
        }
        case ReflectSpread:
        {
          if ((x != CastDoubleToLong(ceil(gradient_vector->x1-0.5))) ||
              (y != CastDoubleToLong(ceil(gradient_vector->y1-0.5))))
            {
              offset=GetStopColorOffset(gradient,x,y);
              if (gradient->type != RadialGradient)
                offset*=PerceptibleReciprocal(length);
            }
          if (offset < 0.0)
            offset=(-offset);
          if ((ssize_t) fmod(offset,2.0) == 0)
            offset=fmod(offset,1.0);
          else
            offset=1.0-fmod(offset,1.0);
          for (i=0; i < (ssize_t) gradient->number_stops; i++)
            if (offset < gradient->stops[i].offset)
              break;
          if (i == 0)
            composite=gradient->stops[0].color;
          else
            if (i == (ssize_t) gradient->number_stops)
              composite=gradient->stops[gradient->number_stops-1].color;
            else
              {
                j=i;
                i--;
                alpha=(offset-gradient->stops[i].offset)/
                  (gradient->stops[j].offset-gradient->stops[i].offset);
                CompositePixelInfoBlend(&gradient->stops[i].color,1.0-alpha,
                  &gradient->stops[j].color,alpha,&composite);
              }
          break;
        }
        case RepeatSpread:
        {
          double
            repeat;

          MagickBooleanType
            antialias;

          antialias=MagickFalse;
          repeat=0.0;
          if ((x != CastDoubleToLong(ceil(gradient_vector->x1-0.5))) ||
              (y != CastDoubleToLong(ceil(gradient_vector->y1-0.5))))
            {
              offset=GetStopColorOffset(gradient,x,y);
              if (gradient->type == LinearGradient)
                {
                  repeat=fmod(offset,length);
                  if (repeat < 0.0)
                    repeat=length-fmod(-repeat,length);
                  else
                    repeat=fmod(offset,length);
                  antialias=(repeat < length) && ((repeat+1.0) > length) ?
                    MagickTrue : MagickFalse;
                  offset=PerceptibleReciprocal(length)*repeat;
                }
              else
                {
                  repeat=fmod(offset,gradient->radius);
                  if (repeat < 0.0)
                    repeat=gradient->radius-fmod(-repeat,gradient->radius);
                  else
                    repeat=fmod(offset,gradient->radius);
                  antialias=repeat+1.0 > gradient->radius ? MagickTrue :
                    MagickFalse;
                  offset=repeat/gradient->radius;
                }
            }
          for (i=0; i < (ssize_t) gradient->number_stops; i++)
            if (offset < gradient->stops[i].offset)
              break;
          if (i == 0)
            composite=gradient->stops[0].color;
          else
            if (i == (ssize_t) gradient->number_stops)
              composite=gradient->stops[gradient->number_stops-1].color;
            else
              {
                j=i;
                i--;
                alpha=(offset-gradient->stops[i].offset)/
                  (gradient->stops[j].offset-gradient->stops[i].offset);
                if (antialias != MagickFalse)
                  {
                    if (gradient->type == LinearGradient)
                      alpha=length-repeat;
                    else
                      alpha=gradient->radius-repeat;
                    i=0;
                    j=(ssize_t) gradient->number_stops-1L;
                  }
                CompositePixelInfoBlend(&gradient->stops[i].color,1.0-alpha,
                  &gradient->stops[j].color,alpha,&composite);
              }
          break;
        }
      }
      CompositePixelInfoOver(&composite,composite.alpha,&pixel,pixel.alpha,
        &pixel);
      SetPixelViaPixelInfo(image,&pixel,q);
      q+=GetPixelChannels(image);
    }
    if (SyncCacheViewAuthenticPixels(image_view,exception) == MagickFalse)
      status=MagickFalse;
  }
  image_view=DestroyCacheView(image_view);
  return(status);
}