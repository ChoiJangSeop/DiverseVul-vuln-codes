ico_read_icon (FILE    *fp,
               guint32  header_size,
               guchar  *buffer,
               gint     maxsize,
               gint    *width,
               gint    *height)
{
  IcoFileDataHeader   data;
  gint                length;
  gint                x, y, w, h;
  guchar             *xor_map, *and_map;
  guint32            *palette;
  guint32            *dest_vec;
  guchar             *row;
  gint                rowstride;

  palette = NULL;

  data.header_size = header_size;
  ico_read_int32 (fp, &data.width, 1);
  ico_read_int32 (fp, &data.height, 1);
  ico_read_int16 (fp, &data.planes, 1);
  ico_read_int16 (fp, &data.bpp, 1);
  ico_read_int32 (fp, &data.compression, 1);
  ico_read_int32 (fp, &data.image_size, 1);
  ico_read_int32 (fp, &data.x_res, 1);
  ico_read_int32 (fp, &data.y_res, 1);
  ico_read_int32 (fp, &data.used_clrs, 1);
  ico_read_int32 (fp, &data.important_clrs, 1);

  D(("  header size %i, "
     "w %i, h %i, planes %i, size %i, bpp %i, used %i, imp %i.\n",
     data.header_size, data.width, data.height,
     data.planes, data.image_size, data.bpp,
     data.used_clrs, data.important_clrs));

  if (data.planes != 1
      || data.compression != 0)
    {
      D(("skipping image: invalid header\n"));
      return FALSE;
    }

  if (data.bpp != 1 && data.bpp != 4
      && data.bpp != 8 && data.bpp != 24
      && data.bpp != 32)
    {
      D(("skipping image: invalid depth: %i\n", data.bpp));
      return FALSE;
    }

  if (data.width * data.height * 2 > maxsize)
    {
      D(("skipping image: too large\n"));
      return FALSE;
    }

  w = data.width;
  h = data.height / 2;

  if (data.bpp <= 8)
    {
      if (data.used_clrs == 0)
        data.used_clrs = (1 << data.bpp);

      D(("  allocating a %i-slot palette for %i bpp.\n",
         data.used_clrs, data.bpp));

      palette = g_new0 (guint32, data.used_clrs);
      ico_read_int8 (fp, (guint8 *) palette, data.used_clrs * 4);
    }

  xor_map = ico_alloc_map (w, h, data.bpp, &length);
  ico_read_int8 (fp, xor_map, length);
  D(("  length of xor_map: %i\n", length));

  /* Read in and_map. It's padded out to 32 bits per line: */
  and_map = ico_alloc_map (w, h, 1, &length);
  ico_read_int8 (fp, and_map, length);
  D(("  length of and_map: %i\n", length));

  dest_vec = (guint32 *) buffer;
  switch (data.bpp)
    {
    case 1:
      for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
          {
            guint32 color = palette[ico_get_bit_from_data (xor_map,
                                                           w, y * w + x)];
            guint32 *dest = dest_vec + (h - 1 - y) * w + x;

            R_VAL_GIMP (dest) = R_VAL (&color);
            G_VAL_GIMP (dest) = G_VAL (&color);
            B_VAL_GIMP (dest) = B_VAL (&color);

            if (ico_get_bit_from_data (and_map, w, y * w + x))
              A_VAL_GIMP (dest) = 0;
            else
              A_VAL_GIMP (dest) = 255;
          }
      break;

    case 4:
      for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
          {
            guint32 color = palette[ico_get_nibble_from_data (xor_map,
                                                              w, y * w + x)];
            guint32 *dest = dest_vec + (h - 1 - y) * w + x;

            R_VAL_GIMP (dest) = R_VAL (&color);
            G_VAL_GIMP (dest) = G_VAL (&color);
            B_VAL_GIMP (dest) = B_VAL (&color);

            if (ico_get_bit_from_data (and_map, w, y * w + x))
              A_VAL_GIMP (dest) = 0;
            else
              A_VAL_GIMP (dest) = 255;
          }
      break;

    case 8:
      for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
          {
            guint32 color = palette[ico_get_byte_from_data (xor_map,
                                                            w, y * w + x)];
            guint32 *dest = dest_vec + (h - 1 - y) * w + x;

            R_VAL_GIMP (dest) = R_VAL (&color);
            G_VAL_GIMP (dest) = G_VAL (&color);
            B_VAL_GIMP (dest) = B_VAL (&color);

            if (ico_get_bit_from_data (and_map, w, y * w + x))
              A_VAL_GIMP (dest) = 0;
            else
              A_VAL_GIMP (dest) = 255;
          }
      break;

    default:
      {
        gint bytespp = data.bpp / 8;

        rowstride = ico_rowstride (w, data.bpp);

        for (y = 0; y < h; y++)
          {
            row = xor_map + rowstride * y;

            for (x = 0; x < w; x++)
              {
                guint32 *dest = dest_vec + (h - 1 - y) * w + x;

                B_VAL_GIMP (dest) = row[0];
                G_VAL_GIMP (dest) = row[1];
                R_VAL_GIMP (dest) = row[2];

                if (data.bpp < 32)
                  {
                    if (ico_get_bit_from_data (and_map, w, y * w + x))
                      A_VAL_GIMP (dest) = 0;
                    else
                      A_VAL_GIMP (dest) = 255;
                  }
                else
                  {
                    A_VAL_GIMP (dest) = row[3];
                  }

                row += bytespp;
              }
          }
      }
    }
  if (palette)
    g_free (palette);
  g_free (xor_map);
  g_free (and_map);
  *width = w;
  *height = h;
  return TRUE;
}