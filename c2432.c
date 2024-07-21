close_pcsc_reader_wrapped (int slot)
{
  long err;
  reader_table_t slotp;
  size_t len;
  int i;
  unsigned char msgbuf[9];

  slotp = reader_table + slot;

  if (slotp->pcsc.req_fd == -1
      || slotp->pcsc.rsp_fd == -1
      || slotp->pcsc.pid == (pid_t)(-1) )
    {
      log_error ("close_pcsc_reader: pcsc-wrapper not running\n");
      return 0;
    }

  msgbuf[0] = 0x02; /* CLOSE command. */
  len = 0;
  msgbuf[1] = (len >> 24);
  msgbuf[2] = (len >> 16);
  msgbuf[3] = (len >>  8);
  msgbuf[4] = (len      );
  if ( writen (slotp->pcsc.req_fd, msgbuf, 5) )
    {
      log_error ("error sending PC/SC CLOSE request: %s\n",
                 strerror (errno));
      goto command_failed;
    }

  /* Read the response. */
  if ((i=readn (slotp->pcsc.rsp_fd, msgbuf, 9, &len)) || len != 9)
    {
      log_error ("error receiving PC/SC CLOSE response: %s\n",
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
    log_error ("pcsc_close failed: %s (0x%lx)\n",
               pcsc_error_string (err), err);

  /* We will close the wrapper in any case - errors are merely
     informational. */

 command_failed:
  close (slotp->pcsc.req_fd);
  close (slotp->pcsc.rsp_fd);
  slotp->pcsc.req_fd = -1;
  slotp->pcsc.rsp_fd = -1;
  if (slotp->pcsc.pid != -1)
    kill (slotp->pcsc.pid, SIGTERM);
  slotp->pcsc.pid = (pid_t)(-1);
  slotp->used = 0;
  return 0;
}