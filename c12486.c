MagickExport void RegisterStaticModules(void)
{
  size_t
    extent;

  ssize_t
    i;

  extent=sizeof(MagickModules)/sizeof(MagickModules[0]);
  for (i=0; i < (ssize_t) extent; i++)
  {
    if (MagickModules[i].registered == MagickFalse)
      {
        (void) (MagickModules[i].register_module)();
        MagickModules[i].registered=MagickTrue;
      }
  }
}