static int exif_scan_FILE_header(image_info_type *ImageInfo TSRMLS_DC)
{
	unsigned char file_header[8];
	int ret = FALSE;

	ImageInfo->FileType = IMAGE_FILETYPE_UNKNOWN;

	if (ImageInfo->FileSize >= 2) {
		php_stream_seek(ImageInfo->infile, 0, SEEK_SET);
		if (php_stream_read(ImageInfo->infile, (char*)file_header, 2) != 2) {
			return FALSE;
		}
		if ((file_header[0]==0xff) && (file_header[1]==M_SOI)) {
			ImageInfo->FileType = IMAGE_FILETYPE_JPEG;
			if (exif_scan_JPEG_header(ImageInfo TSRMLS_CC)) {
				ret = TRUE;
			} else {
				exif_error_docref(NULL EXIFERR_CC, ImageInfo, E_WARNING, "Invalid JPEG file");
			}
		} else if (ImageInfo->FileSize >= 8) {
			if (php_stream_read(ImageInfo->infile, (char*)(file_header+2), 6) != 6) {
				return FALSE;
			}
			if (!memcmp(file_header, "II\x2A\x00", 4)) {
				ImageInfo->FileType = IMAGE_FILETYPE_TIFF_II;
				ImageInfo->motorola_intel = 0;
#ifdef EXIF_DEBUG
				exif_error_docref(NULL EXIFERR_CC, ImageInfo, E_NOTICE, "File has TIFF/II format");
#endif
				ImageInfo->sections_found |= FOUND_IFD0;
				if (exif_process_IFD_in_TIFF(ImageInfo, 
											 php_ifd_get32u(file_header + 4, ImageInfo->motorola_intel),
											 SECTION_IFD0 TSRMLS_CC)) {
					ret = TRUE;
				} else {
					exif_error_docref(NULL EXIFERR_CC, ImageInfo, E_WARNING, "Invalid TIFF file");
				}
			} else if (!memcmp(file_header, "MM\x00\x2a", 4)) {
				ImageInfo->FileType = IMAGE_FILETYPE_TIFF_MM;
				ImageInfo->motorola_intel = 1;
#ifdef EXIF_DEBUG
				exif_error_docref(NULL EXIFERR_CC, ImageInfo, E_NOTICE, "File has TIFF/MM format");
#endif
				ImageInfo->sections_found |= FOUND_IFD0;
				if (exif_process_IFD_in_TIFF(ImageInfo,
											 php_ifd_get32u(file_header + 4, ImageInfo->motorola_intel),
											 SECTION_IFD0 TSRMLS_CC)) {
					ret = TRUE;
				} else {
					exif_error_docref(NULL EXIFERR_CC, ImageInfo, E_WARNING, "Invalid TIFF file");
				}
			} else {
				exif_error_docref(NULL EXIFERR_CC, ImageInfo, E_WARNING, "File not supported");
				return FALSE;
			}
		}
	} else {
		exif_error_docref(NULL EXIFERR_CC, ImageInfo, E_WARNING, "File too small (%d)", ImageInfo->FileSize);
	}
	return ret;
}