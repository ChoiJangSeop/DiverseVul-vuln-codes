int fit_image_load(bootm_headers_t *images, ulong addr,
		   const char **fit_unamep, const char **fit_uname_configp,
		   int arch, int image_type, int bootstage_id,
		   enum fit_load_op load_op, ulong *datap, ulong *lenp)
{
	int cfg_noffset, noffset;
	const char *fit_uname;
	const char *fit_uname_config;
	const char *fit_base_uname_config;
	const void *fit;
	void *buf;
	void *loadbuf;
	size_t size;
	int type_ok, os_ok;
	ulong load, load_end, data, len;
	uint8_t os, comp;
#ifndef USE_HOSTCC
	uint8_t os_arch;
#endif
	const char *prop_name;
	int ret;

	fit = map_sysmem(addr, 0);
	fit_uname = fit_unamep ? *fit_unamep : NULL;
	fit_uname_config = fit_uname_configp ? *fit_uname_configp : NULL;
	fit_base_uname_config = NULL;
	prop_name = fit_get_image_type_property(image_type);
	printf("## Loading %s from FIT Image at %08lx ...\n", prop_name, addr);

	bootstage_mark(bootstage_id + BOOTSTAGE_SUB_FORMAT);
	if (fit_check_format(fit, IMAGE_SIZE_INVAL)) {
		printf("Bad FIT %s image format!\n", prop_name);
		bootstage_error(bootstage_id + BOOTSTAGE_SUB_FORMAT);
		return -ENOEXEC;
	}
	bootstage_mark(bootstage_id + BOOTSTAGE_SUB_FORMAT_OK);
	if (fit_uname) {
		/* get FIT component image node offset */
		bootstage_mark(bootstage_id + BOOTSTAGE_SUB_UNIT_NAME);
		noffset = fit_image_get_node(fit, fit_uname);
	} else {
		/*
		 * no image node unit name, try to get config
		 * node first. If config unit node name is NULL
		 * fit_conf_get_node() will try to find default config node
		 */
		bootstage_mark(bootstage_id + BOOTSTAGE_SUB_NO_UNIT_NAME);
		if (IMAGE_ENABLE_BEST_MATCH && !fit_uname_config) {
			cfg_noffset = fit_conf_find_compat(fit, gd_fdt_blob());
		} else {
			cfg_noffset = fit_conf_get_node(fit,
							fit_uname_config);
		}
		if (cfg_noffset < 0) {
			puts("Could not find configuration node\n");
			bootstage_error(bootstage_id +
					BOOTSTAGE_SUB_NO_UNIT_NAME);
			return -ENOENT;
		}

		fit_base_uname_config = fdt_get_name(fit, cfg_noffset, NULL);
		printf("   Using '%s' configuration\n", fit_base_uname_config);
		/* Remember this config */
		if (image_type == IH_TYPE_KERNEL)
			images->fit_uname_cfg = fit_base_uname_config;

		if (FIT_IMAGE_ENABLE_VERIFY && images->verify) {
			puts("   Verifying Hash Integrity ... ");
			if (fit_config_verify(fit, cfg_noffset)) {
				puts("Bad Data Hash\n");
				bootstage_error(bootstage_id +
					BOOTSTAGE_SUB_HASH);
				return -EACCES;
			}
			puts("OK\n");
		}

		bootstage_mark(BOOTSTAGE_ID_FIT_CONFIG);

		noffset = fit_conf_get_prop_node(fit, cfg_noffset,
						 prop_name);
		fit_uname = fit_get_name(fit, noffset, NULL);
	}
	if (noffset < 0) {
		printf("Could not find subimage node type '%s'\n", prop_name);
		bootstage_error(bootstage_id + BOOTSTAGE_SUB_SUBNODE);
		return -ENOENT;
	}

	printf("   Trying '%s' %s subimage\n", fit_uname, prop_name);

	ret = fit_image_select(fit, noffset, images->verify);
	if (ret) {
		bootstage_error(bootstage_id + BOOTSTAGE_SUB_HASH);
		return ret;
	}

	bootstage_mark(bootstage_id + BOOTSTAGE_SUB_CHECK_ARCH);
	if (!host_build() && IS_ENABLED(CONFIG_SANDBOX)) {
		if (!fit_image_check_target_arch(fit, noffset)) {
			puts("Unsupported Architecture\n");
			bootstage_error(bootstage_id + BOOTSTAGE_SUB_CHECK_ARCH);
			return -ENOEXEC;
		}
	}

#ifndef USE_HOSTCC
	fit_image_get_arch(fit, noffset, &os_arch);
	images->os.arch = os_arch;
#endif

	bootstage_mark(bootstage_id + BOOTSTAGE_SUB_CHECK_ALL);
	type_ok = fit_image_check_type(fit, noffset, image_type) ||
		  fit_image_check_type(fit, noffset, IH_TYPE_FIRMWARE) ||
		  (image_type == IH_TYPE_KERNEL &&
		   fit_image_check_type(fit, noffset, IH_TYPE_KERNEL_NOLOAD));

