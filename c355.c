read_header_block (PSDimage  *img_a,
                   FILE      *f,
                   GError   **error)
{
  guint16  version;
  gchar    sig[4];
  gchar    buf[6];

  if (fread (sig, 4, 1, f) < 1
      || fread (&version, 2, 1, f) < 1
      || fread (buf, 6, 1, f) < 1
      || fread (&img_a->channels, 2, 1, f) < 1
      || fread (&img_a->rows, 4, 1, f) < 1
      || fread (&img_a->columns, 4, 1, f) < 1
      || fread (&img_a->bps, 2, 1, f) < 1
      || fread (&img_a->color_mode, 2, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }
  version = GUINT16_FROM_BE (version);
  img_a->channels = GUINT16_FROM_BE (img_a->channels);
  img_a->rows = GUINT32_FROM_BE (img_a->rows);
  img_a->columns = GUINT32_FROM_BE (img_a->columns);
  img_a->bps = GUINT16_FROM_BE (img_a->bps);
  img_a->color_mode = GUINT16_FROM_BE (img_a->color_mode);

  IFDBG(1) g_debug ("\n\n\tSig: %.4s\n\tVer: %d\n\tChannels: "
                    "%d\n\tSize: %dx%d\n\tBPS: %d\n\tMode: %d\n",
                    sig, version, img_a->channels,
                    img_a->columns, img_a->rows,
                    img_a->bps, img_a->color_mode);

  if (memcmp (sig, "8BPS", 4) != 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                  _("Not a valid photoshop document file"));
      return -1;
    }

  if (version != 1)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                  _("Unsupported file format version: %d"), version);
      return -1;
    }

  if (img_a->channels > MAX_CHANNELS)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                  _("Too many channels in file: %d"), img_a->channels);
      return -1;
    }

    /* Photoshop CS (version 8) supports 300000 x 300000, but this
       is currently larger than GIMP_MAX_IMAGE_SIZE */

  if (img_a->rows < 1 || img_a->rows > GIMP_MAX_IMAGE_SIZE)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                  _("Unsupported or invalid image height: %d"),
                  img_a->rows);
      return -1;
    }

  if (img_a->columns < 1 || img_a->columns > GIMP_MAX_IMAGE_SIZE)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                  _("Unsupported or invalid image width: %d"),
                  img_a->columns);
      return -1;
    }

  if (img_a->color_mode != PSD_BITMAP
      && img_a->color_mode != PSD_GRAYSCALE
      && img_a->color_mode != PSD_INDEXED
      && img_a->color_mode != PSD_RGB
      && img_a->color_mode != PSD_DUOTONE)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Unsupported color mode: %s"),
                   get_psd_color_mode_name (img_a->color_mode));
      return -1;
    }

  /* Warnings for format conversions */
  switch (img_a->bps)
    {
      case 16:
        IFDBG(3) g_debug ("16 Bit Data");
        if (CONVERSION_WARNINGS)
          g_message (_("Warning:\n"
                       "The image you are loading has 16 bits per channel. GIMP "
                       "can only handle 8 bit, so it will be converted for you. "
                       "Information will be lost because of this conversion."));
        break;

      case 8:
        IFDBG(3) g_debug ("8 Bit Data");
        break;

      case 1:
        IFDBG(3) g_debug ("1 Bit Data");
        break;

      default:
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                    _("Unsupported bit depth: %d"), img_a->bps);
        return -1;
        break;
    }

  return 0;
}