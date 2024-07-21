ModuleExport size_t RegisterPSImage(void)
{
  MagickInfo
    *entry;

  entry=AcquireMagickInfo("PS","EPI",
    "Encapsulated PostScript Interchange format");
  entry->decoder=(DecodeImageHandler *) ReadPSImage;
  entry->encoder=(EncodeImageHandler *) WritePSImage;
  entry->magick=(IsImageFormatHandler *) IsPS;
  entry->flags^=CoderAdjoinFlag;
  entry->flags^=CoderBlobSupportFlag;
  entry->mime_type=ConstantString("application/postscript");
  (void) RegisterMagickInfo(entry);
  entry=AcquireMagickInfo("PS","EPS","Encapsulated PostScript");
  entry->decoder=(DecodeImageHandler *) ReadPSImage;
  entry->encoder=(EncodeImageHandler *) WritePSImage;
  entry->magick=(IsImageFormatHandler *) IsPS;
  entry->flags^=CoderAdjoinFlag;
  entry->flags^=CoderBlobSupportFlag;
  entry->mime_type=ConstantString("application/postscript");
  (void) RegisterMagickInfo(entry);
  entry=AcquireMagickInfo("PS","EPSF","Encapsulated PostScript");
  entry->decoder=(DecodeImageHandler *) ReadPSImage;
  entry->encoder=(EncodeImageHandler *) WritePSImage;
  entry->magick=(IsImageFormatHandler *) IsPS;
  entry->flags^=CoderAdjoinFlag;
  entry->flags^=CoderBlobSupportFlag;
  entry->mime_type=ConstantString("application/postscript");
  (void) RegisterMagickInfo(entry);
  entry=AcquireMagickInfo("PS","EPSI",
    "Encapsulated PostScript Interchange format");
  entry->decoder=(DecodeImageHandler *) ReadPSImage;
  entry->encoder=(EncodeImageHandler *) WritePSImage;
  entry->magick=(IsImageFormatHandler *) IsPS;
  entry->flags^=CoderAdjoinFlag;
  entry->flags^=CoderBlobSupportFlag;
  entry->mime_type=ConstantString("application/postscript");
  (void) RegisterMagickInfo(entry);
  entry=AcquireMagickInfo("PS","PS","PostScript");
  entry->decoder=(DecodeImageHandler *) ReadPSImage;
  entry->encoder=(EncodeImageHandler *) WritePSImage;
  entry->magick=(IsImageFormatHandler *) IsPS;
  entry->mime_type=ConstantString("application/postscript");
  entry->flags^=CoderBlobSupportFlag;
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}