ico_load_image (const gchar  *filename,
                GError      **error)
{
  FILE        *fp;
  IcoLoadInfo *info;
  gint         max_width, max_height;
  gint         i;
  gint32       image;
  guchar      *buffer;
  guint        icon_count;
  gint         maxsize;

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  fp = g_fopen (filename, "rb");
  if (! fp )
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  icon_count = ico_read_init (fp);
  if (!icon_count)
    {
      fclose (fp);
      return -1;
    }

  info = ico_read_info (fp, icon_count);
  if (!info)
    {
      fclose (fp);
      return -1;
    }

  /* find width and height of image */
  max_width = 0;
  max_height = 0;
  for (i = 0; i < icon_count; i++)
    {
      if ( info[i].width > max_width )
        max_width = info[i].width;
      if ( info[i].height > max_height )
        max_height = info[i].height;
    }
  if ( max_width <= 0 || max_height <= 0 )
    {
      g_free (info);
      fclose (fp);
      return -1;
    }
  D(("image size: %ix%i\n", max_width, max_height));

  image = gimp_image_new (max_width, max_height, GIMP_RGB);
  gimp_image_set_filename (image, filename);

  maxsize = max_width * max_height * 4;
  buffer = g_new (guchar, max_width * max_height * 4);
  for (i = 0; i < icon_count; i++)
    {
      ico_load_layer (fp, image, i, buffer, maxsize, info+i);
    }
  g_free (buffer);
  g_free (info);
  fclose (fp);

  gimp_progress_update (1.0);

  return image;
}