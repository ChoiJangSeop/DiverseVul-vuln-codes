ModuleExport size_t RegisterXWDImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("XWD");
#if defined(MAGICKCORE_X11_DELEGATE)
  entry->decoder=(DecodeImageHandler *) ReadXWDImage;
  entry->encoder=(EncodeImageHandler *) WriteXWDImage;
#endif
  entry->magick=(IsImageFormatHandler *) IsXWD;
  entry->adjoin=MagickFalse;
  entry->description=ConstantString("X Windows system window dump (color)");
  entry->module=ConstantString("XWD");
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}