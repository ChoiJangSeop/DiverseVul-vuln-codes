libraw_processed_image_t *LibRaw::dcraw_make_mem_thumb(int *errcode)
{
  if (!T.thumb)
  {
    if (!ID.toffset && !(imgdata.thumbnail.tlength > 0 &&
                         load_raw == &LibRaw::broadcom_load_raw) // RPi
    )
    {
      if (errcode)
        *errcode = LIBRAW_NO_THUMBNAIL;
    }
    else
    {
      if (errcode)
        *errcode = LIBRAW_OUT_OF_ORDER_CALL;
    }
    return NULL;
  }

  if (T.tformat == LIBRAW_THUMBNAIL_BITMAP)
  {
    libraw_processed_image_t *ret = (libraw_processed_image_t *)::malloc(
        sizeof(libraw_processed_image_t) + T.tlength);

    if (!ret)
    {
      if (errcode)
        *errcode = ENOMEM;
      return NULL;
    }

    memset(ret, 0, sizeof(libraw_processed_image_t));
    ret->type = LIBRAW_IMAGE_BITMAP;
    ret->height = T.theight;
    ret->width = T.twidth;
    ret->colors = 3;
    ret->bits = 8;
    ret->data_size = T.tlength;
    memmove(ret->data, T.thumb, T.tlength);
    if (errcode)
      *errcode = 0;
    return ret;
  }
  else if (T.tformat == LIBRAW_THUMBNAIL_JPEG)
  {
    ushort exif[5];
    int mk_exif = 0;
    if (strcmp(T.thumb + 6, "Exif"))
      mk_exif = 1;

    int dsize = T.tlength + mk_exif * (sizeof(exif) + sizeof(tiff_hdr));

    libraw_processed_image_t *ret = (libraw_processed_image_t *)::malloc(
        sizeof(libraw_processed_image_t) + dsize);

    if (!ret)
    {
      if (errcode)
        *errcode = ENOMEM;
      return NULL;
    }

    memset(ret, 0, sizeof(libraw_processed_image_t));

    ret->type = LIBRAW_IMAGE_JPEG;
    ret->data_size = dsize;

    ret->data[0] = 0xff;
    ret->data[1] = 0xd8;
    if (mk_exif)
    {
      struct tiff_hdr th;
      memcpy(exif, "\xff\xe1  Exif\0\0", 10);
      exif[1] = htons(8 + sizeof th);
      memmove(ret->data + 2, exif, sizeof(exif));
      tiff_head(&th, 0);
      memmove(ret->data + (2 + sizeof(exif)), &th, sizeof(th));
      memmove(ret->data + (2 + sizeof(exif) + sizeof(th)), T.thumb + 2,
              T.tlength - 2);
    }
    else
    {
      memmove(ret->data + 2, T.thumb + 2, T.tlength - 2);
    }
    if (errcode)
      *errcode = 0;
    return ret;
  }
  else
  {
    if (errcode)
      *errcode = LIBRAW_UNSUPPORTED_THUMBNAIL;
    return NULL;
  }
}