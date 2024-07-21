GF_Err colr_box_dump(GF_Box *a, FILE * trace)
{
	u8 *prof_data_64=NULL;
	u32 size_64;
	GF_ColourInformationBox *ptr = (GF_ColourInformationBox *)a;
	if (!a) return GF_BAD_PARAM;

	gf_isom_box_dump_start(a, "ColourInformationBox", trace);

	if (ptr->is_jp2) {
		gf_fprintf(trace, "method=\"%d\" precedence=\"%d\" approx=\"%d\"", ptr->method, ptr->precedence, ptr->approx);
		if (ptr->opaque_size) {
			gf_fprintf(trace, " colour=\"");
			dump_data_hex(trace, ptr->opaque,ptr->opaque_size);
			gf_fprintf(trace, "\"");
		}
		gf_fprintf(trace, ">\n");
	} else {
		switch (ptr->colour_type) {
		case GF_ISOM_SUBTYPE_NCLC:
			gf_fprintf(trace, "colour_type=\"%s\" colour_primaries=\"%d\" transfer_characteristics=\"%d\" matrix_coefficients=\"%d\">\n", gf_4cc_to_str(ptr->colour_type), ptr->colour_primaries, ptr->transfer_characteristics, ptr->matrix_coefficients);
			break;
		case GF_ISOM_SUBTYPE_NCLX:
			gf_fprintf(trace, "colour_type=\"%s\" colour_primaries=\"%d\" transfer_characteristics=\"%d\" matrix_coefficients=\"%d\" full_range_flag=\"%d\">\n", gf_4cc_to_str(ptr->colour_type), ptr->colour_primaries, ptr->transfer_characteristics, ptr->matrix_coefficients, ptr->full_range_flag);
			break;
		case GF_ISOM_SUBTYPE_PROF:
		case GF_ISOM_SUBTYPE_RICC:
			gf_fprintf(trace, "colour_type=\"%s\">\n", gf_4cc_to_str(ptr->colour_type));
			if (ptr->opaque != NULL) {
				gf_fprintf(trace, "<profile><![CDATA[");
				size_64 = 2*ptr->opaque_size;
				prof_data_64 = gf_malloc(size_64);
				size_64 = gf_base64_encode((const char *) ptr->opaque, ptr->opaque_size, (char *)prof_data_64, size_64);
				prof_data_64[size_64] = 0;
				gf_fprintf(trace, "%s", prof_data_64);
				gf_fprintf(trace, "]]></profile>");
			}
			break;
		default:
			gf_fprintf(trace, "colour_type=\"%s\">\n", gf_4cc_to_str(ptr->colour_type));
			break;
		}
	}

	gf_isom_box_dump_done("ColourInformationBox", a, trace);
	return GF_OK;
}