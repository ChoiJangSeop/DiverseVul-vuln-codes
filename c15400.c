_nss_dns_gethostbyname4_r (const char *name, struct gaih_addrtuple **pat,
			   char *buffer, size_t buflen, int *errnop,
			   int *herrnop, int32_t *ttlp)
{
  if (__res_maybe_init (&_res, 0) == -1)
    return NSS_STATUS_UNAVAIL;

  /*
   * if there aren't any dots, it could be a user-level alias.
   * this is also done in res_query() since we are not the only
   * function that looks up host names.
   */
  if (strchr (name, '.') == NULL)
    {
      char *tmp = alloca (NS_MAXDNAME);
      const char *cp = res_hostalias (&_res, name, tmp, NS_MAXDNAME);
      if (cp != NULL)
	name = cp;
    }

  union
  {
    querybuf *buf;
    u_char *ptr;
  } host_buffer;
  querybuf *orig_host_buffer;
  host_buffer.buf = orig_host_buffer = (querybuf *) alloca (2048);
  u_char *ans2p = NULL;
  int nans2p = 0;
  int resplen2 = 0;
  int ans2p_malloced = 0;

  int olderr = errno;
  enum nss_status status;
  int n = __libc_res_nsearch (&_res, name, C_IN, T_UNSPEC,
			      host_buffer.buf->buf, 2048, &host_buffer.ptr,
			      &ans2p, &nans2p, &resplen2, &ans2p_malloced);
  if (n >= 0)
    {
      status = gaih_getanswer (host_buffer.buf, n, (const querybuf *) ans2p,
			       resplen2, name, pat, buffer, buflen,
			       errnop, herrnop, ttlp);
    }
  else
    {
      switch (errno)
	{
	case ESRCH:
	  status = NSS_STATUS_TRYAGAIN;
	  h_errno = TRY_AGAIN;
	  break;
	/* System has run out of file descriptors.  */
	case EMFILE:
	case ENFILE:
	  h_errno = NETDB_INTERNAL;
	  /* Fall through.  */
	case ECONNREFUSED:
	case ETIMEDOUT:
	  status = NSS_STATUS_UNAVAIL;
	  break;
	default:
	  status = NSS_STATUS_NOTFOUND;
	  break;
	}

      *herrnop = h_errno;
      if (h_errno == TRY_AGAIN)
	*errnop = EAGAIN;
      else
	__set_errno (olderr);
    }

  /* Check whether ans2p was separately allocated.  */
  if (ans2p_malloced)
    free (ans2p);

  if (host_buffer.buf != orig_host_buffer)
    free (host_buffer.buf);

  return status;
}