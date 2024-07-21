GF_Err infe_box_read(GF_Box *s, GF_BitStream *bs)
{
	char *buf;
	u32 buf_len, i, string_len, string_start;
	GF_ItemInfoEntryBox *ptr = (GF_ItemInfoEntryBox *)s;

	ISOM_DECREASE_SIZE(ptr, 4);
	ptr->item_ID = gf_bs_read_u16(bs);
	ptr->item_protection_index = gf_bs_read_u16(bs);

	if (ptr->version == 2) {
		ISOM_DECREASE_SIZE(ptr, 4);
		ptr->item_type = gf_bs_read_u32(bs);
	}
	buf_len = (u32) (ptr->size);
	buf = (char*)gf_malloc(buf_len);
	if (!buf) return GF_OUT_OF_MEM;
	if (buf_len != gf_bs_read_data(bs, buf, buf_len)) {
		gf_free(buf);
		return GF_ISOM_INVALID_FILE;
	}
	string_len = 1;
	string_start = 0;
	for (i = 0; i < buf_len; i++) {
		if (buf[i] == 0) {
			if (!ptr->item_name) {
				ptr->item_name = (char*)gf_malloc(sizeof(char)*string_len);
				if (!ptr->item_name) return GF_OUT_OF_MEM;
				memcpy(ptr->item_name, buf+string_start, string_len);
			} else if (!ptr->content_type) {
				ptr->content_type = (char*)gf_malloc(sizeof(char)*string_len);
				if (!ptr->content_type) return GF_OUT_OF_MEM;
				memcpy(ptr->content_type, buf+string_start, string_len);
			} else {
				ptr->content_encoding = (char*)gf_malloc(sizeof(char)*string_len);
				if (!ptr->content_encoding) return GF_OUT_OF_MEM;
				memcpy(ptr->content_encoding, buf+string_start, string_len);
			}
			string_start += string_len;
			string_len = 0;
			if (ptr->content_encoding && ptr->version == 1) {
				break;
			}
		}
		string_len++;
	}
	gf_free(buf);
	if (!ptr->item_name || (!ptr->content_type && ptr->version < 2)) {
		GF_LOG(GF_LOG_WARNING, GF_LOG_CONTAINER, ("[isoff] Infe without name or content type !\n"));
	}
	return GF_OK;
}