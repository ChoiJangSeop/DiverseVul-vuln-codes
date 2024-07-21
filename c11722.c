static ssize_t show_crash_notes_size(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	ssize_t rc;

	rc = sprintf(buf, "%zu\n", sizeof(note_buf_t));
	return rc;
}