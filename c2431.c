open_pcsc_reader_wrapped (const char *portstr)
{
  int slot;
  reader_table_t slotp;
  int fd, rp[2], wp[2];
  int n, i;
  pid_t pid;
  size_t len;
  unsigned char msgbuf[9];
  int err;
  unsigned int dummy_status;

  /* Note that we use the constant and not the fucntion because this
     code won't be be used under Windows.  */
  const char *wrapperpgm = GNUPG_LIBEXECDIR "/gnupg-pcsc-wrapper";

  if (access (wrapperpgm, X_OK))
    {
      log_error ("can't run PC/SC access module '%s': %s\n",
                 wrapperpgm, strerror (errno));
      return -1;
    }

  slot = new_reader_slot ();
  if (slot == -1)
    return -1;
  slotp = reader_table + slot;

  /* Fire up the PC/SCc wrapper.  We don't use any fork/exec code from
     the common directy but implement it directly so that this file
     may still be source copied. */

  if (pipe (rp) == -1)
    {
      log_error ("error creating a pipe: %s\n", strerror (errno));
      slotp->used = 0;
      unlock_slot (slot);
      return -1;
    }
  if (pipe (wp) == -1)
    {
      log_error ("error creating a pipe: %s\n", strerror (errno));
      close (rp[0]);
      close (rp[1]);
      slotp->used = 0;
      unlock_slot (slot);
      return -1;
    }

  pid = fork ();
  if (pid == -1)
    {
      log_error ("error forking process: %s\n", strerror (errno));
      close (rp[0]);
      close (rp[1]);
      close (wp[0]);
      close (wp[1]);
      slotp->used = 0;
      unlock_slot (slot);
      return -1;
    }
  slotp->pcsc.pid = pid;

  if (!pid)
    { /*
         === Child ===
       */

      /* Double fork. */
      pid = fork ();
      if (pid == -1)
        _exit (31);
      if (pid)
        _exit (0); /* Immediate exit this parent, so that the child
                      gets cleaned up by the init process. */

      /* Connect our pipes. */
      if (wp[0] != 0 && dup2 (wp[0], 0) == -1)
        log_fatal ("dup2 stdin failed: %s\n", strerror (errno));
      if (rp[1] != 1 && dup2 (rp[1], 1) == -1)
        log_fatal ("dup2 stdout failed: %s\n", strerror (errno));

      /* Send stderr to the bit bucket. */
      fd = open ("/dev/null", O_WRONLY);
      if (fd == -1)
        log_fatal ("can't open '/dev/null': %s", strerror (errno));
      if (fd != 2 && dup2 (fd, 2) == -1)
        log_fatal ("dup2 stderr failed: %s\n", strerror (errno));

      /* Close all other files. */
      close_all_fds (3, NULL);

      execl (wrapperpgm,
             "pcsc-wrapper",
             "--",
             "1", /* API version */
             opt.pcsc_driver, /* Name of the PC/SC library. */
              NULL);
      _exit (31);
    }

  /*
     === Parent ===
   */
  close (wp[0]);
  close (rp[1]);
  slotp->pcsc.req_fd = wp[1];
  slotp->pcsc.rsp_fd = rp[0];

  /* Wait for the intermediate child to terminate. */
#ifdef USE_NPTH
#define WAIT npth_waitpid
#else
#define WAIT waitpid
#endif
  while ( (i=WAIT (pid, NULL, 0)) == -1 && errno == EINTR)
    ;
#undef WAIT

  /* Now send the open request. */
  msgbuf[0] = 0x01; /* OPEN command. */
  len = portstr? strlen (portstr):0;
  msgbuf[1] = (len >> 24);
  msgbuf[2] = (len >> 16);
  msgbuf[3] = (len >>  8);
  msgbuf[4] = (len      );
  if ( writen (slotp->pcsc.req_fd, msgbuf, 5)
       || (portstr && writen (slotp->pcsc.req_fd, portstr, len)))
    {
      log_error ("error sending PC/SC OPEN request: %s\n",
                 strerror (errno));
      goto command_failed;
    }
  /* Read the response. */
  if ((i=readn (slotp->pcsc.rsp_fd, msgbuf, 9, &len)) || len != 9)
    {
      log_error ("error receiving PC/SC OPEN response: %s\n",
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
      goto command_failed;
    }
  err = PCSC_ERR_MASK ((msgbuf[5] << 24) | (msgbuf[6] << 16)
                       | (msgbuf[7] << 8 ) | msgbuf[8]);

  if (err)
    {
      log_error ("PC/SC OPEN failed: %s\n", pcsc_error_string (err));
      goto command_failed;
    }

  slotp->last_status = 0;

  /* The open request may return a zero for the ATR length to
     indicate that no card is present.  */
  n = len;
  if (n)
    {
      if ((i=readn (slotp->pcsc.rsp_fd, slotp->atr, n, &len)) || len != n)
        {
          log_error ("error receiving PC/SC OPEN response: %s\n",
                     i? strerror (errno) : "premature EOF");
          goto command_failed;
        }
      /* If we got to here we know that a card is present
         and usable.  Thus remember this.  */
      slotp->last_status = (  APDU_CARD_USABLE
                            | APDU_CARD_PRESENT
                            | APDU_CARD_ACTIVE);
    }
  slotp->atrlen = len;

  reader_table[slot].close_reader = close_pcsc_reader;
  reader_table[slot].reset_reader = reset_pcsc_reader;
  reader_table[slot].get_status_reader = pcsc_get_status;
  reader_table[slot].send_apdu_reader = pcsc_send_apdu;
  reader_table[slot].dump_status_reader = dump_pcsc_reader_status;

  pcsc_vendor_specific_init (slot);

  /* Read the status so that IS_T0 will be set. */
  pcsc_get_status (slot, &dummy_status);

  dump_reader_status (slot);
  unlock_slot (slot);
  return slot;

 command_failed:
  close (slotp->pcsc.req_fd);
  close (slotp->pcsc.rsp_fd);
  slotp->pcsc.req_fd = -1;
  slotp->pcsc.rsp_fd = -1;
  if (slotp->pcsc.pid != -1)
    kill (slotp->pcsc.pid, SIGTERM);
  slotp->pcsc.pid = (pid_t)(-1);
  slotp->used = 0;
  unlock_slot (slot);
  /* There is no way to return SW. */
  return -1;

}