ico_read_png (FILE    *fp,
              guint32  header,
              guchar  *buffer,
              gint     maxsize,
              gint    *width,
              gint    *height)
{
  png_structp   png_ptr;
  png_infop     info;
  png_uint_32   w;
  png_uint_32   h;
  gint32        bit_depth;
  gint32        color_type;
  guint32     **rows;
  gint          i;

  png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (! png_ptr )
    return FALSE;
  info = png_create_info_struct (png_ptr);
  if (! info )
    {
      png_destroy_read_struct (&png_ptr, NULL, NULL);
      return FALSE;
    }

  if (setjmp (png_jmpbuf (png_ptr)))
    {
      png_destroy_read_struct (&png_ptr, &info, NULL);
      return FALSE;
    }

  png_init_io (png_ptr, fp);
  png_set_sig_bytes (png_ptr, 4);
  png_read_info (png_ptr, info);
  png_get_IHDR (png_ptr, info, &w, &h, &bit_depth, &color_type,
                NULL, NULL, NULL);
  if (w*h*4 > maxsize)
    {
      png_destroy_read_struct (&png_ptr, &info, NULL);
      return FALSE;
    }
  D(("ico_read_png: %ix%i, %i bits, %i type\n", (gint)w, (gint)h,
     bit_depth, color_type));
  switch (color_type)
    {
    case PNG_COLOR_TYPE_GRAY:
      png_set_expand_gray_1_2_4_to_8 (png_ptr);
      if ( bit_depth == 16 )
        png_set_strip_16 (png_ptr);
      png_set_gray_to_rgb (png_ptr);
      png_set_add_alpha (png_ptr, 0xff, PNG_FILLER_AFTER);
      break;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
      png_set_expand_gray_1_2_4_to_8 (png_ptr);
      if ( bit_depth == 16 )
        png_set_strip_16 (png_ptr);
      png_set_gray_to_rgb (png_ptr);
      break;
    case PNG_COLOR_TYPE_PALETTE:
      png_set_palette_to_rgb (png_ptr);
      if (png_get_valid (png_ptr, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha (png_ptr);
      else
        png_set_add_alpha (png_ptr, 0xff, PNG_FILLER_AFTER);
      break;
    case PNG_COLOR_TYPE_RGB:
      if (bit_depth == 16)
        png_set_strip_16 (png_ptr);
      png_set_add_alpha (png_ptr, 0xff, PNG_FILLER_AFTER);
      break;
    case PNG_COLOR_TYPE_RGB_ALPHA:
      if (bit_depth == 16)
        png_set_strip_16 (png_ptr);
      break;
    }

  *width = w;
  *height = h;
  rows = g_new (guint32*, h);
  rows[0] = (guint32*) buffer;
  for (i = 1; i < h; i++)
    rows[i] = rows[i-1] + w;
  png_read_image (png_ptr, (png_bytepp) rows);
  png_destroy_read_struct (&png_ptr, &info, NULL);
  g_free (rows);
  return TRUE;
}