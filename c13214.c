hb_set_clear (hb_set_t *set)
{
  if (unlikely (hb_object_is_immutable (set)))
    return;

  set->clear ();
}