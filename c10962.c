int LUKS2_keyslot_area(struct luks2_hdr *hdr,
	int keyslot,
	uint64_t *offset,
	uint64_t *length)
{
	json_object *jobj_keyslot, *jobj_area, *jobj;

	if(LUKS2_keyslot_info(hdr, keyslot) == CRYPT_SLOT_INVALID)
		return -EINVAL;

	jobj_keyslot = LUKS2_get_keyslot_jobj(hdr, keyslot);
	if (!jobj_keyslot)
		return -ENOENT;

	if (!json_object_object_get_ex(jobj_keyslot, "area", &jobj_area))
		return -EINVAL;

	if (!json_object_object_get_ex(jobj_area, "offset", &jobj))
		return -EINVAL;
	*offset = crypt_jobj_get_uint64(jobj);

	if (!json_object_object_get_ex(jobj_area, "size", &jobj))
		return -EINVAL;
	*length = crypt_jobj_get_uint64(jobj);

	return 0;
}