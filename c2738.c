set_lenIV(char *line)
{
  char *p = strstr(line, "/lenIV ");

  /* Allow lenIV to be negative. Thanks to Tom Kacvinsky <tjk@ams.org> */
  if (p && (isdigit(p[7]) || p[7] == '+' || p[7] == '-')) {
    lenIV = atoi(p + 7);
  }
}