oauth2_jwt_body_process(const struct oauth2_settings *set, const char *alg,
			const char *kid, ARRAY_TYPE(oauth2_field) *fields,
			struct json_tree *tree, const char *const *blobs,
			const char **error_r)
{
	const char *sub = get_field(tree, "sub");

	int ret;
	int64_t t0 = time(NULL);
	/* default IAT and NBF to now */
	int64_t iat, nbf, exp;
	int tz_offset ATTR_UNUSED;

	if (sub == NULL) {
		*error_r = "Missing 'sub' field";
		return -1;
	}

	if ((ret = get_time_field(tree, "exp", &exp)) < 1) {
		*error_r = t_strdup_printf("%s 'exp' field",
				ret == 0 ? "Missing" : "Malformed");
		return -1;
	}

	if ((ret = get_time_field(tree, "nbf", &nbf)) < 0) {
		*error_r = "Malformed 'nbf' field";
		return -1;
	} else if (ret == 0 || nbf == 0)
		nbf = t0;

	if ((ret = get_time_field(tree, "iat", &iat)) < 0) {
		*error_r = "Malformed 'iat' field";
		return -1;
	} else if (ret == 0 || iat == 0)
		iat = t0;

	if (nbf > t0) {
		*error_r = "Token is not valid yet";
		return -1;
	}
	if (iat > t0) {
		*error_r = "Token is issued in future";
		return -1;
	}
	if (exp < t0) {
		*error_r = "Token has expired";
		return -1;
	}

	/* ensure token dates are not conflicting */
	if (nbf < iat ||
	    exp < iat ||
	    exp < nbf) {
		*error_r = "Token time values are conflicting";
		return -1;
	}

	const char *iss = get_field(tree, "iss");
	if (set->issuers != NULL && *set->issuers != NULL) {
		if (iss == NULL) {
			*error_r = "Token is missing 'iss' field";
			return -1;
		}
		if (!str_array_find(set->issuers, iss)) {
			*error_r = t_strdup_printf("Issuer '%s' is not allowed",
						   str_sanitize_utf8(iss, 128));
			return -1;
		}
	}

	/* see if there is azp */
	const char *azp = get_field(tree, "azp");
	if (azp == NULL)
		azp = "default";

	if (oauth2_validate_signature(set, azp, alg, kid, blobs, error_r) < 0)
		return -1;

	oauth2_jwt_copy_fields(fields, tree);
	return 0;
}