int LUKS2_config_get_requirements(struct crypt_device *cd, struct luks2_hdr *hdr, uint32_t *reqs)
{
	json_object *jobj_config, *jobj_requirements, *jobj_mandatory, *jobj;
	int i, len;
	uint32_t req;

	assert(hdr);
	if (!hdr || !reqs)
		return -EINVAL;

	*reqs = 0;

	if (!json_object_object_get_ex(hdr->jobj, "config", &jobj_config))
		return 0;

	if (!json_object_object_get_ex(jobj_config, "requirements", &jobj_requirements))
		return 0;

	if (!json_object_object_get_ex(jobj_requirements, "mandatory", &jobj_mandatory))
		return 0;

	len = (int) json_object_array_length(jobj_mandatory);
	if (len <= 0)
		return 0;

	log_dbg(cd, "LUKS2 requirements detected:");

	for (i = 0; i < len; i++) {
		jobj = json_object_array_get_idx(jobj_mandatory, i);
		req = get_requirement_by_name(json_object_get_string(jobj));
		log_dbg(cd, "%s - %sknown", json_object_get_string(jobj),
				        reqs_unknown(req) ? "un" : "");
		*reqs |= req;
	}

	return 0;
}