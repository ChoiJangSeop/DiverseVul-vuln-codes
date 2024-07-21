gdk_pixbuf_from_pixdata (const GdkPixdata *pixdata,
			 gboolean          copy_pixels,
			 GError          **error)
{
  guint encoding, bpp;
  guint8 *data = NULL;

  g_return_val_if_fail (pixdata != NULL, NULL);
  g_return_val_if_fail (pixdata->width > 0, NULL);
  g_return_val_if_fail (pixdata->height > 0, NULL);
  g_return_val_if_fail (pixdata->rowstride >= pixdata->width, NULL);
  g_return_val_if_fail ((pixdata->pixdata_type & GDK_PIXDATA_COLOR_TYPE_MASK) == GDK_PIXDATA_COLOR_TYPE_RGB ||
			(pixdata->pixdata_type & GDK_PIXDATA_COLOR_TYPE_MASK) == GDK_PIXDATA_COLOR_TYPE_RGBA, NULL);
  g_return_val_if_fail ((pixdata->pixdata_type & GDK_PIXDATA_SAMPLE_WIDTH_MASK) == GDK_PIXDATA_SAMPLE_WIDTH_8, NULL);
  g_return_val_if_fail ((pixdata->pixdata_type & GDK_PIXDATA_ENCODING_MASK) == GDK_PIXDATA_ENCODING_RAW ||
			(pixdata->pixdata_type & GDK_PIXDATA_ENCODING_MASK) == GDK_PIXDATA_ENCODING_RLE, NULL);
  g_return_val_if_fail (pixdata->pixel_data != NULL, NULL);

  bpp = (pixdata->pixdata_type & GDK_PIXDATA_COLOR_TYPE_MASK) == GDK_PIXDATA_COLOR_TYPE_RGB ? 3 : 4;
  encoding = pixdata->pixdata_type & GDK_PIXDATA_ENCODING_MASK;
  if (encoding == GDK_PIXDATA_ENCODING_RLE)
    copy_pixels = TRUE;
  if (copy_pixels)
    {
      data = g_try_malloc (pixdata->rowstride * pixdata->height);
      if (!data)
	{
	  g_set_error (error, GDK_PIXBUF_ERROR,
		       GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
		       g_dngettext(GETTEXT_PACKAGE,
				   "failed to allocate image buffer of %u byte",
				   "failed to allocate image buffer of %u bytes",
				   pixdata->rowstride * pixdata->height),
		       pixdata->rowstride * pixdata->height);
	  return NULL;
	}
    }
  if (encoding == GDK_PIXDATA_ENCODING_RLE)
    {
      const guint8 *rle_buffer = pixdata->pixel_data;
      guint8 *image_buffer = data;
      guint8 *image_limit = data + pixdata->rowstride * pixdata->height;
      gboolean check_overrun = FALSE;

      while (image_buffer < image_limit)
	{
	  guint length = *(rle_buffer++);

	  if (length & 128)
	    {
	      length = length - 128;
	      check_overrun = image_buffer + length * bpp > image_limit;
	      if (check_overrun)
		length = (image_limit - image_buffer) / bpp;
	      if (bpp < 4)	/* RGB */
		do
		  {
		    memcpy (image_buffer, rle_buffer, 3);
		    image_buffer += 3;
		  }
		while (--length);
	      else		/* RGBA */
		do
		  {
		    memcpy (image_buffer, rle_buffer, 4);
		    image_buffer += 4;
		  }
		while (--length);
	      rle_buffer += bpp;
	    }
	  else
	    {
	      length *= bpp;
	      check_overrun = image_buffer + length > image_limit;
	      if (check_overrun)
		length = image_limit - image_buffer;
	      memcpy (image_buffer, rle_buffer, length);
	      image_buffer += length;
	      rle_buffer += length;
	    }
	}
      if (check_overrun)
	{
	  g_free (data);
	  g_set_error_literal (error, GDK_PIXBUF_ERROR,
                               GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                               _("Image pixel data corrupt"));
	  return NULL;
	}
    }
  else if (copy_pixels)
    memcpy (data, pixdata->pixel_data, pixdata->rowstride * pixdata->height);
  else
    data = pixdata->pixel_data;

  return gdk_pixbuf_new_from_data (data, GDK_COLORSPACE_RGB,
				   (pixdata->pixdata_type & GDK_PIXDATA_COLOR_TYPE_MASK) == GDK_PIXDATA_COLOR_TYPE_RGBA,
				   8, pixdata->width, pixdata->height, pixdata->rowstride,
				   copy_pixels ? (GdkPixbufDestroyNotify) g_free : NULL, data);
}