	os_ok = image_type == IH_TYPE_FLATDT ||
		image_type == IH_TYPE_FPGA ||
		fit_image_check_os(fit, noffset, IH_OS_LINUX) ||
		fit_image_check_os(fit, noffset, IH_OS_U_BOOT) ||
		fit_image_check_os(fit, noffset, IH_OS_OPENRTOS) ||
		fit_image_check_os(fit, noffset, IH_OS_EFI) ||
		fit_image_check_os(fit, noffset, IH_OS_VXWORKS);

	/*
	 * If either of the checks fail, we should report an error, but
	 * if the image type is coming from the "loadables" field, we
	 * don't care what it is
	 */
	if ((!type_ok || !os_ok) && image_type != IH_TYPE_LOADABLE) {
		fit_image_get_os(fit, noffset, &os);
		printf("No %s %s %s Image\n",
		       genimg_get_os_name(os),
		       genimg_get_arch_name(arch),
		       genimg_get_type_name(image_type));
		bootstage_error(bootstage_id + BOOTSTAGE_SUB_CHECK_ALL);
		return -EIO;
	}

	bootstage_mark(bootstage_id + BOOTSTAGE_SUB_CHECK_ALL_OK);

	/* get image data address and length */
	if (fit_image_get_data_and_size(fit, noffset,
					(const void **)&buf, &size)) {
		printf("Could not find %s subimage data!\n", prop_name);
		bootstage_error(bootstage_id + BOOTSTAGE_SUB_GET_DATA);
		return -ENOENT;
	}

	/* Decrypt data before uncompress/move */
	if (IS_ENABLED(CONFIG_FIT_CIPHER) && IMAGE_ENABLE_DECRYPT) {
		puts("   Decrypting Data ... ");
		if (fit_image_uncipher(fit, noffset, &buf, &size)) {
			puts("Error\n");
			return -EACCES;
		}
		puts("OK\n");
	}

	/* perform any post-processing on the image data */
	if (!host_build() && IS_ENABLED(CONFIG_FIT_IMAGE_POST_PROCESS))
		board_fit_image_post_process(&buf, &size);

	len = (ulong)size;

	bootstage_mark(bootstage_id + BOOTSTAGE_SUB_GET_DATA_OK);

	data = map_to_sysmem(buf);
	load = data;
	if (load_op == FIT_LOAD_IGNORED) {
		/* Don't load */
	} else if (fit_image_get_load(fit, noffset, &load)) {
		if (load_op == FIT_LOAD_REQUIRED) {
			printf("Can't get %s subimage load address!\n",
			       prop_name);
			bootstage_error(bootstage_id + BOOTSTAGE_SUB_LOAD);
			return -EBADF;
		}
	} else if (load_op != FIT_LOAD_OPTIONAL_NON_ZERO || load) {
		ulong image_start, image_end;

		/*
		 * move image data to the load address,
		 * make sure we don't overwrite initial image
		 */
		image_start = addr;
		image_end = addr + fit_get_size(fit);

		load_end = load + len;
		if (image_type != IH_TYPE_KERNEL &&
		    load < image_end && load_end > image_start) {
			printf("Error: %s overwritten\n", prop_name);
			return -EXDEV;
		}

		printf("   Loading %s from 0x%08lx to 0x%08lx\n",
		       prop_name, data, load);
	} else {
		load = data;	/* No load address specified */
	}

	comp = IH_COMP_NONE;
	loadbuf = buf;
	/* Kernel images get decompressed later in bootm_load_os(). */
	if (!fit_image_get_comp(fit, noffset, &comp) &&
	    comp != IH_COMP_NONE &&
	    !(image_type == IH_TYPE_KERNEL ||
	      image_type == IH_TYPE_KERNEL_NOLOAD ||
	      image_type == IH_TYPE_RAMDISK)) {
		ulong max_decomp_len = len * 20;
		if (load == data) {
			loadbuf = malloc(max_decomp_len);
			load = map_to_sysmem(loadbuf);
		} else {
			loadbuf = map_sysmem(load, max_decomp_len);
		}
		if (image_decomp(comp, load, data, image_type,
				loadbuf, buf, len, max_decomp_len, &load_end)) {
			printf("Error decompressing %s\n", prop_name);

			return -ENOEXEC;
		}
		len = load_end - load;
	} else if (load != data) {
		loadbuf = map_sysmem(load, len);
		memcpy(loadbuf, buf, len);
	}

	if (image_type == IH_TYPE_RAMDISK && comp != IH_COMP_NONE)
		puts("WARNING: 'compression' nodes for ramdisks are deprecated,"
		     " please fix your .its file!\n");

	/* verify that image data is a proper FDT blob */
	if (image_type == IH_TYPE_FLATDT && fdt_check_header(loadbuf)) {
		puts("Subimage data is not a FDT");
		return -ENOEXEC;
	}

	bootstage_mark(bootstage_id + BOOTSTAGE_SUB_LOAD);

	*datap = load;
	*lenp = len;
	if (fit_unamep)
		*fit_unamep = (char *)fit_uname;
	if (fit_uname_configp)
		*fit_uname_configp = (char *)(fit_uname_config ? :
					      fit_base_uname_config);

	return noffset;
}