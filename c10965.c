static int reencrypt_load(struct crypt_device *cd, struct luks2_hdr *hdr,
		uint64_t device_size,
		const struct crypt_params_reencrypt *params,
		struct luks2_reencrypt **rh)
{
	int r;
	struct luks2_reencrypt *tmp = NULL;
	crypt_reencrypt_info ri = LUKS2_reencrypt_status(hdr);

	if (ri == CRYPT_REENCRYPT_CLEAN)
		r = reencrypt_load_clean(cd, hdr, device_size, &tmp, params);
	else if (ri == CRYPT_REENCRYPT_CRASH)
		r = reencrypt_load_crashed(cd, hdr, device_size, &tmp);
	else if (ri == CRYPT_REENCRYPT_NONE) {
		log_err(cd, _("Device not marked for LUKS2 reencryption."));
		return -EINVAL;
	} else
		r = -EINVAL;

	if (r < 0 || !tmp) {
		log_err(cd, _("Failed to load LUKS2 reencryption context."));
		return r;
	}

	*rh = tmp;

	return 0;
}