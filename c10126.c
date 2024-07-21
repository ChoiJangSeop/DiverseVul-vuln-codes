MagickExport void ConvertRGBToHSL(const double red,const double green,
  const double blue,double *hue,double *saturation,double *lightness)
{
  double
    c,
    max,
    min;

  /*
    Convert RGB to HSL colorspace.
  */
  assert(hue != (double *) NULL);
  assert(saturation != (double *) NULL);
  assert(lightness != (double *) NULL);
  max=MagickMax(QuantumScale*red,MagickMax(QuantumScale*green,
    QuantumScale*blue));
  min=MagickMin(QuantumScale*red,MagickMin(QuantumScale*green,
    QuantumScale*blue));
  c=max-min;
  *lightness=(max+min)/2.0;
  if (c <= 0.0)
    {
      *hue=0.0;
      *saturation=0.0;
      return;
    }
  if (fabs(max-QuantumScale*red) < MagickEpsilon)
    {
      *hue=(QuantumScale*green-QuantumScale*blue)/c;
      if ((QuantumScale*green) < (QuantumScale*blue))
        *hue+=6.0;
    }
  else
    if (fabs(max-QuantumScale*green) < MagickEpsilon)
      *hue=2.0+(QuantumScale*blue-QuantumScale*red)/c;
    else
      *hue=4.0+(QuantumScale*red-QuantumScale*green)/c;
  *hue*=60.0/360.0;
  if (*lightness <= 0.5)
    *saturation=c/(2.0*(*lightness));
  else
    *saturation=c/(2.0-2.0*(*lightness));
}