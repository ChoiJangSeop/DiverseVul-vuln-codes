static void _php_image_output_ctx(INTERNAL_FUNCTION_PARAMETERS, int image_type, char *tn, void (*func_p)())
{
	zval *imgind;
	char *file = NULL;
	int file_len = 0;
	long quality, basefilter;
	gdImagePtr im;
	FILE *fp = NULL;
	int argc = ZEND_NUM_ARGS();
	int q = -1, i;
	int f = -1;
	gdIOCtx *ctx;

	/* The third (quality) parameter for Wbmp stands for the threshold when called from image2wbmp().
	 * The third (quality) parameter for Wbmp and Xbm stands for the foreground color index when called
	 * from imagey<type>().
	 */

	if (image_type == PHP_GDIMG_TYPE_XBM) {
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs!|ll", &imgind, &file, &file_len, &quality, &basefilter) == FAILURE) {
			return;
		}
	} else {
		/* PHP_GDIMG_TYPE_GIF
		 * PHP_GDIMG_TYPE_PNG 
		 * PHP_GDIMG_TYPE_JPG 
		 * PHP_GDIMG_TYPE_WBM */
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r|s!ll", &imgind, &file, &file_len, &quality, &basefilter) == FAILURE) {
			return;
		}		
	}

	ZEND_FETCH_RESOURCE(im, gdImagePtr, &imgind, -1, "Image", phpi_get_le_gd());

	if (argc > 1) {
		if (argc >= 3) {
			q = quality; /* or colorindex for foreground of BW images (defaults to black) */
			if (argc == 4) {
				f = basefilter;
			}
		}
	}

	if (argc > 1 && file_len) {
		PHP_GD_CHECK_OPEN_BASEDIR(file, "Invalid filename");

		fp = VCWD_FOPEN(file, "wb");
		if (!fp) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to open '%s' for writing: %s", file, strerror(errno));
			RETURN_FALSE;
		}

		ctx = gdNewFileCtx(fp);
	} else {
		ctx = emalloc(sizeof(gdIOCtx));
		ctx->putC = _php_image_output_putc;
		ctx->putBuf = _php_image_output_putbuf;
#if HAVE_LIBGD204
		ctx->gd_free = _php_image_output_ctxfree;
#else
		ctx->free = _php_image_output_ctxfree;
#endif

#if APACHE && defined(CHARSET_EBCDIC)
		/* XXX this is unlikely to work any more thies@thieso.net */
		/* This is a binary file already: avoid EBCDIC->ASCII conversion */
		ap_bsetflag(php3_rqst->connection->client, B_EBCDIC2ASCII, 0);
#endif
	}

	switch(image_type) {
		case PHP_GDIMG_CONVERT_WBM:
			if(q<0||q>255) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid threshold value '%d'. It must be between 0 and 255", q);
			}
		case PHP_GDIMG_TYPE_JPG:
			(*func_p)(im, ctx, q);
			break;
		case PHP_GDIMG_TYPE_PNG:
			(*func_p)(im, ctx, q, f);
			break;
		case PHP_GDIMG_TYPE_XBM:
		case PHP_GDIMG_TYPE_WBM:
			if (argc < 3) {
				for(i=0; i < gdImageColorsTotal(im); i++) {
					if(!gdImageRed(im, i) && !gdImageGreen(im, i) && !gdImageBlue(im, i)) break;
				}
				q = i;
			}
			if (image_type == PHP_GDIMG_TYPE_XBM) {
				(*func_p)(im, file, q, ctx);
			} else {
				(*func_p)(im, q, ctx);
			}
			break;
		default:
			(*func_p)(im, ctx);
			break;
	}

#if HAVE_LIBGD204
	ctx->gd_free(ctx);
#else
	ctx->free(ctx);
#endif

	if(fp) {
		fflush(fp);
		fclose(fp);
	}

    RETURN_TRUE;
}