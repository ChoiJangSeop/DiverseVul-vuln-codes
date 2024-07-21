ModuleExport size_t RegisterSFWImage(void)
{
  MagickInfo
    *entry;

  entry=AcquireMagickInfo("SFW","SFW","Seattle Film Works");
  entry->decoder=(DecodeImageHandler *) ReadSFWImage;
  entry->magick=(IsImageFormatHandler *) IsSFW;
  entry->flags^=CoderAdjoinFlag;
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}