sv_locale (name)
     char *name;
{
  char *v;

  v = get_string_value (name);
  if (name[0] == 'L' && name[1] == 'A')	/* LANG */
    set_lang (name, v);
  else
    set_locale_var (name, v);		/* LC_*, TEXTDOMAIN* */
}