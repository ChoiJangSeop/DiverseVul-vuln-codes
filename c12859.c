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