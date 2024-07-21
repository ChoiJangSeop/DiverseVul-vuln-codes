int message(int priority, const char *msg) {
    char buf[1024];

    /* only handle errno if this is not an informational message */
    if(errno && priority < 5) {
	sprintf(buf, "%s: %s", msg, strerror(errno));
	errno = 0;
    } else strcpy(buf, msg);
    if(use_syslog) syslog(priority, "%s", buf);
    else           fprintf(stderr, "%s: %s\n", progname, buf);
    return(0);
}