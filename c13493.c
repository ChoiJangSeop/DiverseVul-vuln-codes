int fit_check_format(const void *fit, ulong size)
{
	int ret;

	/* A FIT image must be a valid FDT */
	ret = fdt_check_header(fit);
	if (ret) {
		log_debug("Wrong FIT format: not a flattened device tree (err=%d)\n",
			  ret);
		return -ENOEXEC;
	}

	if (CONFIG_IS_ENABLED(FIT_FULL_CHECK)) {
		/*
		 * If we are not given the size, make do wtih calculating it.
		 * This is not as secure, so we should consider a flag to
		 * control this.
		 */
		if (size == IMAGE_SIZE_INVAL)
			size = fdt_totalsize(fit);
		ret = fdt_check_full(fit, size);

		if (ret) {
			log_debug("FIT check error %d\n", ret);
			return -EINVAL;
		}
	}

	/* mandatory / node 'description' property */
	if (!fdt_getprop(fit, 0, FIT_DESC_PROP, NULL)) {
		log_debug("Wrong FIT format: no description\n");
		return -ENOMSG;
	}

	if (IMAGE_ENABLE_TIMESTAMP) {
		/* mandatory / node 'timestamp' property */
		if (!fdt_getprop(fit, 0, FIT_TIMESTAMP_PROP, NULL)) {
			log_debug("Wrong FIT format: no timestamp\n");
			return -ENODATA;
		}
	}

	/* mandatory subimages parent '/images' node */
	if (fdt_path_offset(fit, FIT_IMAGES_PATH) < 0) {
		log_debug("Wrong FIT format: no images parent node\n");
		return -ENOENT;
	}

	return 0;
}