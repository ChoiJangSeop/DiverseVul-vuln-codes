hb_set_invert (hb_set_t *set)
{
  if (unlikely (hb_object_is_immutable (set)))
    return;

  set->invert ();
}