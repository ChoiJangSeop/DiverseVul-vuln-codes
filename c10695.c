ds_resize (dynamic_string *string, int size)
{
  if (size > string->ds_length)
    {
      string->ds_length = size;
      string->ds_string = (char *) xrealloc ((char *) string->ds_string, size);
    }
}