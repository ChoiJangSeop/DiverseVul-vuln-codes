static reenc_status_t reencrypt_step(struct crypt_device *cd,
		struct luks2_hdr *hdr,
		struct luks2_reencrypt *rh,
		uint64_t device_size,
		bool online)
{
	int r;

	/* update reencrypt keyslot protection parameters in memory only */
	r = reencrypt_keyslot_update(cd, rh);
	if (r < 0) {
		log_dbg(cd, "Keyslot update failed.");
		return REENC_ERR;
	}

	/* in memory only */
	r = reencrypt_make_segments(cd, hdr, rh, device_size);
	if (r)
		return REENC_ERR;

	r = reencrypt_assign_segments(cd, hdr, rh, 1, 0);
	if (r) {
		log_err(cd, _("Failed to set device segments for next reencryption hotzone."));
		return REENC_ERR;
	}

	if (online) {
		r = reencrypt_refresh_overlay_devices(cd, hdr, rh->overlay_name, rh->hotzone_name, rh->vks, rh->device_size, rh->flags);
		/* Teardown overlay devices with dm-error. None bio shall pass! */
		if (r != REENC_OK)
			return r;
	}

	log_dbg(cd, "Reencrypting chunk starting at offset: %" PRIu64 ", size :%" PRIu64 ".", rh->offset, rh->length);
	log_dbg(cd, "data_offset: %" PRIu64, crypt_get_data_offset(cd) << SECTOR_SHIFT);

	if (!rh->offset && rh->mode == CRYPT_REENCRYPT_ENCRYPT && rh->data_shift &&
	    rh->jobj_segment_moved) {
		crypt_storage_wrapper_destroy(rh->cw1);
		log_dbg(cd, "Reinitializing old segment storage wrapper for moved segment.");
		r = crypt_storage_wrapper_init(cd, &rh->cw1, crypt_data_device(cd),
				LUKS2_reencrypt_get_data_offset_moved(hdr),
				crypt_get_iv_offset(cd),
				reencrypt_get_sector_size_old(hdr),
				reencrypt_segment_cipher_old(hdr),
				crypt_volume_key_by_id(rh->vks, rh->digest_old),
				rh->wflags1);
		if (r) {
			log_err(cd, _("Failed to initialize old segment storage wrapper."));
			return REENC_ROLLBACK;
		}
	}

	rh->read = crypt_storage_wrapper_read(rh->cw1, rh->offset, rh->reenc_buffer, rh->length);
	if (rh->read < 0) {
		/* severity normal */
		log_err(cd, _("Failed to read hotzone area starting at %" PRIu64 "."), rh->offset);
		return REENC_ROLLBACK;
	}

	/* metadata commit point */
	r = reencrypt_hotzone_protect_final(cd, hdr, rh, rh->reenc_buffer, rh->read);
	if (r < 0) {
		/* severity normal */
		log_err(cd, _("Failed to write reencryption resilience metadata."));
		return REENC_ROLLBACK;
	}

	r = crypt_storage_wrapper_decrypt(rh->cw1, rh->offset, rh->reenc_buffer, rh->read);
	if (r) {
		/* severity normal */
		log_err(cd, _("Decryption failed."));
		return REENC_ROLLBACK;
	}
	if (rh->read != crypt_storage_wrapper_encrypt_write(rh->cw2, rh->offset, rh->reenc_buffer, rh->read)) {
		/* severity fatal */
		log_err(cd, _("Failed to write hotzone area starting at %" PRIu64 "."), rh->offset);
		return REENC_FATAL;
	}

	if (rh->rp.type != REENC_PROTECTION_NONE && crypt_storage_wrapper_datasync(rh->cw2)) {
		log_err(cd, _("Failed to sync data."));
		return REENC_FATAL;
	}

	/* metadata commit safe point */
	r = reencrypt_assign_segments(cd, hdr, rh, 0, rh->rp.type != REENC_PROTECTION_NONE);
	if (r) {
		/* severity fatal */
		log_err(cd, _("Failed to update metadata after current reencryption hotzone completed."));
		return REENC_FATAL;
	}

	if (online) {
		/* severity normal */
		log_dbg(cd, "Resuming device %s", rh->hotzone_name);
		r = dm_resume_device(cd, rh->hotzone_name, DM_RESUME_PRIVATE);
		if (r) {
			log_err(cd, _("Failed to resume device %s."), rh->hotzone_name);
			return REENC_ERR;
		}
	}

	return REENC_OK;
}