static MagickBooleanType WritePTIFImage(const ImageInfo *image_info,
  Image *image,ExceptionInfo *exception)
{
  Image
    *images,
    *next,
    *pyramid_image;

  ImageInfo
    *write_info;

  MagickBooleanType
    status;

  PointInfo
    resolution;

  size_t
    columns,
    rows;

  /*
    Create pyramid-encoded TIFF image.
  */
  images=NewImageList();
  for (next=image; next != (Image *) NULL; next=GetNextImageInList(next))
  {
    Image
      *clone_image;

    clone_image=CloneImage(next,0,0,MagickFalse,exception);
    if (clone_image == (Image *) NULL)
      break;
    clone_image->previous=NewImageList();
    clone_image->next=NewImageList();
    (void) SetImageProperty(clone_image,"tiff:subfiletype","none",exception);
    AppendImageToList(&images,clone_image);
    columns=next->columns;
    rows=next->rows;
    resolution=next->resolution;
    while ((columns > 64) && (rows > 64))
    {
      columns/=2;
      rows/=2;
      resolution.x/=2;
      resolution.y/=2;
      pyramid_image=ResizeImage(next,columns,rows,image->filter,exception);
      if (pyramid_image == (Image *) NULL)
        break;
      pyramid_image->resolution=resolution;
      (void) SetImageProperty(pyramid_image,"tiff:subfiletype","REDUCEDIMAGE",
        exception);
      AppendImageToList(&images,pyramid_image);
    }
  }
  images=GetFirstImageInList(images);
  /*
    Write pyramid-encoded TIFF image.
  */
  write_info=CloneImageInfo(image_info);
  write_info->adjoin=MagickTrue;
  (void) CopyMagickString(write_info->magick,"TIFF",MagickPathExtent);
  (void) CopyMagickString(images->magick,"TIFF",MagickPathExtent);
  status=WriteTIFFImage(write_info,images,exception);
  images=DestroyImageList(images);
  write_info=DestroyImageInfo(write_info);
  return(status);
}