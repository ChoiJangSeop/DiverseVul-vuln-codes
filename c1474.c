PS_READ_FUNC(mm)
{
	PS_MM_DATA;
	ps_sd *sd;
	int ret = FAILURE;

	mm_lock(data->mm, MM_LOCK_RD);

	sd = ps_sd_lookup(data, key, 0);
	if (sd) {
		*vallen = sd->datalen;
		*val = emalloc(sd->datalen + 1);
		memcpy(*val, sd->data, sd->datalen);
		(*val)[sd->datalen] = '\0';
		ret = SUCCESS;
	}

	mm_unlock(data->mm);

	return ret;
}