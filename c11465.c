xcf_load_image_props (XcfInfo   *info,
                      GimpImage *image)
{
  PropType prop_type;
  guint32  prop_size;

  while (TRUE)
    {
      if (! xcf_load_prop (info, &prop_type, &prop_size))
        return FALSE;

      switch (prop_type)
        {
        case PROP_END:
          return TRUE;

        case PROP_COLORMAP:
          {
            guint32 n_colors;
            guchar  cmap[GIMP_IMAGE_COLORMAP_SIZE];

            xcf_read_int32 (info, &n_colors, 1);

            if (n_colors > (GIMP_IMAGE_COLORMAP_SIZE / 3))
              {
                gimp_message (info->gimp, G_OBJECT (info->progress),
                              GIMP_MESSAGE_ERROR,
                              "Maximum colormap size (%d) exceeded",
                              GIMP_IMAGE_COLORMAP_SIZE);
                return FALSE;
              }

            if (info->file_version == 0)
              {
                gint i;

                gimp_message_literal (info->gimp, G_OBJECT (info->progress),
                                      GIMP_MESSAGE_WARNING,
                                      _("XCF warning: version 0 of XCF file format\n"
                                        "did not save indexed colormaps correctly.\n"
                                        "Substituting grayscale map."));

                if (! xcf_seek_pos (info, info->cp + n_colors, NULL))
                  return FALSE;

                for (i = 0; i < n_colors; i++)
                  {
                    cmap[i * 3 + 0] = i;
                    cmap[i * 3 + 1] = i;
                    cmap[i * 3 + 2] = i;
                  }
              }
            else
              {
                xcf_read_int8 (info, cmap, n_colors * 3);
              }

            /* only set color map if image is indexed, this is just
             * sanity checking to make sure gimp doesn't end up with
             * an image state that is impossible.
             */
            if (gimp_image_get_base_type (image) == GIMP_INDEXED)
              gimp_image_set_colormap (image, cmap, n_colors, FALSE);

            GIMP_LOG (XCF, "prop colormap n_colors=%d", n_colors);
          }
          break;

        case PROP_COMPRESSION:
          {
            guint8 compression;

            xcf_read_int8 (info, (guint8 *) &compression, 1);

            if ((compression != COMPRESS_NONE) &&
                (compression != COMPRESS_RLE) &&
                (compression != COMPRESS_ZLIB) &&
                (compression != COMPRESS_FRACTAL))
              {
                gimp_message (info->gimp, G_OBJECT (info->progress),
                              GIMP_MESSAGE_ERROR,
                              "Unknown compression type: %d",
                              (gint) compression);
                return FALSE;
              }

            info->compression = compression;

            gimp_image_set_xcf_compression (image,
                                            compression >= COMPRESS_ZLIB);

            GIMP_LOG (XCF, "prop compression=%d", compression);
          }
          break;

        case PROP_GUIDES:
          {
            GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);
            gint32            position;
            gint8             orientation;
            gint              i, nguides;

            nguides = prop_size / (4 + 1);
            for (i = 0; i < nguides; i++)
              {
                xcf_read_int32 (info, (guint32 *) &position,    1);
                xcf_read_int8  (info, (guint8 *)  &orientation, 1);

                /* Some very old XCF had -1 guides which have been
                 * skipped since 2003 (commit 909a28ced2).
                 * Then XCF up to version 14 only had positive guide
                 * positions.
                 * Since XCF 15 (GIMP 3.0), off-canvas guides became a
                 * thing.
                 */
                if (info->file_version < 15 && position < 0)
                  continue;

                GIMP_LOG (XCF, "prop guide orientation=%d position=%d",
                          orientation, position);

                switch (orientation)
                  {
                  case XCF_ORIENTATION_HORIZONTAL:
                    if (info->file_version < 15 && position > gimp_image_get_height (image))
                      gimp_message (info->gimp, G_OBJECT (info->progress),
                                    GIMP_MESSAGE_WARNING,
                                    "Ignoring off-canvas horizontal guide (position %d) in XCF %d file",
                                    position, info->file_version);
                    else
                      gimp_image_add_hguide (image, position, FALSE);
                    break;

                  case XCF_ORIENTATION_VERTICAL:
                    if (info->file_version < 15 && position > gimp_image_get_width (image))
                      gimp_message (info->gimp, G_OBJECT (info->progress),
                                    GIMP_MESSAGE_WARNING,
                                    "Ignoring off-canvas vertical guide (position %d) in XCF %d file",
                                    position, info->file_version);
                    else
                      gimp_image_add_vguide (image, position, FALSE);
                    break;

                  default:
                    gimp_message_literal (info->gimp, G_OBJECT (info->progress),
                                          GIMP_MESSAGE_WARNING,
                                          "Guide orientation out of range in XCF file");
                    continue;
                  }
              }

            /*  this is silly as the order of guides doesn't really matter,
             *  but it restores the list to its original order, which
             *  cannot be wrong  --Mitch
             */
            private->guides = g_list_reverse (private->guides);
          }
          break;

        case PROP_SAMPLE_POINTS:
          {
            gint n_sample_points, i;

            n_sample_points = prop_size / (5 * 4);
            for (i = 0; i < n_sample_points; i++)
              {
                GimpSamplePoint   *sample_point;
                gint32             x, y;
                GimpColorPickMode  pick_mode;
                guint32            padding[2] = { 0, };

                xcf_read_int32 (info, (guint32 *) &x,         1);
                xcf_read_int32 (info, (guint32 *) &y,         1);
                xcf_read_int32 (info, (guint32 *) &pick_mode, 1);
                xcf_read_int32 (info, (guint32 *) padding,    2);

                GIMP_LOG (XCF, "prop sample point x=%d y=%d mode=%d",
                          x, y, pick_mode);

                if (pick_mode > GIMP_COLOR_PICK_MODE_LAST)
                  pick_mode = GIMP_COLOR_PICK_MODE_PIXEL;

                sample_point = gimp_image_add_sample_point_at_pos (image,
                                                                   x, y, FALSE);
                gimp_image_set_sample_point_pick_mode (image, sample_point,
                                                       pick_mode, FALSE);
              }
          }
          break;

        case PROP_OLD_SAMPLE_POINTS:
          {
            gint32 x, y;
            gint   i, n_sample_points;

            /* if there are already sample points, we loaded the new
             * prop before
             */
            if (gimp_image_get_sample_points (image))
              {
                if (! xcf_skip_unknown_prop (info, prop_size))
                  return FALSE;

                break;
              }

            n_sample_points = prop_size / (4 + 4);
            for (i = 0; i < n_sample_points; i++)
              {
                xcf_read_int32 (info, (guint32 *) &x, 1);
                xcf_read_int32 (info, (guint32 *) &y, 1);

                GIMP_LOG (XCF, "prop old sample point x=%d y=%d", x, y);

                gimp_image_add_sample_point_at_pos (image, x, y, FALSE);
              }
          }
          break;

        case PROP_RESOLUTION:
          {
            gfloat xres, yres;

            xcf_read_float (info, &xres, 1);
            xcf_read_float (info, &yres, 1);

            GIMP_LOG (XCF, "prop resolution x=%f y=%f", xres, yres);

            if (xres < GIMP_MIN_RESOLUTION || xres > GIMP_MAX_RESOLUTION ||
                yres < GIMP_MIN_RESOLUTION || yres > GIMP_MAX_RESOLUTION)
              {
                GimpTemplate *template = image->gimp->config->default_image;

                gimp_message_literal (info->gimp, G_OBJECT (info->progress),
                                      GIMP_MESSAGE_WARNING,
                                      "Warning, resolution out of range in XCF file");
                xres = gimp_template_get_resolution_x (template);
                yres = gimp_template_get_resolution_y (template);
              }

            gimp_image_set_resolution (image, xres, yres);
          }
          break;

        case PROP_TATTOO:
          {
            xcf_read_int32 (info, &info->tattoo_state, 1);

            GIMP_LOG (XCF, "prop tattoo state=%d", info->tattoo_state);
          }
          break;

        case PROP_PARASITES:
          {
            goffset base = info->cp;

            while (info->cp - base < prop_size)
              {
                GimpParasite *p     = xcf_load_parasite (info);
                GError       *error = NULL;

                if (! p)
                  {
                    gimp_message (info->gimp, G_OBJECT (info->progress),
                                  GIMP_MESSAGE_WARNING,
                                  "Invalid image parasite found. "
                                  "Possibly corrupt XCF file.");

                    xcf_seek_pos (info, base + prop_size, NULL);
                    continue;
                  }

                if (! gimp_image_parasite_validate (image, p, &error))
                  {
                    gimp_message (info->gimp, G_OBJECT (info->progress),
                                  GIMP_MESSAGE_WARNING,
                                  "Warning, invalid image parasite in XCF file: %s",
                                  error->message);
                    g_clear_error (&error);
                  }
                else
                  {
                    gimp_image_parasite_attach (image, p, FALSE);
                  }

                gimp_parasite_free (p);
              }

            if (info->cp - base != prop_size)
              gimp_message_literal (info->gimp, G_OBJECT (info->progress),
                                    GIMP_MESSAGE_WARNING,
                                    "Error while loading an image's parasites");
          }
          break;

        case PROP_UNIT:
          {
            guint32 unit;

            xcf_read_int32 (info, &unit, 1);

            GIMP_LOG (XCF, "prop unit=%d", unit);

            if ((unit <= GIMP_UNIT_PIXEL) ||
                (unit >= gimp_unit_get_number_of_built_in_units ()))
              {
                gimp_message_literal (info->gimp, G_OBJECT (info->progress),
                                      GIMP_MESSAGE_WARNING,
                                      "Warning, unit out of range in XCF file, "
                                      "falling back to inches");
                unit = GIMP_UNIT_INCH;
              }

            gimp_image_set_unit (image, unit);
          }
          break;

        case PROP_PATHS:
          xcf_load_old_paths (info, image);
          break;

        case PROP_USER_UNIT:
          {
            gchar    *unit_strings[5];
            float     factor;
            guint32   digits;
            GimpUnit  unit;
            gint      num_units;
            gint      i;

            xcf_read_float  (info, &factor,      1);
            xcf_read_int32  (info, &digits,      1);
            xcf_read_string (info, unit_strings, 5);

            for (i = 0; i < 5; i++)
              if (unit_strings[i] == NULL)
                unit_strings[i] = g_strdup ("");

            num_units = gimp_unit_get_number_of_units ();

            for (unit = gimp_unit_get_number_of_built_in_units ();
                 unit < num_units; unit++)
              {
                /* if the factor and the identifier match some unit
                 * in unitrc, use the unitrc unit
                 */
                if ((ABS (gimp_unit_get_factor (unit) - factor) < 1e-5) &&
                    (strcmp (unit_strings[0],
                             gimp_unit_get_identifier (unit)) == 0))
                  {
                    break;
                  }
              }

            /* no match */
            if (unit == num_units)
              unit = gimp_unit_new (unit_strings[0],
                                    factor,
                                    digits,
                                    unit_strings[1],
                                    unit_strings[2],
                                    unit_strings[3],
                                    unit_strings[4]);

            gimp_image_set_unit (image, unit);

            for (i = 0; i < 5; i++)
              g_free (unit_strings[i]);
          }
         break;

        case PROP_VECTORS:
          {
            goffset base = info->cp;

            if (xcf_load_vectors (info, image))
              {
                if (base + prop_size != info->cp)
                  {
                    g_printerr ("Mismatch in PROP_VECTORS size: "
                                "skipping %" G_GOFFSET_FORMAT " bytes.\n",
                                base + prop_size - info->cp);
                    xcf_seek_pos (info, base + prop_size, NULL);
                  }
              }
            else
              {
                /* skip silently since we don't understand the format and
                 * xcf_load_vectors already explained what was wrong
                 */
                xcf_seek_pos (info, base + prop_size, NULL);
              }
          }
          break;

        case PROP_ITEM_SET:
          {
            GimpItemList *set       = NULL;
            gchar        *label;
            GType         item_type = 0;
            guint32       itype;
            guint32       method;

            xcf_read_int32  (info, &itype, 1);
            xcf_read_int32  (info, &method, 1);
            xcf_read_string (info, &label, 1);

            if (itype == 0)
              item_type = GIMP_TYPE_LAYER;
            else
              item_type = GIMP_TYPE_CHANNEL;

            if (itype > 1)
              {
                g_printerr ("xcf: unsupported item set '%s' type: %d (skipping)\n",
                            label ? label : "unnamed", itype);
                /* Only case where we break because we wouldn't even
                 * know where to categorize the item set anyway. */
                break;
              }
            else if (label == NULL)
              {
                g_printerr ("xcf: item set without a name or pattern (skipping)\n");
              }
            else if (method != G_MAXUINT32 && method > GIMP_SELECT_GLOB_PATTERN)
              {
                g_printerr ("xcf: unsupported item set '%s' selection method attribute: 0x%x (skipping)\n",
                            label, method);
              }
            else
              {
                if (method == G_MAXUINT32)
                  {
                    /* Don't use gimp_item_list_named_new() because it
                     * doesn't allow NULL items (it would try to get the
                     * selected items instead).
                     */
                    set = g_object_new (GIMP_TYPE_ITEM_LIST,
                                        "image",      image,
                                        "name",       label,
                                        "is-pattern", FALSE,
                                        "item-type",  item_type,
                                        "items",      NULL,
                                        NULL);
                  }
                else
                  {
                    set = gimp_item_list_pattern_new (image, item_type,
                                                      method, label);
                  }
              }

            /* Note: we are still adding invalid item sets as NULL on
             * purpose, in order not to break order-base association
             * between PROP_ITEM_SET and PROP_ITEM_SET_ITEM.
             */
            if (item_type == GIMP_TYPE_LAYER)
              info->layer_sets = g_list_prepend (info->layer_sets, set);
            else
              info->channel_sets = g_list_prepend (info->channel_sets, set);
          }
          break;

        default:
#ifdef GIMP_UNSTABLE
          g_printerr ("unexpected/unknown image property: %d (skipping)\n",
                      prop_type);
#endif
          if (! xcf_skip_unknown_prop (info, prop_size))
            return FALSE;
          break;
        }
    }

  return FALSE;
}