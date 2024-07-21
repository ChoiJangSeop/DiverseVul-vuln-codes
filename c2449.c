control_pcsc_wrapped (int slot, pcsc_dword_t ioctl_code,
                      const unsigned char *cntlbuf, size_t len,
                      unsigned char *buffer, pcsc_dword_t *buflen)
{
  long err = PCSC_E_NOT_TRANSACTED;
  reader_table_t slotp;
  unsigned char msgbuf[9];
  int i, n;
  size_t full_len;

  slotp = reader_table + slot;

  msgbuf[0] = 0x06; /* CONTROL command. */
  msgbuf[1] = ((len + 4) >> 24);
  msgbuf[2] = ((len + 4) >> 16);
  msgbuf[3] = ((len + 4) >>  8);
  msgbuf[4] = ((len + 4)      );
  msgbuf[5] = (ioctl_code >> 24);
  msgbuf[6] = (ioctl_code >> 16);
  msgbuf[7] = (ioctl_code >>  8);
  msgbuf[8] = (ioctl_code      );
  if ( writen (slotp->pcsc.req_fd, msgbuf, 9)
       || writen (slotp->pcsc.req_fd, cntlbuf, len))
    {
      log_error ("error sending PC/SC CONTROL request: %s\n",
                 strerror (errno));
      goto command_failed;
    }

  /* Read the response. */
  if ((i=readn (slotp->pcsc.rsp_fd, msgbuf, 9, &len)) || len != 9)
    {
      log_error ("error receiving PC/SC CONTROL response: %s\n",
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
      log_error ("pcsc_control failed: %s (0x%lx)\n",
                 pcsc_error_string (err), err);
      return pcsc_error_to_sw (err);
    }

  full_len = len;

  n = *buflen < len ? *buflen : len;
  if ((i=readn (slotp->pcsc.rsp_fd, buffer, n, &len)) || len != n)
    {
      log_error ("error receiving PC/SC CONTROL response: %s\n",
                 i? strerror (errno) : "premature EOF");
      goto command_failed;
    }
  *buflen = n;

  full_len -= len;
  if (full_len)
    {
      log_error ("pcsc_send_apdu: provided buffer too short - truncated\n");
      err = PCSC_E_INVALID_VALUE;
    }
  /* We need to read any rest of the response, to keep the
     protocol running.  */
  while (full_len)
    {
      unsigned char dummybuf[128];

      n = full_len < DIM (dummybuf) ? full_len : DIM (dummybuf);
      if ((i=readn (slotp->pcsc.rsp_fd, dummybuf, n, &len)) || len != n)
        {
          log_error ("error receiving PC/SC CONTROL response: %s\n",
                     i? strerror (errno) : "premature EOF");
          goto command_failed;
        }
      full_len -= n;
    }

  if (!err)
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
  return pcsc_error_to_sw (err);
}