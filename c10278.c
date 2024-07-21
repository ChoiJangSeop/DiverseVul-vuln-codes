MagickExport void *ResetMagickMemory(void *memory,int byte,const size_t size)
{
  assert(memory != (void *) NULL);
  return(memset(memory,byte,size));
}