PHP_FUNCTION(exif_read_data)
{
	char *p_name, *p_sections_needed = NULL;
	int p_name_len, p_sections_needed_len = 0;
	zend_bool sub_arrays=0, read_thumbnail=0, read_all=0;

	int i, ret, sections_needed=0;
	image_info_type ImageInfo;
	char tmp[64], *sections_str, *s;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "p|sbb", &p_name, &p_name_len, &p_sections_needed, &p_sections_needed_len, &sub_arrays, &read_thumbnail) == FAILURE) {
		return;
	}

	memset(&ImageInfo, 0, sizeof(ImageInfo));

	if (p_sections_needed) {
		spprintf(&sections_str, 0, ",%s,", p_sections_needed);
		/* sections_str DOES start with , and SPACES are NOT allowed in names */
		s = sections_str;
		while (*++s) {
			if (*s == ' ') {
				*s = ',';
			}
		}

		for (i = 0; i < SECTION_COUNT; i++) {
			snprintf(tmp, sizeof(tmp), ",%s,", exif_get_sectionname(i));
			if (strstr(sections_str, tmp)) {
				sections_needed |= 1<<i;
			}
		}
		EFREE_IF(sections_str);
		/* now see what we need */
#ifdef EXIF_DEBUG
		sections_str = exif_get_sectionlist(sections_needed TSRMLS_CC);
		if (!sections_str) {
			RETURN_FALSE;
		}
		exif_error_docref(NULL EXIFERR_CC, &ImageInfo, E_NOTICE, "Sections needed: %s", sections_str[0] ? sections_str : "None");
		EFREE_IF(sections_str);
#endif
	}

	ret = exif_read_file(&ImageInfo, p_name, read_thumbnail, read_all TSRMLS_CC);
	sections_str = exif_get_sectionlist(ImageInfo.sections_found TSRMLS_CC);

#ifdef EXIF_DEBUG
	if (sections_str) 
		exif_error_docref(NULL EXIFERR_CC, &ImageInfo, E_NOTICE, "Sections found: %s", sections_str[0] ? sections_str : "None");
#endif

	ImageInfo.sections_found |= FOUND_COMPUTED|FOUND_FILE;/* do not inform about in debug*/

	if (ret == FALSE || (sections_needed && !(sections_needed&ImageInfo.sections_found))) {
		/* array_init must be checked at last! otherwise the array must be freed if a later test fails. */
		exif_discard_imageinfo(&ImageInfo);
	   	EFREE_IF(sections_str);
		RETURN_FALSE;
	}

	array_init(return_value);

#ifdef EXIF_DEBUG
	exif_error_docref(NULL EXIFERR_CC, &ImageInfo, E_NOTICE, "Generate section FILE");
#endif

	/* now we can add our information */
	exif_iif_add_str(&ImageInfo, SECTION_FILE, "FileName",      ImageInfo.FileName TSRMLS_CC);
	exif_iif_add_int(&ImageInfo, SECTION_FILE, "FileDateTime",  ImageInfo.FileDateTime TSRMLS_CC);
	exif_iif_add_int(&ImageInfo, SECTION_FILE, "FileSize",      ImageInfo.FileSize TSRMLS_CC);
	exif_iif_add_int(&ImageInfo, SECTION_FILE, "FileType",      ImageInfo.FileType TSRMLS_CC);
	exif_iif_add_str(&ImageInfo, SECTION_FILE, "MimeType",      (char*)php_image_type_to_mime_type(ImageInfo.FileType) TSRMLS_CC);
	exif_iif_add_str(&ImageInfo, SECTION_FILE, "SectionsFound", sections_str ? sections_str : "NONE" TSRMLS_CC);

#ifdef EXIF_DEBUG
	exif_error_docref(NULL EXIFERR_CC, &ImageInfo, E_NOTICE, "Generate section COMPUTED");
