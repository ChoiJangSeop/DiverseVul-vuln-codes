static void InsertRow(Image *image,ssize_t depth,unsigned char *p,ssize_t y,
  ExceptionInfo *exception)
{
  size_t bit; ssize_t x;
  register Quantum *q;
  Quantum index;

  index=0;
  switch (depth)
  {
    case 1:  /* Convert bitmap scanline. */
      {
        q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
        if (q == (Quantum *) NULL)
          break;
        for (x=0; x < ((ssize_t) image->columns-7); x+=8)
        {
          for (bit=0; bit < 8; bit++)
          {
            index=(Quantum) ((((*p) & (0x80 >> bit)) != 0) ? 0x01 : 0x00);
            SetPixelIndex(image,index,q);
            q+=GetPixelChannels(image);
          }
          p++;
        }
        if ((image->columns % 8) != 0)
          {
            for (bit=0; bit < (image->columns % 8); bit++)
              {
                index=(Quantum) ((((*p) & (0x80 >> bit)) != 0) ? 0x01 : 0x00);
                SetPixelIndex(image,index,q);
                q+=GetPixelChannels(image);
              }
            p++;
          }
        (void) SyncAuthenticPixels(image,exception);
        break;
      }
    case 2:  /* Convert PseudoColor scanline. */
      {
        q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
        if (q == (Quantum *) NULL)
          break;
        for (x=0; x < ((ssize_t) image->columns-1); x+=2)
        {
          index=ConstrainColormapIndex(image,(*p >> 6) & 0x3,exception);
          SetPixelIndex(image,index,q);
          q+=GetPixelChannels(image);
          index=ConstrainColormapIndex(image,(*p >> 4) & 0x3,exception);
          SetPixelIndex(image,index,q);
          q+=GetPixelChannels(image);
          index=ConstrainColormapIndex(image,(*p >> 2) & 0x3,exception);
          SetPixelIndex(image,index,q);
          q+=GetPixelChannels(image);
          index=ConstrainColormapIndex(image,(*p) & 0x3,exception);
          SetPixelIndex(image,index,q);
          q+=GetPixelChannels(image);
          p++;
        }
        if ((image->columns % 4) != 0)
          {
            index=ConstrainColormapIndex(image,(*p >> 6) & 0x3,exception);
            SetPixelIndex(image,index,q);
            q+=GetPixelChannels(image);
            if ((image->columns % 4) >= 1)

              {
                index=ConstrainColormapIndex(image,(*p >> 4) & 0x3,exception);
                SetPixelIndex(image,index,q);
                q+=GetPixelChannels(image);
                if ((image->columns % 4) >= 2)

                  {
                    index=ConstrainColormapIndex(image,(*p >> 2) & 0x3,
                      exception);
                    SetPixelIndex(image,index,q);
                    q+=GetPixelChannels(image);
                  }
              }
            p++;
          }
        (void) SyncAuthenticPixels(image,exception);
        break;
      }

    case 4:  /* Convert PseudoColor scanline. */
      {
        q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
        if (q == (Quantum *) NULL)
          break;
        for (x=0; x < ((ssize_t) image->columns-1); x+=2)
        {
            index=ConstrainColormapIndex(image,(*p >> 4) & 0xf,exception);
            SetPixelIndex(image,index,q);
            q+=GetPixelChannels(image);
            index=ConstrainColormapIndex(image,(*p) & 0xf,exception);
            SetPixelIndex(image,index,q);
            q+=GetPixelChannels(image);
            p++;
          }
        if ((image->columns % 2) != 0)
          {
            index=ConstrainColormapIndex(image,(*p >> 4) & 0xf,exception);
            SetPixelIndex(image,index,q);
            q+=GetPixelChannels(image);
            p++;
          }
        (void) SyncAuthenticPixels(image,exception);
        break;
      }
    case 8: /* Convert PseudoColor scanline. */
      {
        q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
        if (q == (Quantum *) NULL)
          break;
        for (x=0; x < (ssize_t) image->columns; x++)
        {
          index=ConstrainColormapIndex(image,*p,exception);
          SetPixelIndex(image,index,q);
          p++;
          q+=GetPixelChannels(image);
        }
        (void) SyncAuthenticPixels(image,exception);
        break;
      }
    }
}