Bool rfbOptPamAuth(void)
{
  SecTypeData *s;

  for (s = secTypes; s->name != NULL; s++) {
    if ((!strcmp(s->name, "unixlogin") ||
         !strcmp(&s->name[strlen(s->name) - 5], "plain")) && s->enabled)
      return TRUE;
  }

  return FALSE;
}