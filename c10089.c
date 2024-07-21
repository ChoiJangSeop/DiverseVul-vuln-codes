int intel_uc_fw_fetch(struct intel_uc_fw *uc_fw)
{
	struct drm_i915_private *i915 = __uc_fw_to_gt(uc_fw)->i915;
	struct device *dev = i915->drm.dev;
	struct drm_i915_gem_object *obj;
	const struct firmware *fw = NULL;
	struct uc_css_header *css;
	size_t size;
	int err;

	GEM_BUG_ON(!i915->wopcm.size);
	GEM_BUG_ON(!intel_uc_fw_is_enabled(uc_fw));

	err = i915_inject_probe_error(i915, -ENXIO);
	if (err)
		goto fail;

	__force_fw_fetch_failures(uc_fw, -EINVAL);
	__force_fw_fetch_failures(uc_fw, -ESTALE);

	err = request_firmware(&fw, uc_fw->path, dev);
	if (err)
		goto fail;

	/* Check the size of the blob before examining buffer contents */
	if (unlikely(fw->size < sizeof(struct uc_css_header))) {
		drm_warn(&i915->drm, "%s firmware %s: invalid size: %zu < %zu\n",
			 intel_uc_fw_type_repr(uc_fw->type), uc_fw->path,
			 fw->size, sizeof(struct uc_css_header));
		err = -ENODATA;
		goto fail;
	}

	css = (struct uc_css_header *)fw->data;

	/* Check integrity of size values inside CSS header */
	size = (css->header_size_dw - css->key_size_dw - css->modulus_size_dw -
		css->exponent_size_dw) * sizeof(u32);
	if (unlikely(size != sizeof(struct uc_css_header))) {
		drm_warn(&i915->drm,
			 "%s firmware %s: unexpected header size: %zu != %zu\n",
			 intel_uc_fw_type_repr(uc_fw->type), uc_fw->path,
			 fw->size, sizeof(struct uc_css_header));
		err = -EPROTO;
		goto fail;
	}

	/* uCode size must calculated from other sizes */
	uc_fw->ucode_size = (css->size_dw - css->header_size_dw) * sizeof(u32);

	/* now RSA */
	if (unlikely(css->key_size_dw != UOS_RSA_SCRATCH_COUNT)) {
		drm_warn(&i915->drm, "%s firmware %s: unexpected key size: %u != %u\n",
			 intel_uc_fw_type_repr(uc_fw->type), uc_fw->path,
			 css->key_size_dw, UOS_RSA_SCRATCH_COUNT);
		err = -EPROTO;
		goto fail;
	}
	uc_fw->rsa_size = css->key_size_dw * sizeof(u32);

	/* At least, it should have header, uCode and RSA. Size of all three. */
	size = sizeof(struct uc_css_header) + uc_fw->ucode_size + uc_fw->rsa_size;
	if (unlikely(fw->size < size)) {
		drm_warn(&i915->drm, "%s firmware %s: invalid size: %zu < %zu\n",
			 intel_uc_fw_type_repr(uc_fw->type), uc_fw->path,
			 fw->size, size);
		err = -ENOEXEC;
		goto fail;
	}

	/* Sanity check whether this fw is not larger than whole WOPCM memory */
	size = __intel_uc_fw_get_upload_size(uc_fw);
	if (unlikely(size >= i915->wopcm.size)) {
		drm_warn(&i915->drm, "%s firmware %s: invalid size: %zu > %zu\n",
			 intel_uc_fw_type_repr(uc_fw->type), uc_fw->path,
			 size, (size_t)i915->wopcm.size);
		err = -E2BIG;
		goto fail;
	}

	/* Get version numbers from the CSS header */
	uc_fw->major_ver_found = FIELD_GET(CSS_SW_VERSION_UC_MAJOR,
					   css->sw_version);
	uc_fw->minor_ver_found = FIELD_GET(CSS_SW_VERSION_UC_MINOR,
					   css->sw_version);

	if (uc_fw->major_ver_found != uc_fw->major_ver_wanted ||
	    uc_fw->minor_ver_found < uc_fw->minor_ver_wanted) {
		drm_notice(&i915->drm, "%s firmware %s: unexpected version: %u.%u != %u.%u\n",
			   intel_uc_fw_type_repr(uc_fw->type), uc_fw->path,
			   uc_fw->major_ver_found, uc_fw->minor_ver_found,
			   uc_fw->major_ver_wanted, uc_fw->minor_ver_wanted);
		if (!intel_uc_fw_is_overridden(uc_fw)) {
			err = -ENOEXEC;
			goto fail;
		}
	}

	obj = i915_gem_object_create_shmem_from_data(i915, fw->data, fw->size);
	if (IS_ERR(obj)) {
		err = PTR_ERR(obj);
		goto fail;
	}

	uc_fw->obj = obj;
	uc_fw->size = fw->size;
	intel_uc_fw_change_status(uc_fw, INTEL_UC_FIRMWARE_AVAILABLE);

	release_firmware(fw);
	return 0;

fail:
	intel_uc_fw_change_status(uc_fw, err == -ENOENT ?
				  INTEL_UC_FIRMWARE_MISSING :
				  INTEL_UC_FIRMWARE_ERROR);

	drm_notice(&i915->drm, "%s firmware %s: fetch failed with error %d\n",
		   intel_uc_fw_type_repr(uc_fw->type), uc_fw->path, err);
	drm_info(&i915->drm, "%s firmware(s) can be downloaded from %s\n",
		 intel_uc_fw_type_repr(uc_fw->type), INTEL_UC_FIRMWARE_URL);

	release_firmware(fw);		/* OK even if fw is NULL */
	return err;
}