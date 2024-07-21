static int reencrypt_keyslot_update(struct crypt_device *cd,
	const struct luks2_reencrypt *rh)
{
	json_object *jobj_keyslot, *jobj_area, *jobj_area_type;
	struct luks2_hdr *hdr;

	if (!(hdr = crypt_get_hdr(cd, CRYPT_LUKS2)))
		return -EINVAL;

	jobj_keyslot = LUKS2_get_keyslot_jobj(hdr, rh->reenc_keyslot);
	if (!jobj_keyslot)
		return -EINVAL;

	json_object_object_get_ex(jobj_keyslot, "area", &jobj_area);
	json_object_object_get_ex(jobj_area, "type", &jobj_area_type);

	if (rh->rp.type == REENC_PROTECTION_CHECKSUM) {
		log_dbg(cd, "Updating reencrypt keyslot for checksum protection.");
		json_object_object_add(jobj_area, "type", json_object_new_string("checksum"));
		json_object_object_add(jobj_area, "hash", json_object_new_string(rh->rp.p.csum.hash));
		json_object_object_add(jobj_area, "sector_size", json_object_new_int64(rh->alignment));
	} else if (rh->rp.type == REENC_PROTECTION_NONE) {
		log_dbg(cd, "Updating reencrypt keyslot for none protection.");
		json_object_object_add(jobj_area, "type", json_object_new_string("none"));
		json_object_object_del(jobj_area, "hash");
	} else if (rh->rp.type == REENC_PROTECTION_JOURNAL) {
		log_dbg(cd, "Updating reencrypt keyslot for journal protection.");
		json_object_object_add(jobj_area, "type", json_object_new_string("journal"));
		json_object_object_del(jobj_area, "hash");
	} else
		log_dbg(cd, "No update of reencrypt keyslot needed.");

	return 0;
}