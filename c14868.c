dwg_section_wtype (const DWGCHAR *restrict wname)
{
  DWGCHAR *wp;
  char name[24];
  uint16_t c;
  int i = 0;

  if (wname == NULL)
    return SECTION_UNKNOWN; // but could also be INFO or SYSTEM_MAP
  wp = (DWGCHAR *)wname;
  while ((c = *wp++))
    {
      name[i++] = (char)(c & 0xff);
    }
  name[i] = '\0';
  return dwg_section_type (name);
}