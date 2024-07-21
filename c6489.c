static void exif_thumbnail_extract(image_info_type *ImageInfo, char *offset, size_t length TSRMLS_DC) {
	if (ImageInfo->Thumbnail.data) {
		exif_error_docref("exif_read_data#error_mult_thumb" EXIFERR_CC, ImageInfo, E_WARNING, "Multiple possible thumbnails");
		return; /* Should not happen */
	}
	if (!ImageInfo->read_thumbnail)	{
		return; /* ignore this call */
	}
	/* according to exif2.1, the thumbnail is not supposed to be greater than 64K */
	if (ImageInfo->Thumbnail.size >= 65536
	 || ImageInfo->Thumbnail.size <= 0
	 || ImageInfo->Thumbnail.offset <= 0
	) {
		exif_error_docref(NULL EXIFERR_CC, ImageInfo, E_WARNING, "Illegal thumbnail size/offset");
		return;
	}
	/* Check to make sure we are not going to go past the ExifLength */
	if ((ImageInfo->Thumbnail.offset + ImageInfo->Thumbnail.size) > length) {
		EXIF_ERRLOG_THUMBEOF(ImageInfo)
		return;
	}
	ImageInfo->Thumbnail.data = estrndup(offset + ImageInfo->Thumbnail.offset, ImageInfo->Thumbnail.size);
	exif_thumbnail_build(ImageInfo TSRMLS_CC);
}