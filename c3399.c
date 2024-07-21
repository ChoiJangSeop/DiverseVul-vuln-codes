void cgit_print_blob(const char *hex, char *path, const char *head, int file_only)
{
	unsigned char sha1[20];
	enum object_type type;
	char *buf;
	unsigned long size;
	struct commit *commit;
	struct pathspec_item path_items = {
		.match = path,
		.len = path ? strlen(path) : 0
	};
	struct pathspec paths = {
		.nr = 1,
		.items = &path_items
	};
	struct walk_tree_context walk_tree_ctx = {
		.match_path = path,
		.matched_sha1 = sha1,
		.found_path = 0,
		.file_only = file_only
	};

	if (hex) {
		if (get_sha1_hex(hex, sha1)) {
			cgit_print_error_page(400, "Bad request",
					"Bad hex value: %s", hex);
			return;
		}
	} else {
		if (get_sha1(head, sha1)) {
			cgit_print_error_page(404, "Not found",
					"Bad ref: %s", head);
			return;
		}
	}

	type = sha1_object_info(sha1, &size);

	if ((!hex) && type == OBJ_COMMIT && path) {
		commit = lookup_commit_reference(sha1);
		read_tree_recursive(commit->tree, "", 0, 0, &paths, walk_tree, &walk_tree_ctx);
		type = sha1_object_info(sha1,&size);
	}

	if (type == OBJ_BAD) {
		cgit_print_error_page(404, "Not found",
				"Bad object name: %s", hex);
		return;
	}

	buf = read_sha1_file(sha1, &type, &size);
	if (!buf) {
		cgit_print_error_page(500, "Internal server error",
				"Error reading object %s", hex);
		return;
	}

	buf[size] = '\0';
	ctx.page.mimetype = ctx.qry.mimetype;
	if (!ctx.page.mimetype) {
		if (buffer_is_binary(buf, size))
			ctx.page.mimetype = "application/octet-stream";
		else
			ctx.page.mimetype = "text/plain";
	}
	ctx.page.filename = path;
	cgit_print_http_headers();
	html_raw(buf, size);
	free(buf);
}