read_layer_block (PSDimage  *img_a,
                  FILE      *f,
                  GError   **error)
{
  PSDlayer **lyr_a;
  guint32    block_len;
  guint32    block_end;
  guint32    block_rem;
  gint32     read_len;
  gint32     write_len;
  gint       lidx;                  /* Layer index */
  gint       cidx;                  /* Channel index */

  if (fread (&block_len, 4, 1, f) < 1)
    {
      psd_set_error (feof (f), errno, error);
      img_a->num_layers = -1;
      return NULL;
    }
  img_a->mask_layer_len = GUINT32_FROM_BE (block_len);

  IFDBG(1) g_debug ("Layer and mask block size = %d", img_a->mask_layer_len);

  img_a->transparency = FALSE;
  img_a->layer_data_len = 0;

  if (!img_a->mask_layer_len)
    {
      img_a->num_layers = 0;
      return NULL;
    }
  else
    {
      img_a->mask_layer_start = ftell (f);
      block_end = img_a->mask_layer_start + img_a->mask_layer_len;

      /* Get number of layers */
      if (fread (&block_len, 4, 1, f) < 1
          || fread (&img_a->num_layers, 2, 1, f) < 1)
        {
          psd_set_error (feof (f), errno, error);
          img_a->num_layers = -1;
          return NULL;
        }
      img_a->num_layers = GINT16_FROM_BE (img_a->num_layers);
      IFDBG(2) g_debug ("Number of layers: %d", img_a->num_layers);

      if (img_a->num_layers < 0)
        {
          img_a->transparency = TRUE;
          img_a->num_layers = -img_a->num_layers;
        }

      if (img_a->num_layers)
        {
          /* Read layer records */
          PSDlayerres           res_a;

          /* Create pointer array for the layer records */
          lyr_a = g_new (PSDlayer *, img_a->num_layers);
          for (lidx = 0; lidx < img_a->num_layers; ++lidx)
            {
              /* Allocate layer record */
              lyr_a[lidx] = (PSDlayer *) g_malloc (sizeof (PSDlayer) );

              /* Initialise record */
              lyr_a[lidx]->drop = FALSE;
              lyr_a[lidx]->id = 0;

              if (fread (&lyr_a[lidx]->top, 4, 1, f) < 1
                  || fread (&lyr_a[lidx]->left, 4, 1, f) < 1
                  || fread (&lyr_a[lidx]->bottom, 4, 1, f) < 1
                  || fread (&lyr_a[lidx]->right, 4, 1, f) < 1
                  || fread (&lyr_a[lidx]->num_channels, 2, 1, f) < 1)
                {
                  psd_set_error (feof (f), errno, error);
                  return NULL;
                }
              lyr_a[lidx]->top = GUINT32_FROM_BE (lyr_a[lidx]->top);
              lyr_a[lidx]->left = GUINT32_FROM_BE (lyr_a[lidx]->left);
              lyr_a[lidx]->bottom = GUINT32_FROM_BE (lyr_a[lidx]->bottom);
              lyr_a[lidx]->right = GUINT32_FROM_BE (lyr_a[lidx]->right);
              lyr_a[lidx]->num_channels = GUINT16_FROM_BE (lyr_a[lidx]->num_channels);

              if (lyr_a[lidx]->num_channels > MAX_CHANNELS)
                {
                  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                              _("Too many channels in layer: %d"),
                              lyr_a[lidx]->num_channels);
                  return NULL;
                }
              if (lyr_a[lidx]->bottom - lyr_a[lidx]->top > GIMP_MAX_IMAGE_SIZE)
                {
                  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                              _("Unsupported or invalid layer height: %d"),
                              lyr_a[lidx]->bottom - lyr_a[lidx]->top);
                  return NULL;
                }
              if (lyr_a[lidx]->right - lyr_a[lidx]->left > GIMP_MAX_IMAGE_SIZE)
                {
                  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                              _("Unsupported or invalid layer width: %d"),
                              lyr_a[lidx]->right - lyr_a[lidx]->left);
                  return NULL;
                }

              IFDBG(2) g_debug ("Layer %d, Coords %d %d %d %d, channels %d, ",
                                 lidx, lyr_a[lidx]->left, lyr_a[lidx]->top,
                                 lyr_a[lidx]->right, lyr_a[lidx]->bottom,
                                 lyr_a[lidx]->num_channels);

              lyr_a[lidx]->chn_info = g_new (ChannelLengthInfo, lyr_a[lidx]->num_channels);
              for (cidx = 0; cidx < lyr_a[lidx]->num_channels; ++cidx)
                {
                  if (fread (&lyr_a[lidx]->chn_info[cidx].channel_id, 2, 1, f) < 1
                      || fread (&lyr_a[lidx]->chn_info[cidx].data_len, 4, 1, f) < 1)
                    {
                      psd_set_error (feof (f), errno, error);
                      return NULL;
                    }
                  lyr_a[lidx]->chn_info[cidx].channel_id =
                    GINT16_FROM_BE (lyr_a[lidx]->chn_info[cidx].channel_id);
                  lyr_a[lidx]->chn_info[cidx].data_len =
                    GUINT32_FROM_BE (lyr_a[lidx]->chn_info[cidx].data_len);
                  img_a->layer_data_len += lyr_a[lidx]->chn_info[cidx].data_len;
                  IFDBG(3) g_debug ("Channel ID %d, data len %d",
                                     lyr_a[lidx]->chn_info[cidx].channel_id,
                                     lyr_a[lidx]->chn_info[cidx].data_len);
                }

              if (fread (lyr_a[lidx]->mode_key, 4, 1, f) < 1
                  || fread (lyr_a[lidx]->blend_mode, 4, 1, f) < 1
                  || fread (&lyr_a[lidx]->opacity, 1, 1, f) < 1
                  || fread (&lyr_a[lidx]->clipping, 1, 1, f) < 1
                  || fread (&lyr_a[lidx]->flags, 1, 1, f) < 1
                  || fread (&lyr_a[lidx]->filler, 1, 1, f) < 1
                  || fread (&lyr_a[lidx]->extra_len, 4, 1, f) < 1)
                {
                  psd_set_error (feof (f), errno, error);
                  return NULL;
                }
              if (memcmp (lyr_a[lidx]->mode_key, "8BIM", 4) != 0)
                {
                  IFDBG(1) g_debug ("Incorrect layer mode signature %.4s",
                                    lyr_a[lidx]->mode_key);
                  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                              _("The file is corrupt!"));
                  return NULL;
                }

              lyr_a[lidx]->layer_flags.trans_prot = lyr_a[lidx]->flags & 1 ? TRUE : FALSE;
              lyr_a[lidx]->layer_flags.visible = lyr_a[lidx]->flags & 2 ? FALSE : TRUE;
              if (lyr_a[lidx]->flags & 8)
                lyr_a[lidx]->layer_flags.irrelevant = lyr_a[lidx]->flags & 16 ? TRUE : FALSE;
              else
                lyr_a[lidx]->layer_flags.irrelevant = FALSE;

              lyr_a[lidx]->extra_len = GUINT32_FROM_BE (lyr_a[lidx]->extra_len);
              block_rem = lyr_a[lidx]->extra_len;
              IFDBG(2) g_debug ("\n\tLayer mode sig: %.4s\n\tBlend mode: %.4s\n\t"
                                "Opacity: %d\n\tClipping: %d\n\tExtra data len: %d\n\t"
                                "Alpha lock: %d\n\tVisible: %d\n\tIrrelevant: %d",
                                    lyr_a[lidx]->mode_key,
                                    lyr_a[lidx]->blend_mode,
                                    lyr_a[lidx]->opacity,
                                    lyr_a[lidx]->clipping,
                                    lyr_a[lidx]->extra_len,
                                    lyr_a[lidx]->layer_flags.trans_prot,
                                    lyr_a[lidx]->layer_flags.visible,
                                    lyr_a[lidx]->layer_flags.irrelevant);
              IFDBG(3) g_debug ("Remaining length %d", block_rem);

              /* Layer mask data */
              if (fread (&block_len, 4, 1, f) < 1)
                {
                  psd_set_error (feof (f), errno, error);
                  return NULL;
                }
              block_len = GUINT32_FROM_BE (block_len);
              block_rem -= (block_len + 4);
              IFDBG(3) g_debug ("Remaining length %d", block_rem);

              lyr_a[lidx]->layer_mask_extra.top = 0;
              lyr_a[lidx]->layer_mask_extra.left = 0;
              lyr_a[lidx]->layer_mask_extra.bottom = 0;
              lyr_a[lidx]->layer_mask_extra.right = 0;
              lyr_a[lidx]->layer_mask.top = 0;
              lyr_a[lidx]->layer_mask.left = 0;
              lyr_a[lidx]->layer_mask.bottom = 0;
              lyr_a[lidx]->layer_mask.right = 0;
              lyr_a[lidx]->layer_mask.def_color = 0;
              lyr_a[lidx]->layer_mask.extra_def_color = 0;
              lyr_a[lidx]->layer_mask.mask_flags.relative_pos = FALSE;
              lyr_a[lidx]->layer_mask.mask_flags.disabled = FALSE;
              lyr_a[lidx]->layer_mask.mask_flags.invert = FALSE;

              switch (block_len)
                {
                  case 0:
                    break;

                  case 20:
                    if (fread (&lyr_a[lidx]->layer_mask.top, 4, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.left, 4, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.bottom, 4, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.right, 4, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.def_color, 1, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.flags, 1, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.extra_def_color, 1, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.extra_flags, 1, 1, f) < 1)
                      {
                        psd_set_error (feof (f), errno, error);
                        return NULL;
                      }
                    lyr_a[lidx]->layer_mask.top =
                      GUINT32_FROM_BE (lyr_a[lidx]->layer_mask.top);
                    lyr_a[lidx]->layer_mask.left =
                      GUINT32_FROM_BE (lyr_a[lidx]->layer_mask.left);
                    lyr_a[lidx]->layer_mask.bottom =
                      GUINT32_FROM_BE (lyr_a[lidx]->layer_mask.bottom);
                    lyr_a[lidx]->layer_mask.right =
                      GUINT32_FROM_BE (lyr_a[lidx]->layer_mask.right);
                    lyr_a[lidx]->layer_mask.mask_flags.relative_pos =
                      lyr_a[lidx]->layer_mask.flags & 1 ? TRUE : FALSE;
                    lyr_a[lidx]->layer_mask.mask_flags.disabled =
                      lyr_a[lidx]->layer_mask.flags & 2 ? TRUE : FALSE;
                    lyr_a[lidx]->layer_mask.mask_flags.invert =
                      lyr_a[lidx]->layer_mask.flags & 4 ? TRUE : FALSE;
                    break;
                  case 36: /* If we have a 36 byte mask record assume second data set is correct */
                    if (fread (&lyr_a[lidx]->layer_mask_extra.top, 4, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask_extra.left, 4, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask_extra.bottom, 4, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask_extra.right, 4, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.extra_def_color, 1, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.extra_flags, 1, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.def_color, 1, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.flags, 1, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.top, 4, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.left, 4, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.bottom, 4, 1, f) < 1
                        || fread (&lyr_a[lidx]->layer_mask.right, 4, 1, f) < 1)
                      {
                        psd_set_error (feof (f), errno, error);
                        return NULL;
                      }
                    lyr_a[lidx]->layer_mask_extra.top =
                      GUINT32_FROM_BE (lyr_a[lidx]->layer_mask_extra.top);
                    lyr_a[lidx]->layer_mask_extra.left =
                      GUINT32_FROM_BE (lyr_a[lidx]->layer_mask_extra.left);
                    lyr_a[lidx]->layer_mask_extra.bottom =
                      GUINT32_FROM_BE (lyr_a[lidx]->layer_mask_extra.bottom);
                    lyr_a[lidx]->layer_mask_extra.right =
                      GUINT32_FROM_BE (lyr_a[lidx]->layer_mask_extra.right);
                    lyr_a[lidx]->layer_mask.top =
                      GUINT32_FROM_BE (lyr_a[lidx]->layer_mask.top);
                    lyr_a[lidx]->layer_mask.left =
                      GUINT32_FROM_BE (lyr_a[lidx]->layer_mask.left);
                    lyr_a[lidx]->layer_mask.bottom =
                      GUINT32_FROM_BE (lyr_a[lidx]->layer_mask.bottom);
                    lyr_a[lidx]->layer_mask.right =
                      GUINT32_FROM_BE (lyr_a[lidx]->layer_mask.right);
                    lyr_a[lidx]->layer_mask.mask_flags.relative_pos =
                      lyr_a[lidx]->layer_mask.flags & 1 ? TRUE : FALSE;
                    lyr_a[lidx]->layer_mask.mask_flags.disabled =
                      lyr_a[lidx]->layer_mask.flags & 2 ? TRUE : FALSE;
                    lyr_a[lidx]->layer_mask.mask_flags.invert =
                      lyr_a[lidx]->layer_mask.flags & 4 ? TRUE : FALSE;
                    break;

                  default:
                    IFDBG(1) g_debug ("Unknown layer mask record size ... skipping");
                    if (fseek (f, block_len, SEEK_CUR) < 0)
                      {
                        psd_set_error (feof (f), errno, error);
                        return NULL;
                      }
                }

              IFDBG(2) g_debug ("Layer mask coords %d %d %d %d, Rel pos %d",
                                lyr_a[lidx]->layer_mask.left,
                                lyr_a[lidx]->layer_mask.top,
                                lyr_a[lidx]->layer_mask.right,
                                lyr_a[lidx]->layer_mask.bottom,
                                lyr_a[lidx]->layer_mask.mask_flags.relative_pos);

              IFDBG(3) g_debug ("Default mask color, %d, %d",
                                lyr_a[lidx]->layer_mask.def_color,
                                lyr_a[lidx]->layer_mask.extra_def_color);

              /* Layer blending ranges */           /* FIXME  */
              if (fread (&block_len, 4, 1, f) < 1)
                {
                  psd_set_error (feof (f), errno, error);
                  return NULL;
                }
              block_len = GUINT32_FROM_BE (block_len);
              block_rem -= (block_len + 4);
              IFDBG(3) g_debug ("Remaining length %d", block_rem);
              if (block_len > 0)
                {
                  if (fseek (f, block_len, SEEK_CUR) < 0)
                    {
                      psd_set_error (feof (f), errno, error);
                      return NULL;
                    }
                }

              lyr_a[lidx]->name = fread_pascal_string (&read_len, &write_len,
                                                       4, f, error);
              if (*error)
                return NULL;
              block_rem -= read_len;
              IFDBG(3) g_debug ("Remaining length %d", block_rem);

              /* Adjustment layer info */           /* FIXME */

              while (block_rem > 7)
                {
                  if (get_layer_resource_header (&res_a, f, error) < 0)
                    return NULL;
                  block_rem -= 12;

                  if (res_a.data_len > block_rem)
                    {
                      IFDBG(1) g_debug ("Unexpected end of layer resource data");
                      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                                  _("The file is corrupt!"));
                      return NULL;
                    }

                  if (load_layer_resource (&res_a, lyr_a[lidx], f, error) < 0)
                    return NULL;
                  block_rem -= res_a.data_len;
                }
              if (block_rem > 0)
                {
                  if (fseek (f, block_rem, SEEK_CUR) < 0)
                    {
                      psd_set_error (feof (f), errno, error);
                      return NULL;
                    }
                }
            }

          img_a->layer_data_start = ftell(f);
          if (fseek (f, img_a->layer_data_len, SEEK_CUR) < 0)
            {
              psd_set_error (feof (f), errno, error);
              return NULL;
            }

          IFDBG(1) g_debug ("Layer image data block size %d",
                             img_a->layer_data_len);
        }
      else
        lyr_a = NULL;

      /* Read global layer mask record */       /* FIXME */

      /* Skip to end of block */
      if (fseek (f, block_end, SEEK_SET) < 0)
        {
          psd_set_error (feof (f), errno, error);
          return NULL;
        }
    }

  return lyr_a;
}