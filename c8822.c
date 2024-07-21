static int exif_scan_JPEG_header(image_info_type *ImageInfo)
{
	int section, sn;
	int marker = 0, last_marker = M_PSEUDO, comment_correction=1;
	unsigned int ll, lh;
	uchar *Data;
	size_t fpos, size, got, itemlen;
	jpeg_sof_info sof_info;

	for(section=0;;section++) {
#ifdef EXIF_DEBUG
		fpos = php_stream_tell(ImageInfo->infile);
		exif_error_docref(NULL EXIFERR_CC, ImageInfo, E_NOTICE, "Needing section %d @ 0x%08X", ImageInfo->file.count, fpos);
#endif

		/* get marker byte, swallowing possible padding                           */
		/* some software does not count the length bytes of COM section           */
		/* one company doing so is very much involved in JPEG... so we accept too */
		if (last_marker==M_COM && comment_correction) {
			comment_correction = 2;
		}
		do {
			if ((marker = php_stream_getc(ImageInfo->infile)) == EOF) {
				EXIF_ERRLOG_CORRUPT(ImageInfo)
				return FALSE;
			}
			if (last_marker==M_COM && comment_correction>0) {
				if (marker!=0xFF) {
					marker = 0xff;
					comment_correction--;
				} else  {
					last_marker = M_PSEUDO; /* stop skipping 0 for M_COM */
				}
			}
		} while (marker == 0xff);
		if (last_marker==M_COM && !comment_correction) {
			exif_error_docref("exif_read_data#error_mcom" EXIFERR_CC, ImageInfo, E_NOTICE, "Image has corrupt COM section: some software set wrong length information");
		}
		if (last_marker==M_COM && comment_correction)
			return M_EOI; /* ah illegal: char after COM section not 0xFF */

		fpos = php_stream_tell(ImageInfo->infile);

		if (marker == 0xff) {
			/* 0xff is legal padding, but if we get that many, something's wrong. */
			exif_error_docref(NULL EXIFERR_CC, ImageInfo, E_WARNING, "To many padding bytes");
			return FALSE;
		}

		/* Read the length of the section. */
		if ((lh = php_stream_getc(ImageInfo->infile)) == (unsigned int)EOF) {
			EXIF_ERRLOG_CORRUPT(ImageInfo)
			return FALSE;
		}
		if ((ll = php_stream_getc(ImageInfo->infile)) == (unsigned int)EOF) {
			EXIF_ERRLOG_CORRUPT(ImageInfo)
			return FALSE;
		}

		itemlen = (lh << 8) | ll;

		if (itemlen < 2) {
#ifdef EXIF_DEBUG
			exif_error_docref(NULL EXIFERR_CC, ImageInfo, E_WARNING, "%s, Section length: 0x%02X%02X", EXIF_ERROR_CORRUPT, lh, ll);
#else
			EXIF_ERRLOG_CORRUPT(ImageInfo)
#endif
			return FALSE;
		}

		sn = exif_file_sections_add(ImageInfo, marker, itemlen+1, NULL);
		Data = ImageInfo->file.list[sn].data;

		/* Store first two pre-read bytes. */
		Data[0] = (uchar)lh;
		Data[1] = (uchar)ll;

		got = php_stream_read(ImageInfo->infile, (char*)(Data+2), itemlen-2); /* Read the whole section. */
		if (got != itemlen-2) {
			exif_error_docref(NULL EXIFERR_CC, ImageInfo, E_WARNING, "Error reading from file: got=x%04X(=%d) != itemlen-2=x%04X(=%d)", got, got, itemlen-2, itemlen-2);
			return FALSE;
		}

#ifdef EXIF_DEBUG
		exif_error_docref(NULL EXIFERR_CC, ImageInfo, E_NOTICE, "Process section(x%02X=%s) @ x%04X + x%04X(=%d)", marker, exif_get_markername(marker), fpos, itemlen, itemlen);
#endif
		switch(marker) {
			case M_SOS:   /* stop before hitting compressed data  */
				/* If reading entire image is requested, read the rest of the data. */
				if (ImageInfo->read_all) {
					/* Determine how much file is left. */
					fpos = php_stream_tell(ImageInfo->infile);
					size = ImageInfo->FileSize - fpos;
					sn = exif_file_sections_add(ImageInfo, M_PSEUDO, size, NULL);
					Data = ImageInfo->file.list[sn].data;
					got = php_stream_read(ImageInfo->infile, (char*)Data, size);
					if (got != size) {
						EXIF_ERRLOG_FILEEOF(ImageInfo)
						return FALSE;
					}
				}
				return TRUE;

			case M_EOI:   /* in case it's a tables-only JPEG stream */
				exif_error_docref(NULL EXIFERR_CC, ImageInfo, E_WARNING, "No image in jpeg!");
				return (ImageInfo->sections_found&(~FOUND_COMPUTED)) ? TRUE : FALSE;

			case M_COM: /* Comment section */
				exif_process_COM(ImageInfo, (char *)Data, itemlen);
				break;

			case M_EXIF:
				if (!(ImageInfo->sections_found&FOUND_IFD0)) {
					/*ImageInfo->sections_found |= FOUND_EXIF;*/
					/* Seen files from some 'U-lead' software with Vivitar scanner
					   that uses marker 31 later in the file (no clue what for!) */
					exif_process_APP1(ImageInfo, (char *)Data, itemlen, fpos);
				}
				break;

			case M_APP12:
				exif_process_APP12(ImageInfo, (char *)Data, itemlen);
				break;


			case M_SOF0:
			case M_SOF1:
			case M_SOF2:
			case M_SOF3:
			case M_SOF5:
			case M_SOF6:
			case M_SOF7:
			case M_SOF9:
			case M_SOF10:
			case M_SOF11:
			case M_SOF13:
			case M_SOF14:
			case M_SOF15:
				if ((itemlen - 2) < 6) {
					return FALSE;
				}

				exif_process_SOFn(Data, marker, &sof_info);
				ImageInfo->Width  = sof_info.width;
				ImageInfo->Height = sof_info.height;
				if (sof_info.num_components == 3) {
					ImageInfo->IsColor = 1;
				} else {
					ImageInfo->IsColor = 0;
				}
				break;
			default:
				/* skip any other marker silently. */
				break;
		}

		/* keep track of last marker */
		last_marker = marker;
	}
#ifdef EXIF_DEBUG
	exif_error_docref(NULL EXIFERR_CC, ImageInfo, E_NOTICE, "Done");
#endif
	return TRUE;
}