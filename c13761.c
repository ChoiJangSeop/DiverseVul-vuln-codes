MagickExport MagickBooleanType DrawPrimitive(Image *image,
  const DrawInfo *draw_info,const PrimitiveInfo *primitive_info,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;

  MagickStatusType
    status;

  ssize_t
    i,
    x;

  ssize_t
    y;

  if (image->debug != MagickFalse)
    {
      (void) LogMagickEvent(DrawEvent,GetMagickModule(),
        "  begin draw-primitive");
      (void) LogMagickEvent(DrawEvent,GetMagickModule(),
        "    affine: %g,%g,%g,%g,%g,%g",draw_info->affine.sx,
        draw_info->affine.rx,draw_info->affine.ry,draw_info->affine.sy,
        draw_info->affine.tx,draw_info->affine.ty);
    }
  status=MagickTrue;
  if ((IsGrayColorspace(image->colorspace) != MagickFalse) &&
      ((IsPixelInfoGray(&draw_info->fill) == MagickFalse) ||
       (IsPixelInfoGray(&draw_info->stroke) == MagickFalse)))
    status&=SetImageColorspace(image,sRGBColorspace,exception);
  if (draw_info->compliance == SVGCompliance)
    {
      status&=SetImageMask(image,WritePixelMask,draw_info->clipping_mask,
        exception);
      status&=SetImageMask(image,CompositePixelMask,draw_info->composite_mask,
        exception);
    }
  x=CastDoubleToLong(ceil(primitive_info->point.x-0.5));
  y=CastDoubleToLong(ceil(primitive_info->point.y-0.5));
  image_view=AcquireAuthenticCacheView(image,exception);
  switch (primitive_info->primitive)
  {
    case AlphaPrimitive:
    {
      if (image->alpha_trait == UndefinedPixelTrait)
        status&=SetImageAlphaChannel(image,OpaqueAlphaChannel,exception);
      switch (primitive_info->method)
      {
        case PointMethod:
        default:
        {
          PixelInfo
            pixel;

          Quantum
            *q;

          q=GetCacheViewAuthenticPixels(image_view,x,y,1,1,exception);
          if (q == (Quantum *) NULL)
            break;
          GetFillColor(draw_info,x,y,&pixel,exception);
          SetPixelAlpha(image,ClampToQuantum(pixel.alpha),q);
          status&=SyncCacheViewAuthenticPixels(image_view,exception);
          break;
        }
        case ReplaceMethod:
        {
          PixelInfo
            pixel,
            target;

          status&=GetOneCacheViewVirtualPixelInfo(image_view,x,y,&target,
            exception);
          GetPixelInfo(image,&pixel);
          for (y=0; y < (ssize_t) image->rows; y++)
          {
            Quantum
              *magick_restrict q;

            q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
              exception);
            if (q == (Quantum *) NULL)
              break;
            for (x=0; x < (ssize_t) image->columns; x++)
            {
              GetPixelInfoPixel(image,q,&pixel);
              if (IsFuzzyEquivalencePixelInfo(&pixel,&target) == MagickFalse)
                {
                  q+=GetPixelChannels(image);
                  continue;
                }
              GetFillColor(draw_info,x,y,&pixel,exception);
              SetPixelAlpha(image,ClampToQuantum(pixel.alpha),q);
              q+=GetPixelChannels(image);
            }
            status&=SyncCacheViewAuthenticPixels(image_view,exception);
            if (status == MagickFalse)
              break;
          }
          break;
        }
        case FloodfillMethod:
        case FillToBorderMethod:
        {
          ChannelType
            channel_mask;

          PixelInfo
            target;

          status&=GetOneVirtualPixelInfo(image,TileVirtualPixelMethod,x,y,
            &target,exception);
          if (primitive_info->method == FillToBorderMethod)
            {
              target.red=(double) draw_info->border_color.red;
              target.green=(double) draw_info->border_color.green;
              target.blue=(double) draw_info->border_color.blue;
            }
          channel_mask=SetImageChannelMask(image,AlphaChannel);
          status&=FloodfillPaintImage(image,draw_info,&target,x,y,
            primitive_info->method == FloodfillMethod ? MagickFalse :
            MagickTrue,exception);
          (void) SetImageChannelMask(image,channel_mask);
          break;
        }
        case ResetMethod:
        {
          PixelInfo
            pixel;

          for (y=0; y < (ssize_t) image->rows; y++)
          {
            Quantum
              *magick_restrict q;

            q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
              exception);
            if (q == (Quantum *) NULL)
              break;
            for (x=0; x < (ssize_t) image->columns; x++)
            {
              GetFillColor(draw_info,x,y,&pixel,exception);
              SetPixelAlpha(image,ClampToQuantum(pixel.alpha),q);
              q+=GetPixelChannels(image);
            }
            status&=SyncCacheViewAuthenticPixels(image_view,exception);
            if (status == MagickFalse)
              break;
          }
          break;
        }
      }
      break;
    }
    case ColorPrimitive:
    {
      switch (primitive_info->method)
      {
        case PointMethod:
        default:
        {
          PixelInfo
            pixel;

          Quantum
            *q;

          q=GetCacheViewAuthenticPixels(image_view,x,y,1,1,exception);
          if (q == (Quantum *) NULL)
            break;
          GetPixelInfo(image,&pixel);
          GetFillColor(draw_info,x,y,&pixel,exception);
          SetPixelViaPixelInfo(image,&pixel,q);
          status&=SyncCacheViewAuthenticPixels(image_view,exception);
          break;
        }
        case ReplaceMethod:
        {
          PixelInfo
            pixel,
            target;

          status&=GetOneCacheViewVirtualPixelInfo(image_view,x,y,&target,
            exception);
          for (y=0; y < (ssize_t) image->rows; y++)
          {
            Quantum
              *magick_restrict q;

            q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
              exception);
            if (q == (Quantum *) NULL)
              break;
            for (x=0; x < (ssize_t) image->columns; x++)
            {
              GetPixelInfoPixel(image,q,&pixel);
              if (IsFuzzyEquivalencePixelInfo(&pixel,&target) == MagickFalse)
                {
                  q+=GetPixelChannels(image);
                  continue;
                }
              GetFillColor(draw_info,x,y,&pixel,exception);
              SetPixelViaPixelInfo(image,&pixel,q);
              q+=GetPixelChannels(image);
            }
            status&=SyncCacheViewAuthenticPixels(image_view,exception);
            if (status == MagickFalse)
              break;
          }
          break;
        }
        case FloodfillMethod:
        case FillToBorderMethod:
        {
          PixelInfo
            target;

          status&=GetOneVirtualPixelInfo(image,TileVirtualPixelMethod,x,y,
            &target,exception);
          if (primitive_info->method == FillToBorderMethod)
            {
              target.red=(double) draw_info->border_color.red;
              target.green=(double) draw_info->border_color.green;
              target.blue=(double) draw_info->border_color.blue;
            }
          status&=FloodfillPaintImage(image,draw_info,&target,x,y,
            primitive_info->method == FloodfillMethod ? MagickFalse :
            MagickTrue,exception);
          break;
        }
        case ResetMethod:
        {
          PixelInfo
            pixel;

          GetPixelInfo(image,&pixel);
          for (y=0; y < (ssize_t) image->rows; y++)
          {
            Quantum
              *magick_restrict q;

            q=GetCacheViewAuthenticPixels(image_view,0,y,image->columns,1,
              exception);
            if (q == (Quantum *) NULL)
              break;
            for (x=0; x < (ssize_t) image->columns; x++)
            {
              GetFillColor(draw_info,x,y,&pixel,exception);
              SetPixelViaPixelInfo(image,&pixel,q);
              q+=GetPixelChannels(image);
            }
            status&=SyncCacheViewAuthenticPixels(image_view,exception);
            if (status == MagickFalse)
              break;
          }
          break;
        }
      }
      break;
    }
    case ImagePrimitive:
    {
      AffineMatrix
        affine;

      char
        composite_geometry[MagickPathExtent];

      Image
        *composite_image,
        *composite_images;

      ImageInfo
        *clone_info;

      RectangleInfo
        geometry;

      ssize_t
        x1,
        y1;

      if (primitive_info->text == (char *) NULL)
        break;
      clone_info=AcquireImageInfo();
      composite_images=(Image *) NULL;
      if (LocaleNCompare(primitive_info->text,"data:",5) == 0)
        composite_images=ReadInlineImage(clone_info,primitive_info->text,
          exception);
      else
        if (*primitive_info->text != '\0')
          {
            (void) CopyMagickString(clone_info->filename,primitive_info->text,
              MagickPathExtent);
            status&=SetImageInfo(clone_info,0,exception);
            (void) CopyMagickString(clone_info->filename,primitive_info->text,
              MagickPathExtent);
            if (clone_info->size != (char *) NULL)
              clone_info->size=DestroyString(clone_info->size);
            if (clone_info->extract != (char *) NULL)
              clone_info->extract=DestroyString(clone_info->extract);
            if ((LocaleCompare(clone_info->magick,"file") == 0) ||
                (LocaleCompare(clone_info->magick,"https") == 0) ||
                (LocaleCompare(clone_info->magick,"http") == 0) ||
                (LocaleCompare(clone_info->magick,"mpri") == 0) ||
                (IsPathAccessible(clone_info->filename) != MagickFalse))
              composite_images=ReadImage(clone_info,exception);
          }
      clone_info=DestroyImageInfo(clone_info);
      if (composite_images == (Image *) NULL)
        {
          status=MagickFalse;
          break;
        }
      composite_image=RemoveFirstImageFromList(&composite_images);
      composite_images=DestroyImageList(composite_images);
      (void) SetImageProgressMonitor(composite_image,(MagickProgressMonitor)
        NULL,(void *) NULL);
      x1=CastDoubleToLong(ceil(primitive_info[1].point.x-0.5));
      y1=CastDoubleToLong(ceil(primitive_info[1].point.y-0.5));
      if (((x1 != 0L) && (x1 != (ssize_t) composite_image->columns)) ||
          ((y1 != 0L) && (y1 != (ssize_t) composite_image->rows)))
        {
          /*
            Resize image.
          */
          (void) FormatLocaleString(composite_geometry,MagickPathExtent,
            "%gx%g!",primitive_info[1].point.x,primitive_info[1].point.y);
          composite_image->filter=image->filter;
          status&=TransformImage(&composite_image,(char *) NULL,
            composite_geometry,exception);
        }
      if (composite_image->alpha_trait == UndefinedPixelTrait)
        status&=SetImageAlphaChannel(composite_image,OpaqueAlphaChannel,
          exception);
      if (draw_info->alpha != OpaqueAlpha)
        status&=SetImageAlpha(composite_image,draw_info->alpha,exception);
      SetGeometry(image,&geometry);
      image->gravity=draw_info->gravity;
      geometry.x=x;
      geometry.y=y;
      (void) FormatLocaleString(composite_geometry,MagickPathExtent,
        "%.20gx%.20g%+.20g%+.20g",(double) composite_image->columns,(double)
        composite_image->rows,(double) geometry.x,(double) geometry.y);
      (void) ParseGravityGeometry(image,composite_geometry,&geometry,exception);
      affine=draw_info->affine;
      affine.tx=(double) geometry.x;
      affine.ty=(double) geometry.y;
      composite_image->interpolate=image->interpolate;
      if ((draw_info->compose == OverCompositeOp) ||
          (draw_info->compose == SrcOverCompositeOp))
        status&=DrawAffineImage(image,composite_image,&affine,exception);
      else
        status&=CompositeImage(image,composite_image,draw_info->compose,
          MagickTrue,geometry.x,geometry.y,exception);
      composite_image=DestroyImage(composite_image);
      break;
    }
    case PointPrimitive:
    {
      PixelInfo
        fill_color;

      Quantum
        *q;

      if ((y < 0) || (y >= (ssize_t) image->rows))
        break;
      if ((x < 0) || (x >= (ssize_t) image->columns))
        break;
      q=GetCacheViewAuthenticPixels(image_view,x,y,1,1,exception);
      if (q == (Quantum *) NULL)
        break;
      GetFillColor(draw_info,x,y,&fill_color,exception);
      CompositePixelOver(image,&fill_color,(double) fill_color.alpha,q,(double)
        GetPixelAlpha(image,q),q);
      status&=SyncCacheViewAuthenticPixels(image_view,exception);
      break;
    }
    case TextPrimitive:
    {
      char
        geometry[MagickPathExtent];

      DrawInfo
        *clone_info;

      if (primitive_info->text == (char *) NULL)
        break;
      clone_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
      (void) CloneString(&clone_info->text,primitive_info->text);
      (void) FormatLocaleString(geometry,MagickPathExtent,"%+f%+f",
        primitive_info->point.x,primitive_info->point.y);
      (void) CloneString(&clone_info->geometry,geometry);
      status&=AnnotateImage(image,clone_info,exception);
      clone_info=DestroyDrawInfo(clone_info);
      break;
    }
    default:
    {
      double
        mid,
        scale;

      DrawInfo
        *clone_info;

      if (IsEventLogging() != MagickFalse)
        LogPrimitiveInfo(primitive_info);
      scale=ExpandAffine(&draw_info->affine);
      if ((draw_info->dash_pattern != (double *) NULL) &&
          (fabs(draw_info->dash_pattern[0]) >= MagickEpsilon) &&
          (fabs(scale*draw_info->stroke_width) >= MagickEpsilon) &&
          (draw_info->stroke.alpha != (Quantum) TransparentAlpha))
        {
          /*
            Draw dash polygon.
          */
          clone_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
          clone_info->stroke_width=0.0;
          clone_info->stroke.alpha=(MagickRealType) TransparentAlpha;
          status&=DrawPolygonPrimitive(image,clone_info,primitive_info,
            exception);
          clone_info=DestroyDrawInfo(clone_info);
          if (status != MagickFalse)
            status&=DrawDashPolygon(draw_info,primitive_info,image,exception);
          break;
        }
      mid=ExpandAffine(&draw_info->affine)*draw_info->stroke_width/2.0;
      if ((mid > 1.0) &&
          ((draw_info->stroke.alpha != (Quantum) TransparentAlpha) ||
           (draw_info->stroke_pattern != (Image *) NULL)))
        {
          double
            point_x,
            point_y;

          MagickBooleanType
            closed_path;

          /*
            Draw strokes while respecting line cap/join attributes.
          */
          closed_path=primitive_info[0].closed_subpath;
          i=(ssize_t) primitive_info[0].coordinates;
          point_x=fabs(primitive_info[i-1].point.x-primitive_info[0].point.x);
          point_y=fabs(primitive_info[i-1].point.y-primitive_info[0].point.y);
          if ((point_x < MagickEpsilon) && (point_y < MagickEpsilon))
            closed_path=MagickTrue;
          if ((((draw_info->linecap == RoundCap) ||
                (closed_path != MagickFalse)) &&
               (draw_info->linejoin == RoundJoin)) ||
               (primitive_info[i].primitive != UndefinedPrimitive))
            {
              status&=DrawPolygonPrimitive(image,draw_info,primitive_info,
                exception);
              break;
            }
          clone_info=CloneDrawInfo((ImageInfo *) NULL,draw_info);
          clone_info->stroke_width=0.0;
          clone_info->stroke.alpha=(MagickRealType) TransparentAlpha;
          status&=DrawPolygonPrimitive(image,clone_info,primitive_info,
            exception);
          clone_info=DestroyDrawInfo(clone_info);
          if (status != MagickFalse)
            status&=DrawStrokePolygon(image,draw_info,primitive_info,exception);
          break;
        }
      status&=DrawPolygonPrimitive(image,draw_info,primitive_info,exception);
      break;
    }
  }
  image_view=DestroyCacheView(image_view);
  if (draw_info->compliance == SVGCompliance)
    {
      status&=SetImageMask(image,WritePixelMask,(Image *) NULL,exception);
      status&=SetImageMask(image,CompositePixelMask,(Image *) NULL,exception);
    }
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(DrawEvent,GetMagickModule(),"  end draw-primitive");
  return(status != 0 ? MagickTrue : MagickFalse);
}