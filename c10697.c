ds_init (dynamic_string *string, int size)
{
  string->ds_length = size;
  string->ds_string = (char *) xmalloc (size);
}