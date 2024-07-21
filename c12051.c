MagickExport MagickBooleanType RegisterStaticModule(const char *module,
  ExceptionInfo *exception)
{
  char
    module_name[MagickPathExtent];

  PolicyRights
    rights;

  const CoderInfo
    *p;

  size_t
    extent;

  ssize_t
    i;

  /*
    Assign module name from alias.
  */
  assert(module != (const char *) NULL);
  (void) CopyMagickString(module_name,module,MagickPathExtent);
  p=GetCoderInfo(module,exception);
  if (p != (CoderInfo *) NULL)
    (void) CopyMagickString(module_name,p->name,MagickPathExtent);
  rights=AllPolicyRights;
  if (IsRightsAuthorized(ModulePolicyDomain,rights,module_name) == MagickFalse)
    {
      errno=EPERM;
      (void) ThrowMagickException(exception,GetMagickModule(),PolicyError,
        "NotAuthorized","`%s'",module);
      return(MagickFalse);
    }
  extent=sizeof(MagickModules)/sizeof(MagickModules[0]);
  for (i=0; i < (ssize_t) extent; i++)
    if (LocaleCompare(MagickModules[i].module,module_name) == 0)
      {
        if (MagickModules[i].registered == MagickFalse)
          {
            (void) (MagickModules[i].register_module)();
            MagickModules[i].registered=MagickTrue;
          }
        return(MagickTrue);
      }
  return(MagickFalse);
}