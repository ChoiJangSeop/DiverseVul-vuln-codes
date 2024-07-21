static int fsmMkdir(const char *path, mode_t mode)
{
    int rc = mkdir(path, (mode & 07777));
    if (_fsm_debug)
	rpmlog(RPMLOG_DEBUG, " %8s (%s, 0%04o) %s\n", __func__,
	       path, (unsigned)(mode & 07777),
	       (rc < 0 ? strerror(errno) : ""));
    if (rc < 0)	rc = RPMERR_MKDIR_FAILED;
    return rc;
}