ico_load_layer (FILE        *fp,
                gint32       image,
                gint32       icon_num,
                guchar      *buffer,
                gint         maxsize,
                IcoLoadInfo *info)
{
  gint          width, height;
  gint32        layer;
  guint32       first_bytes;
  GimpDrawable *drawable;
  GimpPixelRgn  pixel_rgn;
  gchar         buf [ICO_MAXBUF];

  if ( fseek (fp, info->offset, SEEK_SET) < 0
       || !ico_read_int32 (fp, &first_bytes, 1) )
    return -1;

  if (first_bytes == ICO_PNG_MAGIC)
    {
      if (!ico_read_png (fp, first_bytes, buffer, maxsize, &width, &height))
        return -1;
    }
  else if (first_bytes == 40)
    {
      if (!ico_read_icon (fp, first_bytes, buffer, maxsize, &width, &height))
        return -1;
    }
  else
    {
      return -1;
    }


  /* read successfully. add to image */
  g_snprintf (buf, sizeof (buf), _("Icon #%i"), icon_num+1);
  layer = gimp_layer_new (image, buf, width, height,
                          GIMP_RGBA_IMAGE, 100, GIMP_NORMAL_MODE);
  gimp_image_insert_layer (image, layer, -1, icon_num);
  drawable = gimp_drawable_get (layer);
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
                       drawable->width, drawable->height, TRUE, FALSE);
  gimp_pixel_rgn_set_rect (&pixel_rgn, (const guchar*) buffer,
                           0, 0, drawable->width,
                           drawable->height);
  gimp_drawable_detach (drawable);

  return layer;
}