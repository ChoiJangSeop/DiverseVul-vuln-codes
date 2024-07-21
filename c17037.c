RefreshXtermOSC()
{
  int i;
  struct win *p;

  p = Layer2Window(D_forecv->c_layer);
  for (i = 3; i >=0; i--)
    SetXtermOSC(i, p ? p->w_xtermosc[i] : 0);
}