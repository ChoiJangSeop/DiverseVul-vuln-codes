static int reencrypt_load_by_passphrase(struct crypt_device *cd,
		const char *name,
		const char *passphrase,
		size_t passphrase_size,
		int keyslot_old,
		int keyslot_new,
		struct volume_key **vks,
		const struct crypt_params_reencrypt *params)
{
	int r, old_ss, new_ss;
	struct luks2_hdr *hdr;
	struct crypt_lock_handle *reencrypt_lock;
	struct luks2_reencrypt *rh;
	const struct volume_key *vk;
	struct crypt_dm_active_device dmd_target, dmd_source = {
		.uuid = crypt_get_uuid(cd),
		.flags = CRYPT_ACTIVATE_SHARED /* turn off exclusive open checks */
	};
	uint64_t minimal_size, device_size, mapping_size = 0, required_size = 0;
	bool dynamic;
	struct crypt_params_reencrypt rparams = {};
	uint32_t flags = 0;

	if (params) {
		rparams = *params;
		required_size = params->device_size;
	}

	log_dbg(cd, "Loading LUKS2 reencryption context.");

	rh = crypt_get_luks2_reencrypt(cd);
	if (rh) {
		LUKS2_reencrypt_free(cd, rh);
		crypt_set_luks2_reencrypt(cd, NULL);
		rh = NULL;
	}

	hdr = crypt_get_hdr(cd, CRYPT_LUKS2);

	r = reencrypt_lock_and_verify(cd, hdr, &reencrypt_lock);
	if (r)
		return r;

	/* From now on we hold reencryption lock */

	if (LUKS2_get_data_size(hdr, &minimal_size, &dynamic))
		return -EINVAL;

	/* some configurations provides fixed device size */
	r = LUKS2_reencrypt_check_device_size(cd, hdr, minimal_size, &device_size, false, dynamic);
	if (r) {
		r = -EINVAL;
		goto err;
	}

	minimal_size >>= SECTOR_SHIFT;

	old_ss = reencrypt_get_sector_size_old(hdr);
	new_ss = reencrypt_get_sector_size_new(hdr);

	r = reencrypt_verify_and_upload_keys(cd, hdr, LUKS2_reencrypt_digest_old(hdr), LUKS2_reencrypt_digest_new(hdr), *vks);
	if (r == -ENOENT) {
		log_dbg(cd, "Keys are not ready. Unlocking all volume keys.");
		r = LUKS2_keyslot_open_all_segments(cd, keyslot_old, keyslot_new, passphrase, passphrase_size, vks);
		if (r < 0)
			goto err;
		r = reencrypt_verify_and_upload_keys(cd, hdr, LUKS2_reencrypt_digest_old(hdr), LUKS2_reencrypt_digest_new(hdr), *vks);
	}

	if (r < 0)
		goto err;

	if (name) {
		r = dm_query_device(cd, name, DM_ACTIVE_UUID | DM_ACTIVE_DEVICE |
				    DM_ACTIVE_CRYPT_KEYSIZE | DM_ACTIVE_CRYPT_KEY |
				    DM_ACTIVE_CRYPT_CIPHER, &dmd_target);
		if (r < 0)
			goto err;
		flags = dmd_target.flags;

		/*
		 * By default reencryption code aims to retain flags from existing dm device.
		 * The keyring activation flag can not be inherited if original cipher is null.
		 *
		 * In this case override the flag based on decision made in reencrypt_verify_and_upload_keys
		 * above. The code checks if new VK is eligible for keyring.
		 */
		vk = crypt_volume_key_by_id(*vks, LUKS2_reencrypt_digest_new(hdr));
		if (vk && vk->key_description && crypt_is_cipher_null(reencrypt_segment_cipher_old(hdr))) {
			flags |= CRYPT_ACTIVATE_KEYRING_KEY;
			dmd_source.flags |= CRYPT_ACTIVATE_KEYRING_KEY;
		}

		r = LUKS2_assembly_multisegment_dmd(cd, hdr, *vks, LUKS2_get_segments_jobj(hdr), &dmd_source);
		if (!r) {
			r = crypt_compare_dm_devices(cd, &dmd_source, &dmd_target);
			if (r)
				log_err(cd, _("Mismatching parameters on device %s."), name);
		}

		dm_targets_free(cd, &dmd_source);
		dm_targets_free(cd, &dmd_target);
		free(CONST_CAST(void*)dmd_target.uuid);
		if (r)
			goto err;
		mapping_size = dmd_target.size;
	}

	r = -EINVAL;
	if (required_size && mapping_size && (required_size != mapping_size)) {
		log_err(cd, _("Active device size and requested reencryption size don't match."));
		goto err;
	}

	if (mapping_size)
		required_size = mapping_size;

	if (required_size) {
		/* TODO: Add support for changing fixed minimal size in reencryption mda where possible */
		if ((minimal_size && (required_size < minimal_size)) ||
		    (required_size > (device_size >> SECTOR_SHIFT)) ||
		    (!dynamic && (required_size != minimal_size)) ||
		    (old_ss > 0 && MISALIGNED(required_size, old_ss >> SECTOR_SHIFT)) ||
		    (new_ss > 0 && MISALIGNED(required_size, new_ss >> SECTOR_SHIFT))) {
			log_err(cd, _("Illegal device size requested in reencryption parameters."));
			goto err;
		}
		rparams.device_size = required_size;
	}

	r = reencrypt_load(cd, hdr, device_size, &rparams, &rh);
	if (r < 0 || !rh)
		goto err;

	if (name && (r = reencrypt_context_set_names(rh, name)))
		goto err;

	/* Reassure device is not mounted and there's no dm mapping active */
	if (!name && (device_open_excl(cd, crypt_data_device(cd), O_RDONLY) < 0)) {
		log_err(cd,_("Failed to open %s in exclusive mode (already mapped or mounted)."), device_path(crypt_data_device(cd)));
		r = -EBUSY;
		goto err;
	}
	device_release_excl(cd, crypt_data_device(cd));

	/* There's a race for dm device activation not managed by cryptsetup.
	 *
	 * 1) excl close
	 * 2) rogue dm device activation
	 * 3) one or more dm-crypt based wrapper activation
	 * 4) next excl open gets skipped due to 3) device from 2) remains undetected.
	 */
	r = reencrypt_init_storage_wrappers(cd, hdr, rh, *vks);
	if (r)
		goto err;

	/* If one of wrappers is based on dmcrypt fallback it already blocked mount */
	if (!name && crypt_storage_wrapper_get_type(rh->cw1) != DMCRYPT &&
	    crypt_storage_wrapper_get_type(rh->cw2) != DMCRYPT) {
		if (device_open_excl(cd, crypt_data_device(cd), O_RDONLY) < 0) {
			log_err(cd,_("Failed to open %s in exclusive mode (already mapped or mounted)."), device_path(crypt_data_device(cd)));
			r = -EBUSY;
			goto err;
		}
	}

	rh->flags = flags;

	MOVE_REF(rh->vks, *vks);
	MOVE_REF(rh->reenc_lock, reencrypt_lock);

	crypt_set_luks2_reencrypt(cd, rh);

	return 0;
err:
	LUKS2_reencrypt_unlock(cd, reencrypt_lock);
	LUKS2_reencrypt_free(cd, rh);
	return r;
}