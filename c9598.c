WandExport MagickBooleanType PixelSyncIterator(PixelIterator *iterator)
{
  MagickBooleanType
    status;

  register ssize_t
    x;

  register Quantum
    *_magickcore_restrict pixels;

  assert(iterator != (const PixelIterator *) NULL);
  assert(iterator->signature == MagickWandSignature);
  if (iterator->debug != MagickFalse)
    (void) LogMagickEvent(WandEvent,GetMagickModule(),"%s",iterator->name);
  status=SetCacheViewStorageClass(iterator->view,DirectClass,
    iterator->exception);
  if (status == MagickFalse)
    return(MagickFalse);
  pixels=GetCacheViewAuthenticPixels(iterator->view,iterator->region.x,
    iterator->region.y+iterator->y,iterator->region.width,1,
    iterator->exception);
  if (pixels == (Quantum *) NULL)
    return(MagickFalse);
  for (x=0; x < (ssize_t) iterator->region.width; x++)
  {
    PixelGetQuantumPixel(GetCacheViewImage(iterator->view),
      iterator->pixel_wands[x],pixels);
    pixels+=GetPixelChannels(GetCacheViewImage(iterator->view));
  }
  if (SyncCacheViewAuthenticPixels(iterator->view,iterator->exception) == MagickFalse)
    return(MagickFalse);
  return(MagickTrue);
}