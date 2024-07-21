auth_getpwent (
  /* PARAMETERS */
  const char *login,			/* I: plaintext authenticator */
  const char *password,			/* I: plaintext password */
  const char *service __attribute__((unused)),
  const char *realm __attribute__((unused))
  /* END PARAMETERS */
  )
{
    /* VARIABLES */
    struct passwd *pw;			/* pointer to passwd file entry */
    int errnum;
    /* END VARIABLES */
  
    errno = 0;
    pw = getpwnam(login);
    errnum = errno;
    endpwent();

    if (pw == NULL) {
	if (errnum != 0) {
	    char *errstr;

	    if (flags & VERBOSE) {
		syslog(LOG_DEBUG, "DEBUG: auth_getpwent: getpwnam(%s) failure: %m", login);
	    }
	    if (asprintf(&errstr, "NO Username lookup failure: %s", strerror(errno)) == -1) {
		/* XXX the hidden strdup() will likely fail and return NULL here.... */
		RETURN("NO Username lookup failure: unknown error (ENOMEM formatting strerror())");
	    }
	    return errstr;
	} else {
	    if (flags & VERBOSE) {
		syslog(LOG_DEBUG, "DEBUG: auth_getpwent: getpwnam(%s): invalid username", login);
	    }
	    RETURN("NO Invalid username");
	}
    }

    if (strcmp(pw->pw_passwd, (const char *)crypt(password, pw->pw_passwd))) {
	if (flags & VERBOSE) {
	    syslog(LOG_DEBUG, "DEBUG: auth_getpwent: %s: invalid password", login);
	}
	RETURN("NO Incorrect password");
    }

    if (flags & VERBOSE) {
	syslog(LOG_DEBUG, "DEBUG: auth_getpwent: OK: %s", login);
    }
    RETURN("OK");
}