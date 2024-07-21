ModuleExport size_t RegisterRLEImage(void)
{
  MagickInfo
    *entry;

  entry=AcquireMagickInfo("RLE","RLE","Utah Run length encoded image");
  entry->decoder=(DecodeImageHandler *) ReadRLEImage;
  entry->magick=(IsImageFormatHandler *) IsRLE;
  entry->flags^=CoderAdjoinFlag;
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}