#endif

	if (ImageInfo.Width>0 &&  ImageInfo.Height>0) {
		exif_iif_add_fmt(&ImageInfo, SECTION_COMPUTED, "html"    TSRMLS_CC, "width=\"%d\" height=\"%d\"", ImageInfo.Width, ImageInfo.Height);
		exif_iif_add_int(&ImageInfo, SECTION_COMPUTED, "Height", ImageInfo.Height TSRMLS_CC);
		exif_iif_add_int(&ImageInfo, SECTION_COMPUTED, "Width",  ImageInfo.Width TSRMLS_CC);
	}
	exif_iif_add_int(&ImageInfo, SECTION_COMPUTED, "IsColor", ImageInfo.IsColor TSRMLS_CC);
	if (ImageInfo.motorola_intel != -1) {
		exif_iif_add_int(&ImageInfo, SECTION_COMPUTED, "ByteOrderMotorola", ImageInfo.motorola_intel TSRMLS_CC);
	}
	if (ImageInfo.FocalLength) {
		exif_iif_add_fmt(&ImageInfo, SECTION_COMPUTED, "FocalLength" TSRMLS_CC, "%4.1Fmm", ImageInfo.FocalLength);
		if(ImageInfo.CCDWidth) {
			exif_iif_add_fmt(&ImageInfo, SECTION_COMPUTED, "35mmFocalLength" TSRMLS_CC, "%dmm", (int)(ImageInfo.FocalLength/ImageInfo.CCDWidth*35+0.5));
		}
	}
	if(ImageInfo.CCDWidth) {
		exif_iif_add_fmt(&ImageInfo, SECTION_COMPUTED, "CCDWidth" TSRMLS_CC, "%dmm", (int)ImageInfo.CCDWidth);
	}
	if(ImageInfo.ExposureTime>0) {
		if(ImageInfo.ExposureTime <= 0.5) {
			exif_iif_add_fmt(&ImageInfo, SECTION_COMPUTED, "ExposureTime" TSRMLS_CC, "%0.3F s (1/%d)", ImageInfo.ExposureTime, (int)(0.5 + 1/ImageInfo.ExposureTime));
		} else {
			exif_iif_add_fmt(&ImageInfo, SECTION_COMPUTED, "ExposureTime" TSRMLS_CC, "%0.3F s", ImageInfo.ExposureTime);
		}
	}
	if(ImageInfo.ApertureFNumber) {
		exif_iif_add_fmt(&ImageInfo, SECTION_COMPUTED, "ApertureFNumber" TSRMLS_CC, "f/%.1F", ImageInfo.ApertureFNumber);
	}
	if(ImageInfo.Distance) {
		if(ImageInfo.Distance<0) {
			exif_iif_add_str(&ImageInfo, SECTION_COMPUTED, "FocusDistance", "Infinite" TSRMLS_CC);
		} else {
			exif_iif_add_fmt(&ImageInfo, SECTION_COMPUTED, "FocusDistance" TSRMLS_CC, "%0.2Fm", ImageInfo.Distance);
		}
	}
	if (ImageInfo.UserComment) {
		exif_iif_add_buffer(&ImageInfo, SECTION_COMPUTED, "UserComment", ImageInfo.UserCommentLength, ImageInfo.UserComment TSRMLS_CC);
		if (ImageInfo.UserCommentEncoding && strlen(ImageInfo.UserCommentEncoding)) {
			exif_iif_add_str(&ImageInfo, SECTION_COMPUTED, "UserCommentEncoding", ImageInfo.UserCommentEncoding TSRMLS_CC);
		}
	}

	exif_iif_add_str(&ImageInfo, SECTION_COMPUTED, "Copyright",              ImageInfo.Copyright TSRMLS_CC);
	exif_iif_add_str(&ImageInfo, SECTION_COMPUTED, "Copyright.Photographer", ImageInfo.CopyrightPhotographer TSRMLS_CC);
	exif_iif_add_str(&ImageInfo, SECTION_COMPUTED, "Copyright.Editor",       ImageInfo.CopyrightEditor TSRMLS_CC);

	for (i=0; i<ImageInfo.xp_fields.count; i++) {
		exif_iif_add_str(&ImageInfo, SECTION_WINXP, exif_get_tagname(ImageInfo.xp_fields.list[i].tag, NULL, 0, exif_get_tag_table(SECTION_WINXP) TSRMLS_CC), ImageInfo.xp_fields.list[i].value TSRMLS_CC);
	}
	if (ImageInfo.Thumbnail.size) {
		if (read_thumbnail) {
			/* not exif_iif_add_str : this is a buffer */
			exif_iif_add_tag(&ImageInfo, SECTION_THUMBNAIL, "THUMBNAIL", TAG_NONE, TAG_FMT_UNDEFINED, ImageInfo.Thumbnail.size, ImageInfo.Thumbnail.data TSRMLS_CC);
		}
		if (!ImageInfo.Thumbnail.width || !ImageInfo.Thumbnail.height) {
			/* try to evaluate if thumbnail data is present */
			exif_scan_thumbnail(&ImageInfo TSRMLS_CC);
		}
		exif_iif_add_int(&ImageInfo, SECTION_COMPUTED, "Thumbnail.FileType", ImageInfo.Thumbnail.filetype TSRMLS_CC);
		exif_iif_add_str(&ImageInfo, SECTION_COMPUTED, "Thumbnail.MimeType", (char*)php_image_type_to_mime_type(ImageInfo.Thumbnail.filetype) TSRMLS_CC);
	}
	if (ImageInfo.Thumbnail.width && ImageInfo.Thumbnail.height) {
		exif_iif_add_int(&ImageInfo, SECTION_COMPUTED, "Thumbnail.Height", ImageInfo.Thumbnail.height TSRMLS_CC);
		exif_iif_add_int(&ImageInfo, SECTION_COMPUTED, "Thumbnail.Width",  ImageInfo.Thumbnail.width TSRMLS_CC);
	}
   	EFREE_IF(sections_str);

