xcf_load_layer_props (XcfInfo    *info,
                      GimpImage  *image,
                      GimpLayer **layer,
                      GList     **item_path,
                      gboolean   *apply_mask,
                      gboolean   *edit_mask,
                      gboolean   *show_mask,
                      guint32    *text_layer_flags,
                      guint32    *group_layer_flags)
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

        case PROP_ACTIVE_LAYER:
          info->active_layer = *layer;
          break;

        case PROP_FLOATING_SELECTION:
          info->floating_sel = *layer;
          info->cp +=
            xcf_read_int32 (info->fp,
                            (guint32 *) &info->floating_sel_offset, 1);
          break;

        case PROP_OPACITY:
          {
            guint32 opacity;

            info->cp += xcf_read_int32 (info->fp, &opacity, 1);
            gimp_layer_set_opacity (*layer, (gdouble) opacity / 255.0, FALSE);
          }
          break;

        case PROP_VISIBLE:
          {
            gboolean visible;

            info->cp += xcf_read_int32 (info->fp, (guint32 *) &visible, 1);
            gimp_item_set_visible (GIMP_ITEM (*layer), visible, FALSE);
          }
          break;

        case PROP_LINKED:
          {
            gboolean linked;

            info->cp += xcf_read_int32 (info->fp, (guint32 *) &linked, 1);
            gimp_item_set_linked (GIMP_ITEM (*layer), linked, FALSE);
          }
          break;

        case PROP_LOCK_CONTENT:
          {
            gboolean lock_content;

            info->cp += xcf_read_int32 (info->fp, (guint32 *) &lock_content, 1);

            if (gimp_item_can_lock_content (GIMP_ITEM (*layer)))
              gimp_item_set_lock_content (GIMP_ITEM (*layer),
                                          lock_content, FALSE);
          }
          break;

        case PROP_LOCK_ALPHA:
          {
            gboolean lock_alpha;

            info->cp += xcf_read_int32 (info->fp, (guint32 *) &lock_alpha, 1);

            if (gimp_layer_can_lock_alpha (*layer))
              gimp_layer_set_lock_alpha (*layer, lock_alpha, FALSE);
          }
          break;

        case PROP_APPLY_MASK:
          info->cp += xcf_read_int32 (info->fp, (guint32 *) apply_mask, 1);
          break;

        case PROP_EDIT_MASK:
          info->cp += xcf_read_int32 (info->fp, (guint32 *) edit_mask, 1);
          break;

        case PROP_SHOW_MASK:
          info->cp += xcf_read_int32 (info->fp, (guint32 *) show_mask, 1);
          break;

        case PROP_OFFSETS:
          {
            guint32 offset_x;
            guint32 offset_y;

            info->cp += xcf_read_int32 (info->fp, &offset_x, 1);
            info->cp += xcf_read_int32 (info->fp, &offset_y, 1);

            gimp_item_set_offset (GIMP_ITEM (*layer), offset_x, offset_y);
          }
          break;

        case PROP_MODE:
          {
            guint32 mode;

            info->cp += xcf_read_int32 (info->fp, &mode, 1);
            gimp_layer_set_mode (*layer, (GimpLayerModeEffects) mode, FALSE);
          }
          break;

        case PROP_TATTOO:
          {
            GimpTattoo tattoo;

            info->cp += xcf_read_int32 (info->fp, (guint32 *) &tattoo, 1);
            gimp_item_set_tattoo (GIMP_ITEM (*layer), tattoo);
          }
          break;

        case PROP_PARASITES:
          {
            glong base = info->cp;

            while (info->cp - base < prop_size)
              {
                GimpParasite *p = xcf_load_parasite (info);

                if (! p)
                  return FALSE;

                gimp_item_parasite_attach (GIMP_ITEM (*layer), p, FALSE);
                gimp_parasite_free (p);
              }

            if (info->cp - base != prop_size)
              gimp_message_literal (info->gimp, G_OBJECT (info->progress),
				    GIMP_MESSAGE_WARNING,
				    "Error while loading a layer's parasites");
          }
          break;

        case PROP_TEXT_LAYER_FLAGS:
          info->cp += xcf_read_int32 (info->fp, text_layer_flags, 1);
          break;

        case PROP_GROUP_ITEM:
          {
            GimpLayer *group;

            group = gimp_group_layer_new (image);

            gimp_object_set_name (GIMP_OBJECT (group),
                                  gimp_object_get_name (*layer));

            GIMP_DRAWABLE (group)->private->type =
              gimp_drawable_type (GIMP_DRAWABLE (*layer));

            g_object_ref_sink (*layer);
            g_object_unref (*layer);
            *layer = group;
          }
          break;

        case PROP_ITEM_PATH:
          {
            glong  base = info->cp;
            GList *path = NULL;

            while (info->cp - base < prop_size)
              {
                guint32 index;

                if (xcf_read_int32 (info->fp, &index, 1) != 4)
                  {
                    g_list_free (path);
                    return FALSE;
                  }

                info->cp += 4;
                path = g_list_append (path, GUINT_TO_POINTER (index));
              }

            *item_path = path;
          }
          break;

        case PROP_GROUP_ITEM_FLAGS:
          info->cp += xcf_read_int32 (info->fp, group_layer_flags, 1);
          break;

        default:
#ifdef GIMP_UNSTABLE
          g_printerr ("unexpected/unknown layer property: %d (skipping)\n",
                      prop_type);
#endif
          if (! xcf_skip_unknown_prop (info, prop_size))
            return FALSE;
          break;
        }
    }

  return FALSE;
}