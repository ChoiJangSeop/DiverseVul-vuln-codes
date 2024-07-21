static int IntensityCompare(const void *x,const void *y)
{
  PixelInfo
    *color_1,
    *color_2;

  ssize_t
    intensity;

  color_1=(PixelInfo *) x;
  color_2=(PixelInfo *) y;
  intensity=(ssize_t) (GetPixelInfoIntensity((const Image *) NULL,color_1)-
    GetPixelInfoIntensity((const Image *) NULL,color_2));
  return((int) intensity);
}