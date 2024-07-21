reset_pcsc_reader_wrapped (int slot)
{
  long err;
  reader_table_t slotp;
  size_t len;
  int i, n;
  unsigned char msgbuf[9];
  unsigned int dummy_status;
  int sw = SW_HOST_CARD_IO_ERROR;

  slotp = reader_table + slot;

  if (slotp->pcsc.req_fd == -1
      || slotp->pcsc.rsp_fd == -1
      || slotp->pcsc.pid == (pid_t)(-1) )
    {
      log_error ("pcsc_get_status: pcsc-wrapper not running\n");
      return sw;
    }

  msgbuf[0] = 0x05; /* RESET command. */
  len = 0;
  msgbuf[1] = (len >> 24);
  msgbuf[2] = (len >> 16);
  msgbuf[3] = (len >>  8);
  msgbuf[4] = (len      );
  if ( writen (slotp->pcsc.req_fd, msgbuf, 5) )
    {
      log_error ("error sending PC/SC RESET request: %s\n",
                 strerror (errno));
      goto command_failed;
    }

  /* Read the response. */
  if ((i=readn (slotp->pcsc.rsp_fd, msgbuf, 9, &len)) || len != 9)
    {
      log_error ("error receiving PC/SC RESET response: %s\n",
                 i? strerror (errno) : "premature EOF");
      goto command_failed;
    }
  len = (msgbuf[1] << 24) | (msgbuf[2] << 16) | (msgbuf[3] << 8 ) | msgbuf[4];
  if (msgbuf[0] != 0x81 || len < 4)
    {
      log_error ("invalid response header from PC/SC received\n");
      goto command_failed;
    }
  len -= 4; /* Already read the error code. */
  if (len > DIM (slotp->atr))
    {
      log_error ("PC/SC returned a too large ATR (len=%lx)\n",
                 (unsigned long)len);
      sw = SW_HOST_GENERAL_ERROR;
      goto command_failed;
    }
  err = PCSC_ERR_MASK ((msgbuf[5] << 24) | (msgbuf[6] << 16)
                       | (msgbuf[7] << 8 ) | msgbuf[8]);
  if (err)
    {
      log_error ("PC/SC RESET failed: %s (0x%lx)\n",
                 pcsc_error_string (err), err);
      /* If the error code is no smart card, we should not considere
         this a major error and close the wrapper.  */
      sw = pcsc_error_to_sw (err);
      if (err == PCSC_E_NO_SMARTCARD)
        return sw;
      goto command_failed;
    }

  /* The open function may return a zero for the ATR length to
     indicate that no card is present.  */
  n = len;
  if (n)
    {
      if ((i=readn (slotp->pcsc.rsp_fd, slotp->atr, n, &len)) || len != n)
        {
          log_error ("error receiving PC/SC RESET response: %s\n",
                     i? strerror (errno) : "premature EOF");
          goto command_failed;
        }
    }
  slotp->atrlen = len;

  /* Read the status so that IS_T0 will be set. */
  pcsc_get_status (slot, &dummy_status);

  return 0;

 command_failed:
  close (slotp->pcsc.req_fd);
  close (slotp->pcsc.rsp_fd);
  slotp->pcsc.req_fd = -1;
  slotp->pcsc.rsp_fd = -1;
  if (slotp->pcsc.pid != -1)
    kill (slotp->pcsc.pid, SIGTERM);
  slotp->pcsc.pid = (pid_t)(-1);
  slotp->used = 0;
  return sw;
}