ClearAllXtermOSC()
{
  int i;
  for (i = 3; i >= 0; i--)
    SetXtermOSC(i, 0);
  if (D_xtermosc[0])
    AddStr("\033[23;" WT_FLAG "t");	/* unstack titles (xterm patch #251) */
}