GF_Err schm_box_read(GF_Box *s, GF_BitStream *bs)
{
	GF_SchemeTypeBox *ptr = (GF_SchemeTypeBox *)s;

	ISOM_DECREASE_SIZE(ptr, 8);
	ptr->scheme_type = gf_bs_read_u32(bs);
	ptr->scheme_version = gf_bs_read_u32(bs);

	if (ptr->size && (ptr->flags & 0x000001)) {
		u32 len = (u32) (ptr->size);
		ptr->URI = (char*)gf_malloc(sizeof(char)*len);
		if (!ptr->URI) return GF_OUT_OF_MEM;
		gf_bs_read_data(bs, ptr->URI, len);
	}
	return GF_OK;
}