#ifdef EXIF_DEBUG
	exif_error_docref(NULL EXIFERR_CC, &ImageInfo, E_NOTICE, "Adding image infos");
#endif

	add_assoc_image_info(return_value, sub_arrays, &ImageInfo, SECTION_FILE       TSRMLS_CC);
	add_assoc_image_info(return_value, 1,          &ImageInfo, SECTION_COMPUTED   TSRMLS_CC);
	add_assoc_image_info(return_value, sub_arrays, &ImageInfo, SECTION_ANY_TAG    TSRMLS_CC);
	add_assoc_image_info(return_value, sub_arrays, &ImageInfo, SECTION_IFD0       TSRMLS_CC);
	add_assoc_image_info(return_value, 1,          &ImageInfo, SECTION_THUMBNAIL  TSRMLS_CC);
	add_assoc_image_info(return_value, 1,          &ImageInfo, SECTION_COMMENT    TSRMLS_CC);
	add_assoc_image_info(return_value, sub_arrays, &ImageInfo, SECTION_EXIF       TSRMLS_CC);
	add_assoc_image_info(return_value, sub_arrays, &ImageInfo, SECTION_GPS        TSRMLS_CC);
	add_assoc_image_info(return_value, sub_arrays, &ImageInfo, SECTION_INTEROP    TSRMLS_CC);
	add_assoc_image_info(return_value, sub_arrays, &ImageInfo, SECTION_FPIX       TSRMLS_CC);
	add_assoc_image_info(return_value, sub_arrays, &ImageInfo, SECTION_APP12      TSRMLS_CC);
	add_assoc_image_info(return_value, sub_arrays, &ImageInfo, SECTION_WINXP      TSRMLS_CC);
	add_assoc_image_info(return_value, sub_arrays, &ImageInfo, SECTION_MAKERNOTE  TSRMLS_CC);

#ifdef EXIF_DEBUG
	exif_error_docref(NULL EXIFERR_CC, &ImageInfo, E_NOTICE, "Discarding info");
#endif

	exif_discard_imageinfo(&ImageInfo);

#ifdef EXIF_DEBUG
	php_error_docref1(NULL TSRMLS_CC, Z_STRVAL_PP(p_name), E_NOTICE, "done");
#endif
}