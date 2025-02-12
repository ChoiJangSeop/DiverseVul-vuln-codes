MagickExport int LocaleUppercase(const int c)
{
  if (c < 0)
    return(c);
#if defined(MAGICKCORE_LOCALE_SUPPORT)
  if (c_locale != (locale_t) NULL)
    return(toupper_l((int) ((unsigned char) c),c_locale));
#endif
  return(toupper((int) ((unsigned char) c)));
}