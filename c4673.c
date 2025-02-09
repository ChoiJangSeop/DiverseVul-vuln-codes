static void ImportGrayQuantum(const Image *image,QuantumInfo *quantum_info,
  const MagickSizeType number_pixels,const unsigned char *magick_restrict p,
  Quantum *magick_restrict q,ExceptionInfo *exception)
{
  QuantumAny
    range;

  register ssize_t
    x;

  ssize_t
    bit;

  unsigned int
    pixel;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  switch (quantum_info->depth)
  {
    case 1:
    {
      register Quantum
        black,
        white;

      black=0;
      white=QuantumRange;
      if (quantum_info->min_is_white != MagickFalse)
        {
          black=QuantumRange;
          white=0;
        }
      for (x=0; x < ((ssize_t) number_pixels-7); x+=8)
      {
        for (bit=0; bit < 8; bit++)
        {
          SetPixelGray(image,((*p) & (1 << (7-bit))) == 0 ? black : white,q);
          q+=GetPixelChannels(image);
        }
        p++;
      }
      for (bit=0; bit < (ssize_t) (number_pixels % 8); bit++)
      {
        SetPixelGray(image,((*p) & (0x01 << (7-bit))) == 0 ? black : white,q);
        q+=GetPixelChannels(image);
      }
      if (bit != 0)
        p++;
      break;
    }
    case 4:
    {
      register unsigned char
        pixel;

      range=GetQuantumRange(quantum_info->depth);
      for (x=0; x < ((ssize_t) number_pixels-1); x+=2)
      {
        pixel=(unsigned char) ((*p >> 4) & 0xf);
        SetPixelGray(image,ScaleAnyToQuantum(pixel,range),q);
        q+=GetPixelChannels(image);
        pixel=(unsigned char) ((*p) & 0xf);
        SetPixelGray(image,ScaleAnyToQuantum(pixel,range),q);
        p++;
        q+=GetPixelChannels(image);
      }
      for (bit=0; bit < (ssize_t) (number_pixels % 2); bit++)
      {
        pixel=(unsigned char) (*p++ >> 4);
        SetPixelGray(image,ScaleAnyToQuantum(pixel,range),q);
        q+=GetPixelChannels(image);
      }
      break;
    }
    case 8:
    {
      unsigned char
        pixel;

      if (quantum_info->min_is_white != MagickFalse)
        {
          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            p=PushCharPixel(p,&pixel);
            SetPixelGray(image,ScaleCharToQuantum(pixel),q);
            SetPixelAlpha(image,OpaqueAlpha,q);
            p+=quantum_info->pad;
            q+=GetPixelChannels(image);
          }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushCharPixel(p,&pixel);
        SetPixelGray(image,ScaleCharToQuantum(pixel),q);
        SetPixelAlpha(image,OpaqueAlpha,q);
        p+=quantum_info->pad;
        q+=GetPixelChannels(image);
      }
      break;
    }
    case 10:
    {
      range=GetQuantumRange(quantum_info->depth);
      if (quantum_info->pack == MagickFalse)
        {
          if (image->endian == LSBEndian)
            {
              for (x=0; x < (ssize_t) (number_pixels-2); x+=3)
              {
                p=PushLongPixel(quantum_info->endian,p,&pixel);
                SetPixelGray(image,ScaleAnyToQuantum((pixel >> 22) & 0x3ff,
                  range),q);
                q+=GetPixelChannels(image);
                SetPixelGray(image,ScaleAnyToQuantum((pixel >> 12) & 0x3ff,
                  range),q);
                q+=GetPixelChannels(image);
                SetPixelGray(image,ScaleAnyToQuantum((pixel >> 2) & 0x3ff,
                  range),q);
                p+=quantum_info->pad;
                q+=GetPixelChannels(image);
              }
              p=PushLongPixel(quantum_info->endian,p,&pixel);
              if (x++ < (ssize_t) (number_pixels-1))
                {
                  SetPixelGray(image,ScaleAnyToQuantum((pixel >> 22) & 0x3ff,
                    range),q);
                  q+=GetPixelChannels(image);
                }
              if (x++ < (ssize_t) number_pixels)
                {
                  SetPixelGray(image,ScaleAnyToQuantum((pixel >> 12) & 0x3ff,
                    range),q);
                  q+=GetPixelChannels(image);
                }
              break;
            }
          for (x=0; x < (ssize_t) (number_pixels-2); x+=3)
          {
            p=PushLongPixel(quantum_info->endian,p,&pixel);
            SetPixelGray(image,ScaleAnyToQuantum((pixel >> 2) & 0x3ff,range),
              q);
            q+=GetPixelChannels(image);
            SetPixelGray(image,ScaleAnyToQuantum((pixel >> 12) & 0x3ff,range),
              q);
            q+=GetPixelChannels(image);
            SetPixelGray(image,ScaleAnyToQuantum((pixel >> 22) & 0x3ff,range),
              q);
            p+=quantum_info->pad;
            q+=GetPixelChannels(image);
          }
          p=PushLongPixel(quantum_info->endian,p,&pixel);
          if (x++ < (ssize_t) (number_pixels-1))
            {
              SetPixelGray(image,ScaleAnyToQuantum((pixel >> 2) & 0x3ff,
                range),q);
              q+=GetPixelChannels(image);
            }
          if (x++ < (ssize_t) number_pixels)
            {
              SetPixelGray(image,ScaleAnyToQuantum((pixel >> 12) & 0x3ff,
                range),q);
              q+=GetPixelChannels(image);
            }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelGray(image,ScaleAnyToQuantum(pixel,range),q);
        p+=quantum_info->pad;
        q+=GetPixelChannels(image);
      }
      break;
    }
    case 12:
    {
      range=GetQuantumRange(quantum_info->depth);
      if (quantum_info->pack == MagickFalse)
        {
          unsigned short
            pixel;

          for (x=0; x < (ssize_t) (number_pixels-1); x+=2)
          {
            p=PushShortPixel(quantum_info->endian,p,&pixel);
            SetPixelGray(image,ScaleAnyToQuantum((QuantumAny) (pixel >> 4),
              range),q);
            q+=GetPixelChannels(image);
            p=PushShortPixel(quantum_info->endian,p,&pixel);
            SetPixelGray(image,ScaleAnyToQuantum((QuantumAny) (pixel >> 4),
              range),q);
            p+=quantum_info->pad;
            q+=GetPixelChannels(image);
          }
          for (bit=0; bit < (ssize_t) (number_pixels % 2); bit++)
          {
            p=PushShortPixel(quantum_info->endian,p,&pixel);
            SetPixelGray(image,ScaleAnyToQuantum((QuantumAny) (pixel >> 4),
              range),q);
            p+=quantum_info->pad;
            q+=GetPixelChannels(image);
          }
          if (bit != 0)
            p++;
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelGray(image,ScaleAnyToQuantum(pixel,range),q);
        p+=quantum_info->pad;
        q+=GetPixelChannels(image);
      }
      break;
    }
    case 16:
    {
      unsigned short
        pixel;

      if (quantum_info->min_is_white != MagickFalse)
        {
          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            p=PushShortPixel(quantum_info->endian,p,&pixel);
            SetPixelGray(image,ScaleShortToQuantum(pixel),q);
            p+=quantum_info->pad;
            q+=GetPixelChannels(image);
          }
          break;
        }
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            p=PushShortPixel(quantum_info->endian,p,&pixel);
            SetPixelGray(image,ClampToQuantum(QuantumRange*
              HalfToSinglePrecision(pixel)),q);
            p+=quantum_info->pad;
            q+=GetPixelChannels(image);
          }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushShortPixel(quantum_info->endian,p,&pixel);
        SetPixelGray(image,ScaleShortToQuantum(pixel),q);
        p+=quantum_info->pad;
        q+=GetPixelChannels(image);
      }
      break;
    }
    case 32:
    {
      unsigned int
        pixel;

      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          float
            pixel;

          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            p=PushFloatPixel(quantum_info,p,&pixel);
            SetPixelGray(image,ClampToQuantum(pixel),q);
            p+=quantum_info->pad;
            q+=GetPixelChannels(image);
          }
          break;
        }
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushLongPixel(quantum_info->endian,p,&pixel);
        SetPixelGray(image,ScaleLongToQuantum(pixel),q);
        p+=quantum_info->pad;
        q+=GetPixelChannels(image);
      }
      break;
    }
    case 64:
    {
      if (quantum_info->format == FloatingPointQuantumFormat)
        {
          double
            pixel;

          for (x=0; x < (ssize_t) number_pixels; x++)
          {
            p=PushDoublePixel(quantum_info,p,&pixel);
            SetPixelGray(image,ClampToQuantum(pixel),q);
            p+=quantum_info->pad;
            q+=GetPixelChannels(image);
          }
          break;
        }
    }
    default:
    {
      range=GetQuantumRange(quantum_info->depth);
      for (x=0; x < (ssize_t) number_pixels; x++)
      {
        p=PushQuantumPixel(quantum_info,p,&pixel);
        SetPixelGray(image,ScaleAnyToQuantum(pixel,range),q);
        p+=quantum_info->pad;
        q+=GetPixelChannels(image);
      }
      break;
    }
  }
}