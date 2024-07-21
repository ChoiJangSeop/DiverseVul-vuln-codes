int oauth2_try_parse_jwt(const struct oauth2_settings *set,
			 const char *token, ARRAY_TYPE(oauth2_field) *fields,
			 bool *is_jwt_r, const char **error_r)
{
	const char *const *blobs = t_strsplit(token, ".");
	int ret;

	i_assert(set->key_dict != NULL);

	/* we don't know if it's JWT token yet */
	*is_jwt_r = FALSE;

	if (str_array_length(blobs) != 3) {
		*error_r = "Not a JWT token";
		return -1;
	}

	/* attempt to decode header */
	buffer_t *header =
		t_base64url_decode_str(BASE64_DECODE_FLAG_NO_PADDING, blobs[0]);

	if (header->used == 0) {
		*error_r = "Not a JWT token";
		return -1;
	}

	struct json_tree *header_tree;
	if (oauth2_json_tree_build(header, &header_tree, error_r) < 0)
		return -1;

	const char *alg, *kid;
	ret = oauth2_jwt_header_process(header_tree, &alg, &kid, error_r);
	json_tree_deinit(&header_tree);
	if (ret < 0)
		return -1;

	/* it is now assumed to be a JWT token */
	*is_jwt_r = TRUE;

	if (kid == NULL)
		kid = "default";
	else if (*kid == '\0') {
		*error_r = "'kid' field is empty";
		return -1;
	}

	size_t pos = strcspn(kid, "./%");
	if (pos < strlen(kid)) {
		/* sanitize kid, cannot allow dots or / in it, so we encode them
		 */
		string_t *new_kid = t_str_new(strlen(kid));
		/* put initial data */
		str_append_data(new_kid, kid, pos);
		for (const char *c = kid+pos; *c != '\0'; c++) {
			switch (*c) {
			case '.':
				str_append(new_kid, "%2e");
				break;
			case '/':
				str_append(new_kid, "%2f");
				break;
			case '%':
				str_append(new_kid, "%25");
				break;
			default:
				str_append_c(new_kid, *c);
				break;
			}
		}
		kid = str_c(new_kid);
	}

	/* parse body */
	struct json_tree *body_tree;
	buffer_t *body =
		t_base64url_decode_str(BASE64_DECODE_FLAG_NO_PADDING, blobs[1]);
	if (oauth2_json_tree_build(body, &body_tree, error_r) == -1)
		return -1;
	ret = oauth2_jwt_body_process(set, alg, kid, fields, body_tree, blobs,
				      error_r);
	json_tree_deinit(&body_tree);

	return ret;
}