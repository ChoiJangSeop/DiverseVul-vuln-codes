hb_set_symmetric_difference (hb_set_t       *set,
			     const hb_set_t *other)
{
  if (unlikely (hb_object_is_immutable (set)))
    return;

  set->symmetric_difference (*other);
}