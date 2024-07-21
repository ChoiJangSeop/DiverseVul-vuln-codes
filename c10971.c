static int _open_and_activate_luks2(struct crypt_device *cd,
	int keyslot,
	const char *name,
	const char *passphrase,
	size_t passphrase_size,
	uint32_t flags)
{
	crypt_reencrypt_info ri;
	int r;
	struct luks2_hdr *hdr = &cd->u.luks2.hdr;

	ri = LUKS2_reencrypt_status(hdr);
	if (ri == CRYPT_REENCRYPT_INVALID)
		return -EINVAL;

	if (ri > CRYPT_REENCRYPT_NONE) {
		if (name)
			r = _open_and_activate_reencrypt_device(cd, hdr, keyslot, name, passphrase,
					passphrase_size, flags);
		else
			r = _open_all_keys(cd, hdr, keyslot, passphrase,
					   passphrase_size, flags, NULL);
	} else
		r = _open_and_activate(cd, keyslot, name, passphrase,
				passphrase_size, flags);

	return r;
}