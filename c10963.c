int LUKS2_config_set_requirements(struct crypt_device *cd, struct luks2_hdr *hdr, uint32_t reqs, bool commit)
{
	json_object *jobj_config, *jobj_requirements, *jobj_mandatory, *jobj;
	int i, r = -EINVAL;

	if (!hdr)
		return -EINVAL;

	jobj_mandatory = json_object_new_array();
	if (!jobj_mandatory)
		return -ENOMEM;

	for (i = 0; requirements_flags[i].description; i++) {
		if (reqs & requirements_flags[i].flag) {
			jobj = json_object_new_string(requirements_flags[i].description);
			if (!jobj) {
				r = -ENOMEM;
				goto err;
			}
			json_object_array_add(jobj_mandatory, jobj);
			/* erase processed flag from input set */
			reqs &= ~(requirements_flags[i].flag);
		}
	}

	/* any remaining bit in requirements is unknown therefore illegal */
	if (reqs) {
		log_dbg(cd, "Illegal requirement flag(s) requested");
		goto err;
	}

	if (!json_object_object_get_ex(hdr->jobj, "config", &jobj_config))
		goto err;

	if (!json_object_object_get_ex(jobj_config, "requirements", &jobj_requirements)) {
		jobj_requirements = json_object_new_object();
		if (!jobj_requirements) {
			r = -ENOMEM;
			goto err;
		}
		json_object_object_add(jobj_config, "requirements", jobj_requirements);
	}

	if (json_object_array_length(jobj_mandatory) > 0) {
		/* replace mandatory field with new values */
		json_object_object_add(jobj_requirements, "mandatory", jobj_mandatory);
	} else {
		/* new mandatory field was empty, delete old one */
		json_object_object_del(jobj_requirements, "mandatory");
		json_object_put(jobj_mandatory);
	}

	/* remove empty requirements object */
	if (!json_object_object_length(jobj_requirements))
		json_object_object_del(jobj_config, "requirements");

	return commit ? LUKS2_hdr_write(cd, hdr) : 0;
err:
	json_object_put(jobj_mandatory);
	return r;
}