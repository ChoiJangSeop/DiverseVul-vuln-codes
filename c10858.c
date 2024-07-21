set_fflags_platform(struct archive_write_disk *a, int fd, const char *name,
    mode_t mode, unsigned long set, unsigned long clear)
{
	(void)a; /* UNUSED */
	(void)fd; /* UNUSED */
	(void)name; /* UNUSED */
	(void)mode; /* UNUSED */
	(void)set; /* UNUSED */
	(void)clear; /* UNUSED */
	return (ARCHIVE_OK);
}