static int _open_and_activate_reencrypt_device(struct crypt_device *cd,
	struct luks2_hdr *hdr,
	int keyslot,
	const char *name,
	const char *passphrase,
	size_t passphrase_size,
	uint32_t flags)
{
	bool dynamic_size;
	crypt_reencrypt_info ri;
	uint64_t minimal_size, device_size;
	struct volume_key *vks = NULL;
	int r = 0;
	struct crypt_lock_handle *reencrypt_lock = NULL;

	if (crypt_use_keyring_for_vk(cd))
		flags |= CRYPT_ACTIVATE_KEYRING_KEY;

	r = LUKS2_reencrypt_lock(cd, &reencrypt_lock);
	if (r) {
		if (r == -EBUSY)
			log_err(cd, _("Reencryption in-progress. Cannot activate device."));
		else
			log_err(cd, _("Failed to get reencryption lock."));
		return r;
	}

	if ((r = crypt_load(cd, CRYPT_LUKS2, NULL)))
		goto out;

	ri = LUKS2_reencrypt_status(hdr);

	if (ri == CRYPT_REENCRYPT_CRASH) {
		r = LUKS2_reencrypt_locked_recovery_by_passphrase(cd, keyslot,
				keyslot, passphrase, passphrase_size, flags, &vks);
		if (r < 0) {
			log_err(cd, _("LUKS2 reencryption recovery failed."));
			goto out;
		}
		keyslot = r;

		ri = LUKS2_reencrypt_status(hdr);
	}

	/* recovery finished reencryption or it's already finished */
	if (ri == CRYPT_REENCRYPT_NONE) {
		crypt_drop_keyring_key(cd, vks);
		crypt_free_volume_key(vks);
		LUKS2_reencrypt_unlock(cd, reencrypt_lock);
		return _open_and_activate(cd, keyslot, name, passphrase, passphrase_size, flags);
	}

	if (ri > CRYPT_REENCRYPT_CLEAN) {
		r = -EINVAL;
		goto out;
	}

	if (LUKS2_get_data_size(hdr, &minimal_size, &dynamic_size))
		goto out;

	if (!vks) {
		r = _open_all_keys(cd, hdr, keyslot, passphrase, passphrase_size, flags, &vks);
		if (r >= 0)
			keyslot = r;
	}

	log_dbg(cd, "Entering clean reencryption state mode.");

	if (r >= 0)
		r = LUKS2_reencrypt_check_device_size(cd, hdr, minimal_size, &device_size, true, dynamic_size);

	if (r >= 0)
		r = LUKS2_activate_multi(cd, name, vks, device_size >> SECTOR_SHIFT, flags);
out:
	LUKS2_reencrypt_unlock(cd, reencrypt_lock);
	if (r < 0)
		crypt_drop_keyring_key(cd, vks);
	crypt_free_volume_key(vks);

	return r < 0 ? r : keyslot;
}