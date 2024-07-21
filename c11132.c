ico_read_info (FILE *fp,
               gint  icon_count)
{
  gint            i;
  IcoFileEntry   *entries;
  IcoLoadInfo    *info;

  /* read icon entries */
  entries = g_new (IcoFileEntry, icon_count);
  if ( fread (entries, sizeof(IcoFileEntry), icon_count, fp) <= 0 )
    {
      g_free (entries);
      return NULL;
    }

  info = g_new (IcoLoadInfo, icon_count);
  for (i = 0; i < icon_count; i++)
    {
      info[i].width = entries[i].width;
      info[i].height = entries[i].height;
      info[i].bpp = GUINT16_FROM_LE (entries[i].bpp);
      info[i].size = GUINT32_FROM_LE (entries[i].size);
      info[i].offset = GUINT32_FROM_LE (entries[i].offset);

      if (info[i].width == 0 || info[i].height == 0)
        {
          ico_read_size (fp, info+i);
        }

      D(("ico_read_info: %ix%i (%i bits, size: %i, offset: %i)\n",
         info[i].width, info[i].height, info[i].bpp,
         info[i].size, info[i].offset));
    }

  g_free (entries);
  return info;
}