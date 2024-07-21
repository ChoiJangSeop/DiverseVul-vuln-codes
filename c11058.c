static int legacy_parse_param(struct fs_context *fc, struct fs_parameter *param)
{
	struct legacy_fs_context *ctx = fc->fs_private;
	unsigned int size = ctx->data_size;
	size_t len = 0;
	int ret;

	ret = vfs_parse_fs_param_source(fc, param);
	if (ret != -ENOPARAM)
		return ret;

	if (ctx->param_type == LEGACY_FS_MONOLITHIC_PARAMS)
		return invalf(fc, "VFS: Legacy: Can't mix monolithic and individual options");

	switch (param->type) {
	case fs_value_is_string:
		len = 1 + param->size;
		fallthrough;
	case fs_value_is_flag:
		len += strlen(param->key);
		break;
	default:
		return invalf(fc, "VFS: Legacy: Parameter type for '%s' not supported",
			      param->key);
	}

	if (len > PAGE_SIZE - 2 - size)
		return invalf(fc, "VFS: Legacy: Cumulative options too large");
	if (strchr(param->key, ',') ||
	    (param->type == fs_value_is_string &&
	     memchr(param->string, ',', param->size)))
		return invalf(fc, "VFS: Legacy: Option '%s' contained comma",
			      param->key);
	if (!ctx->legacy_data) {
		ctx->legacy_data = kmalloc(PAGE_SIZE, GFP_KERNEL);
		if (!ctx->legacy_data)
			return -ENOMEM;
	}

	ctx->legacy_data[size++] = ',';
	len = strlen(param->key);
	memcpy(ctx->legacy_data + size, param->key, len);
	size += len;
	if (param->type == fs_value_is_string) {
		ctx->legacy_data[size++] = '=';
		memcpy(ctx->legacy_data + size, param->string, param->size);
		size += param->size;
	}
	ctx->legacy_data[size] = '\0';
	ctx->data_size = size;
	ctx->param_type = LEGACY_FS_INDIVIDUAL_PARAMS;
	return 0;
}