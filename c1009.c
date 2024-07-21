load_xwd_f1_d24_b1 (const gchar     *filename,
                    FILE            *ifp,
                    L_XWDFILEHEADER *xwdhdr,
                    L_XWDCOLOR      *xwdcolmap)
{
  register guchar *dest, outmask, inmask, do_reverse;
  gint             width, height, i, j, plane, fromright;
  gint             tile_height, tile_start, tile_end;
  gint             indexed, bytes_per_pixel;
  gint             maxred, maxgreen, maxblue;
  gint             red, green, blue, ncols, standard_rgb;
  glong            data_offset, plane_offset, tile_offset;
  gulong           redmask, greenmask, bluemask;
  guint            redshift, greenshift, blueshift;
  gulong           g;
  guchar           redmap[256], greenmap[256], bluemap[256];
  guchar           bit_reverse[256];
  guchar          *xwddata, *xwdin, *data;
  L_CARD32         pixelval;
  PIXEL_MAP        pixel_map;
  gint             err = 0;
  gint32           layer_ID, image_ID;
  GimpPixelRgn     pixel_rgn;
  GimpDrawable    *drawable;

#ifdef XWD_DEBUG
  printf ("load_xwd_f1_d24_b1 (%s)\n", filename);
#endif

  xwddata = g_malloc (xwdhdr->l_bytes_per_line);
  if (xwddata == NULL)
    return -1;

  width           = xwdhdr->l_pixmap_width;
  height          = xwdhdr->l_pixmap_height;
  indexed         = (xwdhdr->l_pixmap_depth <= 8);
  bytes_per_pixel = (indexed ? 1 : 3);

  image_ID = create_new_image (filename, width, height,
                               indexed ? GIMP_INDEXED : GIMP_RGB,
                               &layer_ID, &drawable, &pixel_rgn);

  tile_height = gimp_tile_height ();
  data = g_malloc (tile_height * width * bytes_per_pixel);

  for (j = 0; j < 256; j++)   /* Create an array for reversing bits */
    {
      inmask = 0;
      for (i = 0; i < 8; i++)
        {
          inmask <<= 1;
          if (j & (1 << i)) inmask |= 1;
        }
      bit_reverse[j] = inmask;
    }

  redmask   = xwdhdr->l_red_mask;
  greenmask = xwdhdr->l_green_mask;
  bluemask  = xwdhdr->l_blue_mask;

  if (redmask   == 0) redmask   = 0xff0000;
  if (greenmask == 0) greenmask = 0x00ff00;
  if (bluemask  == 0) bluemask  = 0x0000ff;

  standard_rgb =    (redmask == 0xff0000) && (greenmask == 0x00ff00)
    && (bluemask == 0x0000ff);
  redshift = greenshift = blueshift = 0;

  if (!standard_rgb)   /* Do we need to re-map the pixel-values ? */
    {
      /* How to shift RGB to be right aligned ? */
      /* (We rely on the the mask bits are grouped and not mixed) */

      while (((1 << redshift)   & redmask)   == 0) redshift++;
      while (((1 << greenshift) & greenmask) == 0) greenshift++;
      while (((1 << blueshift)  & bluemask)  == 0) blueshift++;

      /* The bits_per_rgb may not be correct. Use redmask instead */

      maxred = 0; while (redmask >> (redshift + maxred)) maxred++;
      maxred = (1 << maxred) - 1;

      maxgreen = 0; while (greenmask >> (greenshift + maxgreen)) maxgreen++;
      maxgreen = (1 << maxgreen) - 1;

      maxblue = 0; while (bluemask >> (blueshift + maxblue)) maxblue++;
      maxblue = (1 << maxblue) - 1;

      /* Set map-arrays for red, green, blue */
      for (red = 0; red <= maxred; red++)
        redmap[red] = (red * 255) / maxred;
      for (green = 0; green <= maxgreen; green++)
        greenmap[green] = (green * 255) / maxgreen;
      for (blue = 0; blue <= maxblue; blue++)
        bluemap[blue] = (blue * 255) / maxblue;
    }

  ncols = xwdhdr->l_colormap_entries;
  if (xwdhdr->l_ncolors < ncols)
    ncols = xwdhdr->l_ncolors;

  if (indexed)
    {
      if (ncols < 2)
        set_bw_color_table (image_ID);
      else
        set_color_table (image_ID, xwdhdr, xwdcolmap);
    }
  else
    {
      set_pixelmap (ncols, xwdcolmap, &pixel_map);
    }

  do_reverse = !xwdhdr->l_bitmap_bit_order;

  /* This is where the image data starts within the file */
  data_offset = ftell (ifp);

  for (tile_start = 0; tile_start < height; tile_start += tile_height)
    {
      memset (data, 0, width*tile_height*bytes_per_pixel);

      tile_end = tile_start + tile_height - 1;
      if (tile_end >= height)
        tile_end = height - 1;

      for (plane = 0; plane < xwdhdr->l_pixmap_depth; plane++)
        {
          dest = data;    /* Position to start of tile within the plane */
          plane_offset = data_offset + plane*height*xwdhdr->l_bytes_per_line;
          tile_offset = plane_offset + tile_start*xwdhdr->l_bytes_per_line;
          fseek (ifp, tile_offset, SEEK_SET);

          /* Place the last plane at the least significant bit */

          if (indexed)   /* Only 1 byte per pixel */
            {
              fromright = xwdhdr->l_pixmap_depth-1-plane;
              outmask = (1 << fromright);
            }
          else           /* 3 bytes per pixel */
            {
              fromright = xwdhdr->l_pixmap_depth-1-plane;
              dest += 2 - fromright/8;
              outmask = (1 << (fromright % 8));
            }

          for (i = tile_start; i <= tile_end; i++)
            {
              if (fread (xwddata,xwdhdr->l_bytes_per_line,1,ifp) != 1)
                {
                  err = 1;
                  break;
                }
              xwdin = xwddata;

              /* Handle bitmap unit */
              if (xwdhdr->l_bitmap_unit == 16)
                {
                  if (xwdhdr->l_bitmap_bit_order != xwdhdr->l_byte_order)
                    {
                      j = xwdhdr->l_bytes_per_line/2;
                      while (j--)
                        {
                          inmask = xwdin[0]; xwdin[0] = xwdin[1]; xwdin[1] = inmask;
                          xwdin += 2;
                        }
                      xwdin = xwddata;
                    }
                }
              else if (xwdhdr->l_bitmap_unit == 32)
                {
                  if (xwdhdr->l_bitmap_bit_order != xwdhdr->l_byte_order)
                    {
                      j = xwdhdr->l_bytes_per_line/4;
                      while (j--)
                        {
                          inmask = xwdin[0]; xwdin[0] = xwdin[3]; xwdin[3] = inmask;
                          inmask = xwdin[1]; xwdin[1] = xwdin[2]; xwdin[2] = inmask;
                          xwdin += 4;
                        }
                      xwdin = xwddata;
                    }
                }

              g = inmask = 0;
              for (j = 0; j < width; j++)
                {
                  if (!inmask)
                    {
                      g = *(xwdin++);
                      if (do_reverse)
                        g = bit_reverse[g];
                      inmask = 0x80;
                    }

                  if (g & inmask)
                    *dest |= outmask;
                  dest += bytes_per_pixel;

                  inmask >>= 1;
                }
            }
        }

      /* For indexed images, the mapping to colors is done by the color table. */
      /* Otherwise we must do the mapping by ourself. */
      if (!indexed)
        {
          dest = data;
          for (i = tile_start; i <= tile_end; i++)
            {
              for (j = 0; j < width; j++)
                {
                  pixelval = (*dest << 16) | (*(dest+1) << 8) | *(dest+2);

                  if (get_pixelmap (pixelval, &pixel_map, dest, dest+1, dest+2)
                      || standard_rgb)
                    {
                      dest += 3;
                    }
                  else   /* We have to map RGB to 0,...,255 */
                    {
                      *(dest++) = redmap[(pixelval & redmask) >> redshift];
                      *(dest++) = greenmap[(pixelval & greenmask) >> greenshift];
                      *(dest++) = bluemap[(pixelval & bluemask) >> blueshift];
                    }
                }
            }
        }

      gimp_progress_update ((gdouble) tile_end / (gdouble) height);

      gimp_pixel_rgn_set_rect (&pixel_rgn, data, 0, tile_start,
                               width, tile_end-tile_start+1);
    }

  g_free (data);
  g_free (xwddata);

  if (err)
    g_message (_("EOF encountered on reading"));

  gimp_drawable_flush (drawable);

  return err ? -1 : image_ID;
}