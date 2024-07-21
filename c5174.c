int CLASS parse_jpeg(int offset)
{
  int len, save, hlen, mark;
  fseek(ifp, offset, SEEK_SET);
  if (fgetc(ifp) != 0xff || fgetc(ifp) != 0xd8)
    return 0;

  while (fgetc(ifp) == 0xff && (mark = fgetc(ifp)) != 0xda)
  {
    order = 0x4d4d;
    len = get2() - 2;
    save = ftell(ifp);
    if (mark == 0xc0 || mark == 0xc3 || mark == 0xc9)
    {
      fgetc(ifp);
      raw_height = get2();
      raw_width = get2();
    }
    order = get2();
    hlen = get4();
    if (get4() == 0x48454150) /* "HEAP" */
    {
#ifdef LIBRAW_LIBRARY_BUILD
      imgdata.lens.makernotes.CameraMount = LIBRAW_MOUNT_FixedLens;
      imgdata.lens.makernotes.LensMount = LIBRAW_MOUNT_FixedLens;
#endif
      parse_ciff(save + hlen, len - hlen, 0);
    }
    if (parse_tiff(save + 6))
      apply_tiff();
    fseek(ifp, save + len, SEEK_SET);
  }
  return 1;
}