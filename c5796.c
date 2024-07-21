load_image (const gchar  *filename,
            GError      **error)
{
  FILE *f;
  struct stat st;
  char buf[32];
  PSPimage ia;
  guint32 block_init_len, block_total_len;
  long block_start;
  PSPBlockID id = -1;
  gint block_number;

  gint32 image_ID = -1;

  if (g_stat (filename, &st) == -1)
    return -1;

  f = g_fopen (filename, "rb");
  if (f == NULL)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  /* Read the PSP File Header and determine file version */
  if (fread (buf, 32, 1, f) < 1
      || fread (&psp_ver_major, 2, 1, f) < 1
      || fread (&psp_ver_minor, 2, 1, f) < 1)
    {
      g_message ("Error reading file header");
      goto error;
    }

  if (memcmp (buf, "Paint Shop Pro Image File\n\032\0\0\0\0\0", 32) != 0)
    {
      g_message ("Incorrect file signature");
      goto error;
    }

  psp_ver_major = GUINT16_FROM_LE (psp_ver_major);
  psp_ver_minor = GUINT16_FROM_LE (psp_ver_minor);

  /* I only have the documentation for file format version 3.0,
   * but PSP 6 writes version 4.0. Let's hope it's backwards compatible.
   * Earlier versions probably don't have all the fields I expect
   * so don't accept those.
   */
  if (psp_ver_major < 3)
    {
      g_message ("Unsupported PSP file format version "
                 "%d.%d, only knows 3.0 (and later?)",
                 psp_ver_major, psp_ver_minor);
      goto error;
    }
  else if ((psp_ver_major == 3)
        || (psp_ver_major == 4)
        || (psp_ver_major == 5)
        || (psp_ver_major == 6))
    ; /* OK */
  else
    {
      g_message ("Unsupported PSP file format version %d.%d",
                 psp_ver_major, psp_ver_minor);
      goto error;
    }

  /* Read all the blocks */
  block_number = 0;

  IFDBG(3) g_message ("size = %d", (int)st.st_size);
  while (ftell (f) != st.st_size
         && (id = read_block_header (f, &block_init_len,
                                     &block_total_len)) != -1)
    {
      block_start = ftell (f);

      if (id == PSP_IMAGE_BLOCK)
        {
          if (block_number != 0)
            {
              g_message ("Duplicate General Image Attributes block");
              goto error;
            }
          if (read_general_image_attribute_block (f, block_init_len,
                                                  block_total_len, &ia) == -1)
            {
              goto error;
            }

          IFDBG(2) g_message ("%d dpi %dx%d %s",
                              (int) ia.resolution,
                              ia.width, ia.height,
                              compression_name (ia.compression));

          image_ID = gimp_image_new (ia.width, ia.height,
                                     ia.grayscale ? GIMP_GRAY : GIMP_RGB);
          if (image_ID == -1)
            {
              goto error;
            }

          gimp_image_set_filename (image_ID, filename);

          gimp_image_set_resolution (image_ID, ia.resolution, ia.resolution);
        }
      else
        {
          if (block_number == 0)
            {
              g_message ("Missing General Image Attributes block");
              goto error;
            }

          switch (id)
            {
            case PSP_CREATOR_BLOCK:
              if (read_creator_block (f, image_ID, block_total_len, &ia) == -1)
                goto error;
              break;

            case PSP_COLOR_BLOCK:
              break;            /* Not yet implemented */

            case PSP_LAYER_START_BLOCK:
              if (read_layer_block (f, image_ID, block_total_len, &ia) == -1)
                goto error;
              break;

            case PSP_SELECTION_BLOCK:
              break;            /* Not yet implemented */

            case PSP_ALPHA_BANK_BLOCK:
              break;            /* Not yet implemented */

            case PSP_THUMBNAIL_BLOCK:
              break;            /* No use for it */

            case PSP_EXTENDED_DATA_BLOCK:
              break;            /* Not yet implemented */

            case PSP_TUBE_BLOCK:
              if (read_tube_block (f, image_ID, block_total_len, &ia) == -1)
                goto error;
              break;

            case PSP_COMPOSITE_IMAGE_BANK_BLOCK:
              break;            /* Not yet implemented */

            case PSP_LAYER_BLOCK:
            case PSP_CHANNEL_BLOCK:
            case PSP_ALPHA_CHANNEL_BLOCK:
            case PSP_ADJUSTMENT_EXTENSION_BLOCK:
            case PSP_VECTOR_EXTENSION_BLOCK:
            case PSP_SHAPE_BLOCK:
            case PSP_PAINTSTYLE_BLOCK:
            case PSP_COMPOSITE_ATTRIBUTES_BLOCK:
            case PSP_JPEG_BLOCK:
              g_message ("Sub-block %s should not occur "
                         "at main level of file",
                         block_name (id));
              break;

            default:
              g_message ("Unrecognized block id %d", id);
              break;
            }
        }

      if (block_start + block_total_len >= st.st_size)
        break;

      if (try_fseek (f, block_start + block_total_len, SEEK_SET) < 0)
        goto error;

      block_number++;
    }

  if (id == -1)
    {
    error:
      fclose (f);
      if (image_ID != -1)
        gimp_image_delete (image_ID);
      return -1;
    }

  fclose (f);

  return image_ID;
}