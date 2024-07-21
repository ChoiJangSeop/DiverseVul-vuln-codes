add_layers (const gint32  image_id,
            PSDimage     *img_a,
            PSDlayer    **lyr_a,
            FILE         *f,
            GError      **error)
{
  PSDchannel          **lyr_chn;
  guchar               *pixels;
  guint16               alpha_chn;
  guint16               user_mask_chn;
  guint16               layer_channels;
  guint16               channel_idx[MAX_CHANNELS];
  guint16              *rle_pack_len;
  gint32                l_x;                   /* Layer x */
  gint32                l_y;                   /* Layer y */
  gint32                l_w;                   /* Layer width */
  gint32                l_h;                   /* Layer height */
  gint32                lm_x;                  /* Layer mask x */
  gint32                lm_y;                  /* Layer mask y */
  gint32                lm_w;                  /* Layer mask width */
  gint32                lm_h;                  /* Layer mask height */
  gint32                layer_size;
  gint32                layer_id = -1;
  gint32                mask_id = -1;
  gint                  lidx;                  /* Layer index */
  gint                  cidx;                  /* Channel index */
  gint                  rowi;                  /* Row index */
  gint                  coli;                  /* Column index */
  gint                  i;
  gboolean              alpha;
  gboolean              user_mask;
  gboolean              empty;
  gboolean              empty_mask;
  GimpDrawable         *drawable;
  GimpPixelRgn          pixel_rgn;
  GimpImageType         image_type;
  GimpLayerModeEffects  layer_mode;


  IFDBG(2) g_debug ("Number of layers: %d", img_a->num_layers);

  if (img_a->num_layers == 0)
    {
      IFDBG(2) g_debug ("No layers to process");
      return 0;
    }

  /* Layered image - Photoshop 3 style */
  if (fseek (f, img_a->layer_data_start, SEEK_SET) < 0)
    {
      psd_set_error (feof (f), errno, error);
      return -1;
    }

  for (lidx = 0; lidx < img_a->num_layers; ++lidx)
    {
      IFDBG(2) g_debug ("Process Layer No %d.", lidx);

      if (lyr_a[lidx]->drop)
        {
          IFDBG(2) g_debug ("Drop layer %d", lidx);

          /* Step past layer data */
          for (cidx = 0; cidx < lyr_a[lidx]->num_channels; ++cidx)
            {
              if (fseek (f, lyr_a[lidx]->chn_info[cidx].data_len, SEEK_CUR) < 0)
                {
                  psd_set_error (feof (f), errno, error);
                  return -1;
                }
            }
          g_free (lyr_a[lidx]->chn_info);
          g_free (lyr_a[lidx]->name);
        }

      else
        {
          /* Empty layer */
          if (lyr_a[lidx]->bottom - lyr_a[lidx]->top == 0
              || lyr_a[lidx]->right - lyr_a[lidx]->left == 0)
              empty = TRUE;
          else
              empty = FALSE;

          /* Empty mask */
          if (lyr_a[lidx]->layer_mask.bottom - lyr_a[lidx]->layer_mask.top == 0
              || lyr_a[lidx]->layer_mask.right - lyr_a[lidx]->layer_mask.left == 0)
              empty_mask = TRUE;
          else
              empty_mask = FALSE;

          IFDBG(3) g_debug ("Empty mask %d, size %d %d", empty_mask,
                            lyr_a[lidx]->layer_mask.bottom - lyr_a[lidx]->layer_mask.top,
                            lyr_a[lidx]->layer_mask.right - lyr_a[lidx]->layer_mask.left);

          /* Load layer channel data */
          IFDBG(2) g_debug ("Number of channels: %d", lyr_a[lidx]->num_channels);
          /* Create pointer array for the channel records */
          lyr_chn = g_new (PSDchannel *, lyr_a[lidx]->num_channels);
          for (cidx = 0; cidx < lyr_a[lidx]->num_channels; ++cidx)
            {
              guint16 comp_mode = PSD_COMP_RAW;

              /* Allocate channel record */
              lyr_chn[cidx] = g_malloc (sizeof (PSDchannel) );

              lyr_chn[cidx]->id = lyr_a[lidx]->chn_info[cidx].channel_id;
              lyr_chn[cidx]->rows = lyr_a[lidx]->bottom - lyr_a[lidx]->top;
              lyr_chn[cidx]->columns = lyr_a[lidx]->right - lyr_a[lidx]->left;

              if (lyr_chn[cidx]->id == PSD_CHANNEL_MASK)
                {
                  /* Works around a bug in panotools psd files where the layer mask
                     size is given as 0 but data exists. Set mask size to layer size.
                  */
                  if (empty_mask && lyr_a[lidx]->chn_info[cidx].data_len - 2 > 0)
                    {
                      empty_mask = FALSE;
                      if (lyr_a[lidx]->layer_mask.top == lyr_a[lidx]->layer_mask.bottom)
                        {
                          lyr_a[lidx]->layer_mask.top = lyr_a[lidx]->top;
                          lyr_a[lidx]->layer_mask.bottom = lyr_a[lidx]->bottom;
                        }
                      if (lyr_a[lidx]->layer_mask.right == lyr_a[lidx]->layer_mask.left)
                        {
                          lyr_a[lidx]->layer_mask.right = lyr_a[lidx]->right;
                          lyr_a[lidx]->layer_mask.left = lyr_a[lidx]->left;
                        }
                    }
                  lyr_chn[cidx]->rows = (lyr_a[lidx]->layer_mask.bottom -
                                        lyr_a[lidx]->layer_mask.top);
                  lyr_chn[cidx]->columns = (lyr_a[lidx]->layer_mask.right -
                                           lyr_a[lidx]->layer_mask.left);
                }

              IFDBG(3) g_debug ("Channel id %d, %dx%d",
                                lyr_chn[cidx]->id,
                                lyr_chn[cidx]->columns,
                                lyr_chn[cidx]->rows);

              /* Only read channel data if there is any channel
               * data. Note that the channel data can contain a
               * compression method but no actual data.
               */
              if (lyr_a[lidx]->chn_info[cidx].data_len >= COMP_MODE_SIZE)
                {
                  if (fread (&comp_mode, COMP_MODE_SIZE, 1, f) < 1)
                    {
                      psd_set_error (feof (f), errno, error);
                      return -1;
                    }
                  comp_mode = GUINT16_FROM_BE (comp_mode);
                  IFDBG(3) g_debug ("Compression mode: %d", comp_mode);
                }
              if (lyr_a[lidx]->chn_info[cidx].data_len > COMP_MODE_SIZE)
                {
                  switch (comp_mode)
                    {
                      case PSD_COMP_RAW:        /* Planar raw data */
                        IFDBG(3) g_debug ("Raw data length: %d",
                                          lyr_a[lidx]->chn_info[cidx].data_len - 2);
                        if (read_channel_data (lyr_chn[cidx], img_a->bps,
                            PSD_COMP_RAW, NULL, f, error) < 1)
                          return -1;
                        break;

                      case PSD_COMP_RLE:        /* Packbits */
                        IFDBG(3) g_debug ("RLE channel length %d, RLE length data: %d, "
                                          "RLE data block: %d",
                                          lyr_a[lidx]->chn_info[cidx].data_len - 2,
                                          lyr_chn[cidx]->rows * 2,
                                          (lyr_a[lidx]->chn_info[cidx].data_len - 2 -
                                           lyr_chn[cidx]->rows * 2));
                        rle_pack_len = g_malloc (lyr_chn[cidx]->rows * 2);
                        for (rowi = 0; rowi < lyr_chn[cidx]->rows; ++rowi)
                          {
                            if (fread (&rle_pack_len[rowi], 2, 1, f) < 1)
                              {
                                psd_set_error (feof (f), errno, error);
                                return -1;
                              }
                                rle_pack_len[rowi] = GUINT16_FROM_BE (rle_pack_len[rowi]);
                          }

                        IFDBG(3) g_debug ("RLE decode - data");
                        if (read_channel_data (lyr_chn[cidx], img_a->bps,
                            PSD_COMP_RLE, rle_pack_len, f, error) < 1)
                          return -1;

                        g_free (rle_pack_len);
                        break;

                      case PSD_COMP_ZIP:                 /* ? */
                      case PSD_COMP_ZIP_PRED:
                      default:
                        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                                    _("Unsupported compression mode: %d"), comp_mode);
                        return -1;
                        break;
                    }
                }
            }
          g_free (lyr_a[lidx]->chn_info);

          /* Draw layer */

          alpha = FALSE;
          alpha_chn = -1;
          user_mask = FALSE;
          user_mask_chn = -1;
          layer_channels = 0;
          l_x = 0;
          l_y = 0;
          l_w = img_a->columns;
          l_h = img_a->rows;

          IFDBG(3) g_debug ("Re-hash channel indices");
          for (cidx = 0; cidx < lyr_a[lidx]->num_channels; ++cidx)
            {
              if (lyr_chn[cidx]->id == PSD_CHANNEL_MASK)
                {
                  user_mask = TRUE;
                  user_mask_chn = cidx;
                }
              else if (lyr_chn[cidx]->id == PSD_CHANNEL_ALPHA)
                {
                  alpha = TRUE;
                  alpha_chn = cidx;
                }
              else
                {
                  channel_idx[layer_channels] = cidx;   /* Assumes in sane order */
                  layer_channels++;                     /* RGB, Lab, CMYK etc.   */
                }
            }
          if (alpha)
            {
              channel_idx[layer_channels] = alpha_chn;
              layer_channels++;
            }

          if (empty)
            {
              IFDBG(2) g_debug ("Create blank layer");
              image_type = get_gimp_image_type (img_a->base_type, TRUE);
              layer_id = gimp_layer_new (image_id, lyr_a[lidx]->name,
                                         img_a->columns, img_a->rows,
                                         image_type, 0, GIMP_NORMAL_MODE);
              g_free (lyr_a[lidx]->name);
              gimp_image_add_layer (image_id, layer_id, -1);
              drawable = gimp_drawable_get (layer_id);
              gimp_drawable_fill (drawable->drawable_id, GIMP_TRANSPARENT_FILL);
              gimp_drawable_set_visible (drawable->drawable_id, lyr_a[lidx]->layer_flags.visible);
              if (lyr_a[lidx]->id)
                gimp_drawable_set_tattoo (drawable->drawable_id, lyr_a[lidx]->id);
              if (lyr_a[lidx]->layer_flags.irrelevant)
                gimp_drawable_set_visible (drawable->drawable_id, FALSE);
              gimp_drawable_flush (drawable);
              gimp_drawable_detach (drawable);
            }
          else
            {
              l_x = lyr_a[lidx]->left;
              l_y = lyr_a[lidx]->top;
              l_w = lyr_a[lidx]->right - lyr_a[lidx]->left;
              l_h = lyr_a[lidx]->bottom - lyr_a[lidx]->top;

              IFDBG(3) g_debug ("Draw layer");
              image_type = get_gimp_image_type (img_a->base_type, alpha);
              IFDBG(3) g_debug ("Layer type %d", image_type);
              layer_size = l_w * l_h;
              pixels = g_malloc (layer_size * layer_channels);
              for (cidx = 0; cidx < layer_channels; ++cidx)
                {
                  IFDBG(3) g_debug ("Start channel %d", channel_idx[cidx]);
                  for (i = 0; i < layer_size; ++i)
                    pixels[(i * layer_channels) + cidx] = lyr_chn[channel_idx[cidx]]->data[i];
                  g_free (lyr_chn[channel_idx[cidx]]->data);
                }

              layer_mode = psd_to_gimp_blend_mode (lyr_a[lidx]->blend_mode);
              layer_id = gimp_layer_new (image_id, lyr_a[lidx]->name, l_w, l_h,
                                         image_type, lyr_a[lidx]->opacity * 100 / 255,
                                         layer_mode);
              IFDBG(3) g_debug ("Layer tattoo: %d", layer_id);
              g_free (lyr_a[lidx]->name);
              gimp_image_add_layer (image_id, layer_id, -1);
              gimp_layer_set_offsets (layer_id, l_x, l_y);
              gimp_layer_set_lock_alpha  (layer_id, lyr_a[lidx]->layer_flags.trans_prot);
              drawable = gimp_drawable_get (layer_id);
              gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
                                   drawable->width, drawable->height, TRUE, FALSE);
              gimp_pixel_rgn_set_rect (&pixel_rgn, pixels,
                                       0, 0, drawable->width, drawable->height);
              gimp_drawable_set_visible (drawable->drawable_id, lyr_a[lidx]->layer_flags.visible);
              if (lyr_a[lidx]->id)
                gimp_drawable_set_tattoo (drawable->drawable_id, lyr_a[lidx]->id);
              gimp_drawable_flush (drawable);
              gimp_drawable_detach (drawable);
              g_free (pixels);
            }

          /* Layer mask */
          if (user_mask)
            {
              if (empty_mask)
                {
                  IFDBG(3) g_debug ("Create empty mask");
                  if (lyr_a[lidx]->layer_mask.def_color == 255)
                    mask_id = gimp_layer_create_mask (layer_id, GIMP_ADD_WHITE_MASK);
                  else
                    mask_id = gimp_layer_create_mask (layer_id, GIMP_ADD_BLACK_MASK);
                  gimp_layer_add_mask (layer_id, mask_id);
                  gimp_layer_set_apply_mask (layer_id,
                    ! lyr_a[lidx]->layer_mask.mask_flags.disabled);
                }
              else
                {
                  /* Load layer mask data */
                  if (lyr_a[lidx]->layer_mask.mask_flags.relative_pos)
                    {
                      lm_x = lyr_a[lidx]->layer_mask.left;
                      lm_y = lyr_a[lidx]->layer_mask.top;
                      lm_w = lyr_a[lidx]->layer_mask.right - lyr_a[lidx]->layer_mask.left;
                      lm_h = lyr_a[lidx]->layer_mask.bottom - lyr_a[lidx]->layer_mask.top;
                    }
                  else
                    {
                      lm_x = lyr_a[lidx]->layer_mask.left - l_x;
                      lm_y = lyr_a[lidx]->layer_mask.top - l_y;
                      lm_w = lyr_a[lidx]->layer_mask.right - lyr_a[lidx]->layer_mask.left;
                      lm_h = lyr_a[lidx]->layer_mask.bottom - lyr_a[lidx]->layer_mask.top;
                    }
                  IFDBG(3) g_debug ("Mask channel index %d", user_mask_chn);
                  IFDBG(3) g_debug ("Relative pos %d",
                                    lyr_a[lidx]->layer_mask.mask_flags.relative_pos);
                  layer_size = lm_w * lm_h;
                  pixels = g_malloc (layer_size);
                  IFDBG(3) g_debug ("Allocate Pixels %d", layer_size);
                  /* Crop mask at layer boundry */
                  IFDBG(3) g_debug ("Original Mask %d %d %d %d", lm_x, lm_y, lm_w, lm_h);
                  if (lm_x < 0
                      || lm_y < 0
                      || lm_w + lm_x > l_w
                      || lm_h + lm_y > l_h)
                    {
                      if (CONVERSION_WARNINGS)
                        g_message ("Warning\n"
                                   "The layer mask is partly outside the "
                                   "layer boundary. The mask will be "
                                   "cropped which may result in data loss.");
                      i = 0;
                      for (rowi = 0; rowi < lm_h; ++rowi)
                        {
                          if (rowi + lm_y >= 0 && rowi + lm_y < l_h)
                            {
                              for (coli = 0; coli < lm_w; ++coli)
                                {
                                  if (coli + lm_x >= 0 && coli + lm_x < l_w)
                                    {
                                      pixels[i] =
                                        lyr_chn[user_mask_chn]->data[(rowi * lm_w) + coli];
                                      i++;
                                    }
                                }
                            }
                        }
                      if (lm_x < 0)
                        {
                          lm_w += lm_x;
                          lm_x = 0;
                        }
                      if (lm_y < 0)
                        {
                          lm_h += lm_y;
                          lm_y = 0;
                        }
                      if (lm_w + lm_x > l_w)
                        lm_w = l_w - lm_x;
                      if (lm_h + lm_y > l_h)
                        lm_h = l_h - lm_y;
                    }
                  else
                    memcpy (pixels, lyr_chn[user_mask_chn]->data, layer_size);
                  g_free (lyr_chn[user_mask_chn]->data);
                  /* Draw layer mask data */
                  IFDBG(3) g_debug ("Layer %d %d %d %d", l_x, l_y, l_w, l_h);
                  IFDBG(3) g_debug ("Mask %d %d %d %d", lm_x, lm_y, lm_w, lm_h);

                  if (lyr_a[lidx]->layer_mask.def_color == 255)
                    mask_id = gimp_layer_create_mask (layer_id, GIMP_ADD_WHITE_MASK);
                  else
                    mask_id = gimp_layer_create_mask (layer_id, GIMP_ADD_BLACK_MASK);

                  IFDBG(3) g_debug ("New layer mask %d", mask_id);
                  gimp_layer_add_mask (layer_id, mask_id);
                  drawable = gimp_drawable_get (mask_id);
                  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0 , 0,
                                       drawable->width, drawable->height, TRUE, FALSE);
                  gimp_pixel_rgn_set_rect (&pixel_rgn, pixels, lm_x, lm_y, lm_w, lm_h);
                  gimp_drawable_flush (drawable);
                  gimp_drawable_detach (drawable);
                  gimp_layer_set_apply_mask (layer_id,
                    ! lyr_a[lidx]->layer_mask.mask_flags.disabled);
                  g_free (pixels);
                }
            }
          for (cidx = 0; cidx < lyr_a[lidx]->num_channels; ++cidx)
            if (lyr_chn[cidx])
              g_free (lyr_chn[cidx]);
          g_free (lyr_chn);
        }
      g_free (lyr_a[lidx]);
    }
  g_free (lyr_a);

  return 0;
}