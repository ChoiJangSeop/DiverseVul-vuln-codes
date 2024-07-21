static int IntensityCompare(const void *x,const void *y)
{
  double
    intensity;

  PixelInfo
    *color_1,
    *color_2;

  color_1=(PixelInfo *) x;
  color_2=(PixelInfo *) y;
  intensity=GetPixelInfoIntensity((const Image *) NULL,color_1)-
    GetPixelInfoIntensity((const Image *) NULL,color_2);
  return((int) intensity);
}