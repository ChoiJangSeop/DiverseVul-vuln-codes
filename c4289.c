MagickExport void *AcquireQuantumMemory(const size_t count,const size_t quantum)
{
  size_t
    size;

  size=count*quantum;
  if ((count == 0) || (quantum != (size/count)))
    {
      errno=ENOMEM;
      return((void *) NULL);
    }
  return(AcquireMagickMemory(size));
}