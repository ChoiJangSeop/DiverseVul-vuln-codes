pcsc_send_apdu_wrapped (int slot, unsigned char *apdu, size_t apdulen,
                        unsigned char *buffer, size_t *buflen,
                        pininfo_t *pininfo)
{
  long err;
  reader_table_t slotp;
  size_t len, full_len;
  int i, n;
  unsigned char msgbuf[9];
  int sw = SW_HOST_CARD_IO_ERROR;

  (void)pininfo;

  if (!reader_table[slot].atrlen
      && (err = reset_pcsc_reader (slot)))
    return err;

  if (DBG_CARD_IO)
    log_printhex ("  PCSC_data:", apdu, apdulen);

  slotp = reader_table + slot;

  if (slotp->pcsc.req_fd == -1
      || slotp->pcsc.rsp_fd == -1
      || slotp->pcsc.pid == (pid_t)(-1) )
    {
      log_error ("pcsc_send_apdu: pcsc-wrapper not running\n");
      return sw;
    }

  msgbuf[0] = 0x03; /* TRANSMIT command. */
  len = apdulen;
  msgbuf[1] = (len >> 24);
  msgbuf[2] = (len >> 16);
  msgbuf[3] = (len >>  8);
  msgbuf[4] = (len      );
  if ( writen (slotp->pcsc.req_fd, msgbuf, 5)
       || writen (slotp->pcsc.req_fd, apdu, len))
    {
      log_error ("error sending PC/SC TRANSMIT request: %s\n",
                 strerror (errno));
      goto command_failed;
    }

  /* Read the response. */
  if ((i=readn (slotp->pcsc.rsp_fd, msgbuf, 9, &len)) || len != 9)
    {
      log_error ("error receiving PC/SC TRANSMIT response: %s\n",
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
      log_error ("pcsc_transmit failed: %s (0x%lx)\n",
                 pcsc_error_string (err), err);
      return pcsc_error_to_sw (err);
    }

   full_len = len;

   n = *buflen < len ? *buflen : len;
   if ((i=readn (slotp->pcsc.rsp_fd, buffer, n, &len)) || len != n)
     {
       log_error ("error receiving PC/SC TRANSMIT response: %s\n",
                  i? strerror (errno) : "premature EOF");
       goto command_failed;
     }
   *buflen = n;

   full_len -= len;
   if (full_len)
     {
       log_error ("pcsc_send_apdu: provided buffer too short - truncated\n");
       err = SW_HOST_INV_VALUE;
     }
   /* We need to read any rest of the response, to keep the
      protocol running.  */
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

   return err;

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