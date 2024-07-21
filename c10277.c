MagickExport MagickBooleanType PasskeyDecipherImage(Image *image,
  const StringInfo *passkey,ExceptionInfo *exception)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  (void) passkey;
  ThrowBinaryException(ImageError,"CipherSupportNotEnabled",image->filename);
}