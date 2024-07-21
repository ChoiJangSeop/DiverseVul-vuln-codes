ModuleExport size_t RegisterWEBPImage(void)
{
  char
    version[MagickPathExtent];

  MagickInfo
    *entry;

  *version='\0';
  entry=AcquireMagickInfo("WEBP","WEBP","WebP Image Format");
#if defined(MAGICKCORE_WEBP_DELEGATE)
  entry->decoder=(DecodeImageHandler *) ReadWEBPImage;
  entry->encoder=(EncodeImageHandler *) WriteWEBPImage;
  (void) FormatLocaleString(version,MagickPathExtent,"libwebp %d.%d.%d [%04X]",
    (WebPGetDecoderVersion() >> 16) & 0xff,
    (WebPGetDecoderVersion() >> 8) & 0xff,
    (WebPGetDecoderVersion() >> 0) & 0xff,WEBP_DECODER_ABI_VERSION);
#endif
  entry->mime_type=ConstantString("image/webp");
  entry->flags^=CoderAdjoinFlag;
  entry->magick=(IsImageFormatHandler *) IsWEBP;
  if (*version != '\0')
    entry->version=ConstantString(version);
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}