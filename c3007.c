char * exif_dump_data(int *dump_free, int format, int components, int length, int motorola_intel, char *value_ptr TSRMLS_DC) /* {{{ */
{
	char *dump;
	int len;

	*dump_free = 0;
	if (format == TAG_FMT_STRING) {
		return value_ptr ? value_ptr : "<no data>";
	}
	if (format == TAG_FMT_UNDEFINED) {
		return "<undefined>\n";
	}
	if (format == TAG_FMT_IFD) {
		return "";
	}
	if (format == TAG_FMT_SINGLE || format == TAG_FMT_DOUBLE) {
		return "<not implemented>";
	}
	*dump_free = 1;
	if (components > 1) {
		len = spprintf(&dump, 0, "(%d,%d) {", components, length);
	} else {
		len = spprintf(&dump, 0, "{");
	}
	while(components > 0) {
		switch(format) {
			case TAG_FMT_BYTE:
			case TAG_FMT_UNDEFINED:
			case TAG_FMT_STRING:
			case TAG_FMT_SBYTE:
				dump = erealloc(dump, len + 4 + 1);
				snprintf(dump + len, 4 + 1, "0x%02X", *value_ptr);
				len += 4;
				value_ptr++;
				break;
			case TAG_FMT_USHORT:
			case TAG_FMT_SSHORT:
				dump = erealloc(dump, len + 6 + 1);
				snprintf(dump + len, 6 + 1, "0x%04X", php_ifd_get16s(value_ptr, motorola_intel));
				len += 6;
				value_ptr += 2;
				break;
			case TAG_FMT_ULONG:
			case TAG_FMT_SLONG:
				dump = erealloc(dump, len + 6 + 1);
				snprintf(dump + len, 6 + 1, "0x%04X", php_ifd_get32s(value_ptr, motorola_intel));
				len += 6;
				value_ptr += 4;
				break;
			case TAG_FMT_URATIONAL:
			case TAG_FMT_SRATIONAL:
				dump = erealloc(dump, len + 13 + 1);
				snprintf(dump + len, 13 + 1, "0x%04X/0x%04X", php_ifd_get32s(value_ptr, motorola_intel), php_ifd_get32s(value_ptr+4, motorola_intel));
				len += 13;
				value_ptr += 8;
				break;
		}
		if (components > 0) {
			dump = erealloc(dump, len + 2 + 1);
			snprintf(dump + len, 2 + 1, ", ");
			len += 2;			
			components--;
		} else{
			break;
		}
	}
	dump = erealloc(dump, len + 1 + 1);
	snprintf(dump + len, 1 + 1, "}");
	return dump;
}