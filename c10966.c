static int reencrypt_init(struct crypt_device *cd,
		const char *name,
		struct luks2_hdr *hdr,
		const char *passphrase,
		size_t passphrase_size,
		int keyslot_old,
		int keyslot_new,
		const char *cipher,
		const char *cipher_mode,
		const struct crypt_params_reencrypt *params,
		struct volume_key **vks)
{
	bool move_first_segment;
	char _cipher[128];
	uint32_t sector_size;
	int r, reencrypt_keyslot, devfd = -1;
	uint64_t data_offset, dev_size = 0;
	struct crypt_dm_active_device dmd_target, dmd_source = {
		.uuid = crypt_get_uuid(cd),
		.flags = CRYPT_ACTIVATE_SHARED /* turn off exclusive open checks */
	};

	if (!params || params->mode > CRYPT_REENCRYPT_DECRYPT)
		return -EINVAL;

	if (params->mode != CRYPT_REENCRYPT_DECRYPT &&
	    (!params->luks2 || !(cipher && cipher_mode) || keyslot_new < 0))
		return -EINVAL;

	log_dbg(cd, "Initializing reencryption (mode: %s) in LUKS2 metadata.",
		    crypt_reencrypt_mode_to_str(params->mode));

	move_first_segment = (params->flags & CRYPT_REENCRYPT_MOVE_FIRST_SEGMENT);

	/* implicit sector size 512 for decryption */
	sector_size = params->luks2 ? params->luks2->sector_size : SECTOR_SIZE;
	if (sector_size < SECTOR_SIZE || sector_size > MAX_SECTOR_SIZE ||
	    NOTPOW2(sector_size)) {
		log_err(cd, _("Unsupported encryption sector size."));
		return -EINVAL;
	}

	if (!cipher_mode || *cipher_mode == '\0')
		r = snprintf(_cipher, sizeof(_cipher), "%s", cipher);
	else
		r = snprintf(_cipher, sizeof(_cipher), "%s-%s", cipher, cipher_mode);
	if (r < 0 || (size_t)r >= sizeof(_cipher))
		return -EINVAL;

	if (MISALIGNED(params->data_shift, sector_size >> SECTOR_SHIFT)) {
		log_err(cd, _("Data shift is not aligned to requested encryption sector size (%" PRIu32 " bytes)."), sector_size);
		return -EINVAL;
	}

	data_offset = LUKS2_get_data_offset(hdr) << SECTOR_SHIFT;

	r = device_check_access(cd, crypt_data_device(cd), DEV_OK);
	if (r)
		return r;

	r = device_check_size(cd, crypt_data_device(cd), data_offset, 1);
	if (r)
		return r;

	r = device_size(crypt_data_device(cd), &dev_size);
	if (r)
		return r;

	dev_size -= data_offset;

	if (MISALIGNED(dev_size, sector_size)) {
		log_err(cd, _("Data device is not aligned to requested encryption sector size (%" PRIu32 " bytes)."), sector_size);
		return -EINVAL;
	}

	reencrypt_keyslot = LUKS2_keyslot_find_empty(cd, hdr, 0);
	if (reencrypt_keyslot < 0) {
		log_err(cd, _("All key slots full."));
		return -EINVAL;
	}

	/*
	 * We must perform data move with exclusive open data device
	 * to exclude another cryptsetup process to colide with
	 * encryption initialization (or mount)
	 */
	if (move_first_segment) {
		if (dev_size < (params->data_shift << SECTOR_SHIFT)) {
			log_err(cd, _("Device %s is too small."), device_path(crypt_data_device(cd)));
			return -EINVAL;
		}
		if (params->data_shift < LUKS2_get_data_offset(hdr)) {
			log_err(cd, _("Data shift (%" PRIu64 " sectors) is less than future data offset (%" PRIu64 " sectors)."), params->data_shift, LUKS2_get_data_offset(hdr));
			return -EINVAL;
		}
		devfd = device_open_excl(cd, crypt_data_device(cd), O_RDWR);
		if (devfd < 0) {
			if (devfd == -EBUSY)
				log_err(cd,_("Failed to open %s in exclusive mode (already mapped or mounted)."), device_path(crypt_data_device(cd)));
			return -EINVAL;
		}
	}

	if (params->mode == CRYPT_REENCRYPT_ENCRYPT) {
		/* in-memory only */
		r = reencrypt_set_encrypt_segments(cd, hdr, dev_size, params->data_shift << SECTOR_SHIFT, move_first_segment, params->direction);
		if (r)
			goto out;
	}

	r = LUKS2_keyslot_reencrypt_create(cd, hdr, reencrypt_keyslot,
					   params);
	if (r < 0)
		goto out;

	r = reencrypt_make_backup_segments(cd, hdr, keyslot_new, _cipher, data_offset, params);
	if (r) {
		log_dbg(cd, "Failed to create reencryption backup device segments.");
		goto out;
	}

	r = LUKS2_keyslot_open_all_segments(cd, keyslot_old, keyslot_new, passphrase, passphrase_size, vks);
	if (r < 0)
		goto out;

	if (name && params->mode != CRYPT_REENCRYPT_ENCRYPT) {
		r = reencrypt_verify_and_upload_keys(cd, hdr, LUKS2_reencrypt_digest_old(hdr), LUKS2_reencrypt_digest_new(hdr), *vks);
		if (r)
			goto out;

		r = dm_query_device(cd, name, DM_ACTIVE_UUID | DM_ACTIVE_DEVICE |
				    DM_ACTIVE_CRYPT_KEYSIZE | DM_ACTIVE_CRYPT_KEY |
				    DM_ACTIVE_CRYPT_CIPHER, &dmd_target);
		if (r < 0)
			goto out;

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
			goto out;
	}

	if (move_first_segment && reencrypt_move_data(cd, devfd, params->data_shift << SECTOR_SHIFT)) {
		r = -EIO;
		goto out;
	}

	/* This must be first and only write in LUKS2 metadata during _reencrypt_init */
	r = reencrypt_update_flag(cd, 1, true);
	if (r) {
		log_dbg(cd, "Failed to set online-reencryption requirement.");
		r = -EINVAL;
	} else
		r = reencrypt_keyslot;
out:
	device_release_excl(cd, crypt_data_device(cd));
	if (r < 0)
		crypt_load(cd, CRYPT_LUKS2, NULL);

	return r;
}