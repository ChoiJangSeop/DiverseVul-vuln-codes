MagickExport MagickBooleanType OpenStream(const ImageInfo *image_info,
  StreamInfo *stream_info,const char *filename,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  (void) CopyMagickString(stream_info->stream->filename,filename,MagickPathExtent);
  status=OpenBlob(image_info,stream_info->stream,WriteBinaryBlobMode,exception);
  return(status);
}