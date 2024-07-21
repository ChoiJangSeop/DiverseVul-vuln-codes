xcf_load_channel_props (XcfInfo      *info,
                        GimpImage    *image,
                        GimpChannel **channel)
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

        case PROP_ACTIVE_CHANNEL:
          info->active_channel = *channel;
          break;

        case PROP_SELECTION:
          {
            GimpChannel *mask;

            mask =
              gimp_selection_new (image,
                                  gimp_item_get_width  (GIMP_ITEM (*channel)),
                                  gimp_item_get_height (GIMP_ITEM (*channel)));
            gimp_image_take_mask (image, mask);

            tile_manager_unref (GIMP_DRAWABLE (mask)->private->tiles);
            GIMP_DRAWABLE (mask)->private->tiles =
              GIMP_DRAWABLE (*channel)->private->tiles;
            GIMP_DRAWABLE (*channel)->private->tiles = NULL;
            g_object_unref (*channel);
            *channel = mask;
            (*channel)->boundary_known = FALSE;
            (*channel)->bounds_known   = FALSE;
          }
          break;

        case PROP_OPACITY:
          {
            guint32 opacity;

            info->cp += xcf_read_int32 (info->fp, &opacity, 1);
            gimp_channel_set_opacity (*channel, opacity / 255.0, FALSE);
          }
          break;

        case PROP_VISIBLE:
          {
            gboolean visible;

            info->cp += xcf_read_int32 (info->fp, (guint32 *) &visible, 1);
            gimp_item_set_visible (GIMP_ITEM (*channel),
                                   visible ? TRUE : FALSE, FALSE);
          }
          break;

        case PROP_LINKED:
          {
            gboolean linked;

            info->cp += xcf_read_int32 (info->fp, (guint32 *) &linked, 1);
            gimp_item_set_linked (GIMP_ITEM (*channel),
                                  linked ? TRUE : FALSE, FALSE);
          }
          break;

        case PROP_LOCK_CONTENT:
          {
            gboolean lock_content;

            info->cp += xcf_read_int32 (info->fp, (guint32 *) &lock_content, 1);
            gimp_item_set_lock_content (GIMP_ITEM (*channel),
                                        lock_content ? TRUE : FALSE, FALSE);
          }
          break;

        case PROP_SHOW_MASKED:
          {
            gboolean show_masked;

            info->cp += xcf_read_int32 (info->fp, (guint32 *) &show_masked, 1);
            gimp_channel_set_show_masked (*channel, show_masked);
          }
          break;

        case PROP_COLOR:
          {
            guchar col[3];

            info->cp += xcf_read_int8 (info->fp, (guint8 *) col, 3);
            gimp_rgb_set_uchar (&(*channel)->color, col[0], col[1], col[2]);
          }
          break;

        case PROP_TATTOO:
          {
            GimpTattoo tattoo;

            info->cp += xcf_read_int32 (info->fp, (guint32 *) &tattoo, 1);
            gimp_item_set_tattoo (GIMP_ITEM (*channel), tattoo);
          }
          break;

        case PROP_PARASITES:
          {
            glong base = info->cp;

            while ((info->cp - base) < prop_size)
              {
                GimpParasite *p = xcf_load_parasite (info);

                if (! p)
                  return FALSE;

                gimp_item_parasite_attach (GIMP_ITEM (*channel), p, FALSE);
                gimp_parasite_free (p);
              }

            if (info->cp - base != prop_size)
              gimp_message_literal (info->gimp, G_OBJECT (info->progress),
				    GIMP_MESSAGE_WARNING,
				    "Error while loading a channel's parasites");
          }
          break;

        default:
#ifdef GIMP_UNSTABLE
          g_printerr ("unexpected/unknown channel property: %d (skipping)\n",
                      prop_type);
#endif
          if (! xcf_skip_unknown_prop (info, prop_size))
            return FALSE;
          break;
        }
    }

  return FALSE;
}