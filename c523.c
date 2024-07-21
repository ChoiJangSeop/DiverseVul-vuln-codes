acquire_buffer (gboolean *free_buffer)
{
  HB_Buffer buffer;

  if (G_LIKELY (G_TRYLOCK (cached_buffer)))
    {
      if (G_UNLIKELY (!cached_buffer))
	hb_buffer_new (&cached_buffer);

      buffer = cached_buffer;
      *free_buffer = FALSE;
    }
  else
    {
      hb_buffer_new (&buffer);
      *free_buffer = TRUE;
    }

  return buffer;
}