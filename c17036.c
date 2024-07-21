SetXtermOSC(i, s)
int i;
char *s;
{
  static char *oscs[][2] = {
    { WT_FLAG ";", "screen" }, /* set window title */
    { "20;", "" },      /* background */
    { "39;", "black" }, /* default foreground (black?) */
    { "49;", "white" }  /* default background (white?) */
  };

  ASSERT(display);
  if (!D_CXT)
    return;
  if (!s)
    s = "";
  if (!D_xtermosc[i] && !*s)
    return;
  if (i == 0 && !D_xtermosc[0])
    AddStr("\033[22;" WT_FLAG "t");	/* stack titles (xterm patch #251) */
  if (!*s)
    s = oscs[i][1];
  D_xtermosc[i] = 1;
  AddStr("\033]");
  AddStr(oscs[i][0]);
  AddStr(s);
  AddChar(7);
}