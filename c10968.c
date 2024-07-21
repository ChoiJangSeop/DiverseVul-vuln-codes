static int reencrypt_recovery(struct crypt_device *cd,
		struct luks2_hdr *hdr,
		uint64_t device_size,
		struct volume_key *vks)
{
	int r;
	struct luks2_reencrypt *rh = NULL;

	r = reencrypt_load(cd, hdr, device_size, NULL, &rh);
	if (r < 0) {
		log_err(cd, _("Failed to load LUKS2 reencryption context."));
		return r;
	}

	r = reencrypt_recover_segment(cd, hdr, rh, vks);
	if (r < 0)
		goto out;

	if ((r = reencrypt_assign_segments(cd, hdr, rh, 0, 0)))
		goto out;

	r = reencrypt_context_update(cd, rh);
	if (r) {
		log_err(cd, _("Failed to update reencryption context."));
		goto out;
	}

	r = reencrypt_teardown_ok(cd, hdr, rh);
	if (!r)
		r = LUKS2_hdr_write(cd, hdr);
out:
	LUKS2_reencrypt_free(cd, rh);

	return r;
}