int get_random_value(unsigned char *data, int length)
{
  SECStatus rv = PK11_GenerateRandom(data,length);
  if (rv != SECSuccess) {
    DBG1("couldn't generate random number: %s", SECU_Strerror(PR_GetError()));
  }
  return (rv == SECSuccess) ? 0 : -1;
}