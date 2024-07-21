parse_header (const char *filename,
              const unsigned char *packet, size_t packetlen,
              size_t *r_headerlen)
{
  unsigned int len;

  if (packetlen != 32)
    return gpg_error (GPG_ERR_BUG);

  len = ((packet[2] << 24) | (packet[3] << 16)
         | (packet[4] << 8) | packet[5]);
  if (packet[0] != (0xc0|61) || len < 26
      || memcmp (packet+6, "GnuPG/G13", 10))
    {
      log_error ("file '%s' is not valid container\n", filename);
      return gpg_error (GPG_ERR_INV_OBJ);
    }
  if (packet[16] != 1)
    {
      log_error ("unknown version %u of container '%s'\n",
                 (unsigned int)packet[16], filename);
      return gpg_error (GPG_ERR_INV_OBJ);
    }
  if (packet[17] || packet[18]
      || packet[26] || packet[27] || packet[28] || packet[29]
      || packet[30] || packet[31])
    log_info ("WARNING: unknown meta information in '%s'\n", filename);
  if (packet[19])
    log_info ("WARNING: OS flag is not supported in '%s'\n", filename);
  if (packet[24] != 1 || packet[25] != 0)
    {
      log_error ("meta data copies in '%s' are not supported\n", filename);
      return gpg_error (GPG_ERR_NOT_IMPLEMENTED);
    }

  len = ((packet[20] << 24) | (packet[21] << 16)
         | (packet[22] << 8) | packet[23]);

  /* Do a basic sanity check on the length.  */
  if (len < 32 || len > 1024*1024)
    {
      log_error ("bad length given in container '%s'\n", filename);
      return gpg_error (GPG_ERR_INV_OBJ);
    }

  *r_headerlen = len;
  return 0;
}