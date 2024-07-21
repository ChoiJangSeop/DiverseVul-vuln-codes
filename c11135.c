ico_read_init (FILE *fp)
{
  IcoFileHeader header;
  /* read and check file header */
  if (!ico_read_int16 (fp, &header.reserved, 1)
      || !ico_read_int16 (fp, &header.resource_type, 1)
      || !ico_read_int16 (fp, &header.icon_count, 1)
      || header.reserved != 0
      || header.resource_type != 1)
    {
      return 0;
    }
  return header.icon_count;
}