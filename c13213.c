hb_set_set (hb_set_t       *set,
	    const hb_set_t *other)
{
  if (unlikely (hb_object_is_immutable (set)))
    return;

  set->set (*other);
}