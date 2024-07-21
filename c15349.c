GF_Err xtra_box_read(GF_Box *s, GF_BitStream *bs)
{
	GF_XtraBox *ptr = (GF_XtraBox *)s;
	while (ptr->size) {
		GF_XtraTag *tag;
		u32 prop_type = 0;

		char *data=NULL, *data2=NULL;
		ISOM_DECREASE_SIZE_NO_ERR(ptr, 8)
		s32 tag_size = gf_bs_read_u32(bs);
		u32 name_size = gf_bs_read_u32(bs);
		if (tag_size < 8) return GF_ISOM_INVALID_FILE;

		tag_size -= 8;
		if ((tag_size>ptr->size) || (name_size>ptr->size)) {
			return GF_ISOM_INVALID_FILE;
		}
		ISOM_DECREASE_SIZE_NO_ERR(ptr, 10)

		ISOM_DECREASE_SIZE_NO_ERR(ptr, name_size)
		data = gf_malloc(sizeof(char) * (name_size+1));
		gf_bs_read_data(bs, data, name_size);
		data[name_size] = 0;
		tag_size-=name_size;

		u32 flags = gf_bs_read_u32(bs);
		u32 prop_size = gf_bs_read_u32(bs);
		tag_size-=8;

		if (prop_size>4) {
			tag_size-=2;
			prop_type = gf_bs_read_u16(bs);
			prop_size -= 6;
			ISOM_DECREASE_SIZE_NO_ERR(ptr, prop_size)
			//add 2 extra bytes for UTF16 case string dump
			data2 = gf_malloc(sizeof(char) * (prop_size+2));
			gf_bs_read_data(bs, data2, prop_size);
			data2[prop_size] = 0;
			data2[prop_size+1] = 0;
			tag_size-=prop_size;
		} else {
			prop_size = 0;
		}
		GF_SAFEALLOC(tag, GF_XtraTag)
		tag->flags = flags;
		tag->name = data;
		tag->prop_size = prop_size;
		tag->prop_value = data2;
		tag->prop_type = prop_type;
		gf_list_add(ptr->tags, tag);

		if (tag_size) {
			GF_LOG(GF_LOG_WARNING, GF_LOG_CONTAINER, ("[isom] invalid tag size in Xtra !\n"));
		}
	}
	return GF_OK;
}