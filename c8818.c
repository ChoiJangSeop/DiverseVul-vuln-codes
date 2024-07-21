static Image *ReadSVGImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  Image
    *image,
    *svg_image;

  MagickBooleanType
    status;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(exception != (ExceptionInfo *) NULL);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception->signature == MagickCoreSignature);
  image=AcquireImage(image_info,exception);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  if ((fabs(image->resolution.x) < MagickEpsilon) ||
      (fabs(image->resolution.y) < MagickEpsilon))
    {
      GeometryInfo
        geometry_info;

      MagickStatusType
        flags;

      flags=ParseGeometry(SVGDensityGeometry,&geometry_info);
      image->resolution.x=geometry_info.rho;
      image->resolution.y=geometry_info.sigma;
      if ((flags & SigmaValue) == 0)
        image->resolution.y=image->resolution.x;
    }
  svg_image=RenderSVGImage(image_info,image,exception);
  image=DestroyImage(image);
  return(svg_image);
}