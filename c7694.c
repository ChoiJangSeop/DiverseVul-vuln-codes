static PixelChannels **AcquirePixelThreadSet(const Image *images)
{
  const Image
    *next;

  PixelChannels
    **pixels;

  register ssize_t
    i;

  size_t
    columns,
    number_threads;

  number_threads=(size_t) GetMagickResourceLimit(ThreadResource);
  pixels=(PixelChannels **) AcquireQuantumMemory(number_threads,
    sizeof(*pixels));
  if (pixels == (PixelChannels **) NULL)
    return((PixelChannels **) NULL);
  (void) memset(pixels,0,number_threads*sizeof(*pixels));
  columns=images->columns;
  for (next=images; next != (Image *) NULL; next=next->next)
    columns=MagickMax(next->columns,columns);
  for (i=0; i < (ssize_t) number_threads; i++)
  {
    register ssize_t
      j;

    pixels[i]=(PixelChannels *) AcquireQuantumMemory(columns,sizeof(**pixels));
    if (pixels[i] == (PixelChannels *) NULL)
      return(DestroyPixelThreadSet(pixels));
    for (j=0; j < (ssize_t) columns; j++)
    {
      register ssize_t
        k;

      for (k=0; k < MaxPixelChannels; k++)
        pixels[i][j].channel[k]=0.0;
    }
  }
  return(pixels);
}