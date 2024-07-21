pcsc_get_status_wrapped (int slot, unsigned int *status)
{
  long err;
  reader_table_t slotp;
  size_t len, full_len;
  int i, n;
  unsigned char msgbuf[9];
  unsigned char buffer[16];
  int sw = SW_HOST_CARD_IO_ERROR;

  slotp = reader_table + slot;

  if (slotp->pcsc.req_fd == -1
      || slotp->pcsc.rsp_fd == -1
      || slotp->pcsc.pid == (pid_t)(-1) )
    {
      log_error ("pcsc_get_status: pcsc-wrapper not running\n");
      return sw;
    }

  msgbuf[0] = 0x04; /* STATUS command. */
  len = 0;
  msgbuf[1] = (len >> 24);
  msgbuf[2] = (len >> 16);
  msgbuf[3] = (len >>  8);
  msgbuf[4] = (len      );
  if ( writen (slotp->pcsc.req_fd, msgbuf, 5) )
    {
      log_error ("error sending PC/SC STATUS request: %s\n",
                 strerror (errno));
      goto command_failed;
    }

  /* Read the response. */
  if ((i=readn (slotp->pcsc.rsp_fd, msgbuf, 9, &len)) || len != 9)
    {
      log_error ("error receiving PC/SC STATUS response: %s\n",
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
  err = PCSC_ERR_MASK ((msgbuf[5] << 24) | (msgbuf[6] << 16)
                       | (msgbuf[7] << 8 ) | msgbuf[8]);
  if (err)
    {
      log_error ("pcsc_status failed: %s (0x%lx)\n",
                 pcsc_error_string (err), err);
      /* This is a proper error code, so return immediately.  */
      return pcsc_error_to_sw (err);
    }

  full_len = len;

  /* The current version returns 3 words but we allow also for old
     versions returning only 2 words. */
  n = 12 < len ? 12 : len;
  if ((i=readn (slotp->pcsc.rsp_fd, buffer, n, &len))
      || (len != 8 && len != 12))
    {
      log_error ("error receiving PC/SC STATUS response: %s\n",
                 i? strerror (errno) : "premature EOF");
      goto command_failed;
    }

  slotp->is_t0 = (len == 12 && !!(buffer[11] & PCSC_PROTOCOL_T0));


  full_len -= len;
  /* Newer versions of the wrapper might send more status bytes.
     Read them. */
  while (full_len)
    {
      unsigned char dummybuf[128];

      n = full_len < DIM (dummybuf) ? full_len : DIM (dummybuf);
      if ((i=readn (slotp->pcsc.rsp_fd, dummybuf, n, &len)) || len != n)
        {
          log_error ("error receiving PC/SC TRANSMIT response: %s\n",
                     i? strerror (errno) : "premature EOF");
          goto command_failed;
        }
      full_len -= n;
    }

  /* We are lucky: The wrapper already returns the data in the
     required format. */
  *status = buffer[3];
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