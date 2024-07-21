PHPAPI int php_getimagetype(php_stream * stream, char *filetype TSRMLS_DC)
{
	char tmp[12];

	if ( !filetype) filetype = tmp;
	if((php_stream_read(stream, filetype, 3)) != 3) {
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Read error!");
		return IMAGE_FILETYPE_UNKNOWN;
	}

/* BYTES READ: 3 */
	if (!memcmp(filetype, php_sig_gif, 3)) {
		return IMAGE_FILETYPE_GIF;
	} else if (!memcmp(filetype, php_sig_jpg, 3)) {
		return IMAGE_FILETYPE_JPEG;
	} else if (!memcmp(filetype, php_sig_png, 3)) {
		if (php_stream_read(stream, filetype+3, 5) != 5) {
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Read error!");
			return IMAGE_FILETYPE_UNKNOWN;
		}
		if (!memcmp(filetype, php_sig_png, 8)) {
			return IMAGE_FILETYPE_PNG;
		} else {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "PNG file corrupted by ASCII conversion");
			return IMAGE_FILETYPE_UNKNOWN;
		}
	} else if (!memcmp(filetype, php_sig_swf, 3)) {
		return IMAGE_FILETYPE_SWF;
	} else if (!memcmp(filetype, php_sig_swc, 3)) {
		return IMAGE_FILETYPE_SWC;
	} else if (!memcmp(filetype, php_sig_psd, 3)) {
		return IMAGE_FILETYPE_PSD;
	} else if (!memcmp(filetype, php_sig_bmp, 2)) {
		return IMAGE_FILETYPE_BMP;
	} else if (!memcmp(filetype, php_sig_jpc, 3)) {
		return IMAGE_FILETYPE_JPC;
	}

	if (php_stream_read(stream, filetype+3, 1) != 1) {
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Read error!");
		return IMAGE_FILETYPE_UNKNOWN;
	}
/* BYTES READ: 4 */
	if (!memcmp(filetype, php_sig_tif_ii, 4)) {
		return IMAGE_FILETYPE_TIFF_II;
	} else if (!memcmp(filetype, php_sig_tif_mm, 4)) {
		return IMAGE_FILETYPE_TIFF_MM;
	} else if (!memcmp(filetype, php_sig_iff, 4)) {
		return IMAGE_FILETYPE_IFF;
	} else if (!memcmp(filetype, php_sig_ico, 4)) {
		return IMAGE_FILETYPE_ICO;
	}

	if (php_stream_read(stream, filetype+4, 8) != 8) {
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Read error!");
		return IMAGE_FILETYPE_UNKNOWN;
	}
/* BYTES READ: 12 */
   	if (!memcmp(filetype, php_sig_jp2, 12)) {
		return IMAGE_FILETYPE_JP2;
	}

/* AFTER ALL ABOVE FAILED */
	if (php_get_wbmp(stream, NULL, 1 TSRMLS_CC)) {
		return IMAGE_FILETYPE_WBMP;
	}
	if (php_get_xbm(stream, NULL TSRMLS_CC)) {
		return IMAGE_FILETYPE_XBM;
	}
	return IMAGE_FILETYPE_UNKNOWN;
}