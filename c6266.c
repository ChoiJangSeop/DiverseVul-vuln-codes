static int valid_master_desc(const char *new_desc, const char *orig_desc)
{
	if (!memcmp(new_desc, KEY_TRUSTED_PREFIX, KEY_TRUSTED_PREFIX_LEN)) {
		if (strlen(new_desc) == KEY_TRUSTED_PREFIX_LEN)
			goto out;
		if (orig_desc)
			if (memcmp(new_desc, orig_desc, KEY_TRUSTED_PREFIX_LEN))
				goto out;
	} else if (!memcmp(new_desc, KEY_USER_PREFIX, KEY_USER_PREFIX_LEN)) {
		if (strlen(new_desc) == KEY_USER_PREFIX_LEN)
			goto out;
		if (orig_desc)
			if (memcmp(new_desc, orig_desc, KEY_USER_PREFIX_LEN))
				goto out;
	} else
		goto out;
	return 0;
out:
	return -EINVAL;
}