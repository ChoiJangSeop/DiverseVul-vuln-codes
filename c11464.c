xcf_load_old_path (XcfInfo   *info,
                   GimpImage *image)
{
  gchar                  *name;
  guint32                 locked;
  guint8                  state;
  guint32                 closed;
  guint32                 num_points;
  guint32                 version; /* changed from num_paths */
  GimpTattoo              tattoo = 0;
  GimpVectors            *vectors;
  GimpVectorsCompatPoint *points;
  gint                    i;

  xcf_read_string (info, &name,       1);
  xcf_read_int32  (info, &locked,     1);
  xcf_read_int8   (info, &state,      1);
  xcf_read_int32  (info, &closed,     1);
  xcf_read_int32  (info, &num_points, 1);
  xcf_read_int32  (info, &version,    1);

  if (version == 2)
    {
      guint32 dummy;

      /* Had extra type field and points are stored as doubles */
      xcf_read_int32 (info, (guint32 *) &dummy, 1);
    }
  else if (version == 3)
    {
      guint32 dummy;

      /* Has extra tattoo field */
      xcf_read_int32 (info, (guint32 *) &dummy,  1);
      xcf_read_int32 (info, (guint32 *) &tattoo, 1);
    }
  else if (version != 1)
    {
      g_printerr ("Unknown path type. Possibly corrupt XCF file");

      g_free (name);
      return FALSE;
    }

  /* skip empty compatibility paths */
  if (num_points == 0)
    {
      g_free (name);
      return FALSE;
    }

  points = g_new0 (GimpVectorsCompatPoint, num_points);

  for (i = 0; i < num_points; i++)
    {
      if (version == 1)
        {
          gint32 x;
          gint32 y;

          xcf_read_int32 (info, &points[i].type, 1);
          xcf_read_int32 (info, (guint32 *) &x,  1);
          xcf_read_int32 (info, (guint32 *) &y,  1);

          points[i].x = x;
          points[i].y = y;
        }
      else
        {
          gfloat x;
          gfloat y;

          xcf_read_int32 (info, &points[i].type, 1);
          xcf_read_float (info, &x,              1);
          xcf_read_float (info, &y,              1);

          points[i].x = x;
          points[i].y = y;
        }
    }

  vectors = gimp_vectors_compat_new (image, name, points, num_points, closed);

  g_free (name);
  g_free (points);

  if (locked)
    info->linked_paths = g_list_prepend (info->linked_paths, vectors);

  if (tattoo)
    gimp_item_set_tattoo (GIMP_ITEM (vectors), tattoo);

  gimp_image_add_vectors (image, vectors,
                          NULL, /* can't be a tree */
                          gimp_container_get_n_children (gimp_image_get_vectors (image)),
                          FALSE);

  return TRUE;
}