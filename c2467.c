send_fprtime_if_not_null (ctrl_t ctrl, const char *keyword,
                          int number, const unsigned char *stamp)
{
  char numbuf1[50], numbuf2[50];
  unsigned long value;

  value = (stamp[0] << 24) | (stamp[1]<<16) | (stamp[2]<<8) | stamp[3];
  if (!value)
    return;
  sprintf (numbuf1, "%d", number);
  sprintf (numbuf2, "%lu", value);
  send_status_info (ctrl, keyword,
                    numbuf1, (size_t)strlen(numbuf1),
                    numbuf2, (size_t)strlen(numbuf2), NULL, 0);
}