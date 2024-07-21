ico_read_size (FILE        *fp,
               IcoLoadInfo *info)
{
  png_structp png_ptr;
  png_infop   info_ptr;
  png_uint_32 w, h;
  gint32      bpp;
  gint32      color_type;
  guint32     magic;

  if ( fseek (fp, info->offset, SEEK_SET) < 0 )
    return FALSE;

  ico_read_int32 (fp, &magic, 1);
  if (magic == ICO_PNG_MAGIC)
    {
      png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL,
                                        NULL);
      if (! png_ptr )
        return FALSE;
      info_ptr = png_create_info_struct (png_ptr);
      if (! info_ptr )
        {
          png_destroy_read_struct (&png_ptr, NULL, NULL);
          return FALSE;
        }
      if (setjmp (png_jmpbuf (png_ptr)))
        {
          png_destroy_read_struct (&png_ptr, NULL, NULL);
          return FALSE;
        }
      png_init_io (png_ptr, fp);
      png_set_sig_bytes (png_ptr, 4);
      png_read_info (png_ptr, info_ptr);
      png_get_IHDR (png_ptr, info_ptr, &w, &h, &bpp, &color_type,
                    NULL, NULL, NULL);
      png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
      info->width = w;
      info->height = h;
      D(("ico_read_size: PNG: %ix%i\n", info->width, info->height));
      return TRUE;
    }
  else if (magic == 40)
    {
      if (ico_read_int32 (fp, &info->width, 1)
          && ico_read_int32 (fp, &info->height, 1))
        {
          info->height /= 2;
          D(("ico_read_size: ICO: %ix%i\n", info->width, info->height));
          return TRUE;
        }
      else
        {
          info->width = 0;
          info->height = 0;
          return FALSE;
        }
    }
  return FALSE;
}