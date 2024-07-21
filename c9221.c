void LibRaw::selectCRXTrack(short maxTrack)
{
  if (maxTrack < 0)
    return;
  INT64 bitcounts[LIBRAW_CRXTRACKS_MAXCOUNT], maxbitcount = 0;
  uint32_t maxjpegbytes = 0;
  memset(bitcounts, 0, sizeof(bitcounts));
  for (int i = 0; i <= maxTrack && i < LIBRAW_CRXTRACKS_MAXCOUNT; i++)
  {
    crx_data_header_t *d = &libraw_internal_data.unpacker_data.crx_header[i];
    if (d->MediaType == 1) // RAW
    {
      bitcounts[i] = INT64(d->nBits) * INT64(d->f_width) * INT64(d->f_height);
      if (bitcounts[i] > maxbitcount)
        maxbitcount = bitcounts[i];
    }
    else if (d->MediaType == 2) // JPEG
    {
      if (d->MediaSize > maxjpegbytes)
      {
        maxjpegbytes = d->MediaSize;
        thumb_offset = d->MediaOffset;
        thumb_length = d->MediaSize;
      }
    }
  }
  if (maxbitcount < 8)
    return;
  int framei = -1, framecnt = 0;
  for (int i = 0; i <= maxTrack && i < LIBRAW_CRXTRACKS_MAXCOUNT; i++)
  {
    if (bitcounts[i] == maxbitcount)
    {
      if (framecnt <= (int)shot_select)
        framei = i;
      framecnt++;
    }
  }
  is_raw = framecnt;
  if (framei >= 0 && framei < LIBRAW_CRXTRACKS_MAXCOUNT)
  {
    crx_data_header_t *d =
        &libraw_internal_data.unpacker_data.crx_header[framei];
    data_offset = d->MediaOffset;
    data_size = d->MediaSize;
    raw_width = d->f_width;
    raw_height = d->f_height;
    load_raw = &LibRaw::crxLoadRaw;
    switch (d->cfaLayout)
    {
    case 0:
      filters = 0x94949494;
      break;
    case 1:
      filters = 0x61616161;
      break;
    case 2:
      filters = 0x49494949;
      break;
    case 3:
      filters = 0x16161616;
      break;
    }

    libraw_internal_data.unpacker_data.crx_track_selected = framei;

    int tiff_idx = -1;
    INT64 tpixels = 0;
    for (unsigned i = 0; i < tiff_nifds; i++)
      if (INT64(tiff_ifd[i].t_height) * INT64(tiff_ifd[i].t_height) > tpixels)
      {
        tpixels = INT64(tiff_ifd[i].t_height) * INT64(tiff_ifd[i].t_height);
        tiff_idx = i;
      }
    if (tiff_idx >= 0)
      flip = tiff_ifd[tiff_idx].t_flip;
  }
}