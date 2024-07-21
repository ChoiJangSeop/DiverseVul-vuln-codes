hb_set_union (hb_set_t       *set,
	      const hb_set_t *other)
{
  if (unlikely (hb_object_is_immutable (set)))
    return;

  set->union_ (*other);
}