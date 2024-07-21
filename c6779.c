cgi_set_sid(void)
{
  char			buffer[512],	/* SID data */
			sid[33];	/* SID string */
  unsigned char		sum[16];	/* MD5 sum */
  const char		*remote_addr,	/* REMOTE_ADDR */
			*server_name,	/* SERVER_NAME */
			*server_port;	/* SERVER_PORT */


  if ((remote_addr = getenv("REMOTE_ADDR")) == NULL)
    remote_addr = "REMOTE_ADDR";
  if ((server_name = getenv("SERVER_NAME")) == NULL)
    server_name = "SERVER_NAME";
  if ((server_port = getenv("SERVER_PORT")) == NULL)
    server_port = "SERVER_PORT";

  CUPS_SRAND(time(NULL));
  snprintf(buffer, sizeof(buffer), "%s:%s:%s:%02X%02X%02X%02X%02X%02X%02X%02X",
           remote_addr, server_name, server_port,
	   (unsigned)CUPS_RAND() & 255, (unsigned)CUPS_RAND() & 255,
	   (unsigned)CUPS_RAND() & 255, (unsigned)CUPS_RAND() & 255,
	   (unsigned)CUPS_RAND() & 255, (unsigned)CUPS_RAND() & 255,
	   (unsigned)CUPS_RAND() & 255, (unsigned)CUPS_RAND() & 255);
  cupsHashData("md5", (unsigned char *)buffer, strlen(buffer), sum, sizeof(sum));

  cgiSetCookie(CUPS_SID, cupsHashString(sum, sizeof(sum), sid, sizeof(sid)), "/", NULL, 0, 0);

  return (cupsGetOption(CUPS_SID, num_cookies, cookies));
}