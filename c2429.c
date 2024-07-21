v3_keyid (gcry_mpi_t a, u32 *ki)
{
  byte *buffer, *p;
  size_t nbytes;

  if (gcry_mpi_print (GCRYMPI_FMT_USG, NULL, 0, &nbytes, a ))
    BUG ();
  /* fixme: allocate it on the stack */
  buffer = xmalloc (nbytes);
  if (gcry_mpi_print( GCRYMPI_FMT_USG, buffer, nbytes, NULL, a ))
    BUG ();
  if (nbytes < 8) /* oops */
    ki[0] = ki[1] = 0;
  else
    {
      p = buffer + nbytes - 8;
      ki[0] = (p[0] << 24) | (p[1] <<16) | (p[2] << 8) | p[3];
      p += 4;
      ki[1] = (p[0] << 24) | (p[1] <<16) | (p[2] << 8) | p[3];
    }
  xfree (buffer);
  return ki[1];
}