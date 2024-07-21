hb_map_clear (hb_map_t *map)
{
  if (unlikely (hb_object_is_immutable (map)))
    return;

  return map->clear ();
}