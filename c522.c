release_buffer (HB_Buffer buffer, gboolean free_buffer)
{
  if (G_LIKELY (!free_buffer))
    {
      hb_buffer_clear (buffer);
      G_UNLOCK (cached_buffer);
    }
  else
    hb_buffer_free (buffer);
}