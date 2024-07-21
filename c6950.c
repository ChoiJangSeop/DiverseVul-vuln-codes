void CLASS parse_sinar_ia()
{
  int entries, off;
  char str[8], *cp;

  order = 0x4949;
  fseek(ifp, 4, SEEK_SET);
  entries = get4();
  fseek(ifp, get4(), SEEK_SET);
  while (entries--)
  {
    off = get4();
    get4();
    fread(str, 8, 1, ifp);
    if (!strcmp(str, "META"))
      meta_offset = off;
    if (!strcmp(str, "THUMB"))
      thumb_offset = off;
    if (!strcmp(str, "RAW0"))
      data_offset = off;
  }
  fseek(ifp, meta_offset + 20, SEEK_SET);
  fread(make, 64, 1, ifp);
  make[63] = 0;
  if ((cp = strchr(make, ' ')))
  {
    strcpy(model, cp + 1);
    *cp = 0;
  }
  raw_width = get2();
  raw_height = get2();
  load_raw = &CLASS unpacked_load_raw;
  thumb_width = (get4(), get2());
  thumb_height = get2();
  write_thumb = &CLASS ppm_thumb;
  maximum = 0x3fff;
}