xcf_load_old_paths (XcfInfo   *info,
                    GimpImage *image)
{
  guint32      num_paths;
  guint32      last_selected_row;
  GimpVectors *active_vectors;

  xcf_read_int32 (info, &last_selected_row, 1);
  xcf_read_int32 (info, &num_paths,         1);

  while (num_paths-- > 0)
    xcf_load_old_path (info, image);

  active_vectors =
    GIMP_VECTORS (gimp_container_get_child_by_index (gimp_image_get_vectors (image),
                                                     last_selected_row));

  if (active_vectors)
    gimp_image_set_active_vectors (image, active_vectors);

  return TRUE;
}