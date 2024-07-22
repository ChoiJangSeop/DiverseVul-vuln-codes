MagickExport int LocaleUppercase(const int c)
{
#if defined(MAGICKCORE_LOCALE_SUPPORT)
  if (c_locale != (locale_t) NULL)
    return(toupper_l(c,c_locale));
#endif
  return(toupper(c));
}