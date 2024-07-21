MagickExport Image *CropImageToTiles(const Image *image,
  const char *crop_geometry,ExceptionInfo *exception)
{
  Image
    *next,
    *crop_image;

  MagickStatusType
    flags;

  RectangleInfo
    geometry;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  crop_image=NewImageList();
  next=NewImageList();
  flags=ParseGravityGeometry(image,crop_geometry,&geometry,exception);
  if ((flags & AreaValue) != 0)
    {
      PointInfo
        delta,
        offset;

      RectangleInfo
        crop;

      size_t
        height,
        width;

      /*
        Crop into NxM tiles (@ flag).
      */
      width=image->columns;
      height=image->rows;
      if (geometry.width == 0)
        geometry.width=1;
      if (geometry.height == 0)
        geometry.height=1;
      if ((flags & AspectValue) == 0)
        {
          width-=(geometry.x < 0 ? -1 : 1)*geometry.x;
          height-=(geometry.y < 0 ? -1 : 1)*geometry.y;
        }
      else
        {
          width+=(geometry.x < 0 ? -1 : 1)*geometry.x;
          height+=(geometry.y < 0 ? -1 : 1)*geometry.y;
        }
      delta.x=(double) width/geometry.width;
      delta.y=(double) height/geometry.height;
      if (delta.x < 1.0)
        delta.x=1.0;
      if (delta.y < 1.0)
        delta.y=1.0;
      for (offset.y=0; offset.y < (double) height; )
      {
        if ((flags & AspectValue) == 0)
          {
            crop.y=(ssize_t) MagickRound((double) (offset.y-
              (geometry.y > 0 ? 0 : geometry.y)));
            offset.y+=delta.y;   /* increment now to find width */
            crop.height=(size_t) MagickRound((double) (offset.y+
              (geometry.y < 0 ? 0 : geometry.y)));
          }
        else
          {
            crop.y=(ssize_t) MagickRound((double) (offset.y-
              (geometry.y > 0 ? geometry.y : 0)));
            offset.y+=delta.y;  /* increment now to find width */
            crop.height=(size_t) MagickRound((double)
              (offset.y+(geometry.y < -1 ? geometry.y : 0)));
          }
        crop.height-=crop.y;
        crop.y+=image->page.y;
        for (offset.x=0; offset.x < (double) width; )
        {
          if ((flags & AspectValue) == 0)
            {
              crop.x=(ssize_t) MagickRound((double) (offset.x-
                (geometry.x > 0 ? 0 : geometry.x)));
              offset.x+=delta.x;  /* increment now to find height */
              crop.width=(size_t) MagickRound((double) (offset.x+
                (geometry.x < 0 ? 0 : geometry.x)));
            }
          else
            {
              crop.x=(ssize_t) MagickRound((double) (offset.x-
                (geometry.x > 0 ? geometry.x : 0)));
              offset.x+=delta.x;  /* increment now to find height */
              crop.width=(size_t) MagickRound((double) (offset.x+
                (geometry.x < 0 ? geometry.x : 0)));
            }
          crop.width-=crop.x;
          crop.x+=image->page.x;
          next=CropImage(image,&crop,exception);
          if (next != (Image *) NULL)
            AppendImageToList(&crop_image,next);
        }
      }
      ClearMagickException(exception);
      return(crop_image);
    }
  if (((geometry.width == 0) && (geometry.height == 0)) ||
      ((flags & XValue) != 0) || ((flags & YValue) != 0))
    {
      /*
        Crop a single region at +X+Y.
      */
      crop_image=CropImage(image,&geometry,exception);
      if ((crop_image != (Image *) NULL) && ((flags & AspectValue) != 0))
        {
          crop_image->page.width=geometry.width;
          crop_image->page.height=geometry.height;
          crop_image->page.x-=geometry.x;
          crop_image->page.y-=geometry.y;
        }
      return(crop_image);
    }
  if ((image->columns > geometry.width) || (image->rows > geometry.height))
    {
      RectangleInfo
        page;

      size_t
        height,
        width;

      ssize_t
        x,
        y;

      /*
        Crop into tiles of fixed size WxH.
      */
      page=image->page;
      if (page.width == 0)
        page.width=image->columns;
      if (page.height == 0)
        page.height=image->rows;
      width=geometry.width;
      if (width == 0)
        width=page.width;
      height=geometry.height;
      if (height == 0)
        height=page.height;
      next=NewImageList();
      for (y=0; y < (ssize_t) page.height; y+=(ssize_t) height)
      {
        for (x=0; x < (ssize_t) page.width; x+=(ssize_t) width)
        {
          geometry.width=width;
          geometry.height=height;
          geometry.x=x;
          geometry.y=y;
          next=CropImage(image,&geometry,exception);
          if (next == (Image *) NULL)
            break;
          AppendImageToList(&crop_image,next);
        }
        if (next == (Image *) NULL)
          break;
      }
      return(crop_image);
    }
  return(CloneImage(image,0,0,MagickTrue,exception));
}