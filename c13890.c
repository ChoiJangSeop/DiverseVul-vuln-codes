GF_Err gf_isom_apple_enum_tag(GF_ISOFile *mov, u32 idx, GF_ISOiTunesTag *out_tag, const u8 **data, u32 *data_len, u64 *out_int_val, u32 *out_int_val2, u32 *out_flags)
{
	u32 i, child_index;
	GF_ListItemBox *info;
	GF_ItemListBox *ilst;
	GF_MetaBox *meta;
	GF_DataBox *dbox = NULL;
	u32 itype, tag_val;
	s32 tag_idx;
	*data = NULL;
	*data_len = 0;

	meta = (GF_MetaBox *) gf_isom_get_meta_extensions(mov, GF_FALSE);
	if (!meta) return GF_URL_ERROR;

	ilst = gf_isom_locate_box(meta->child_boxes, GF_ISOM_BOX_TYPE_ILST, NULL);
	if (!ilst) return GF_URL_ERROR;

	child_index = i = 0;
	while ( (info=(GF_ListItemBox*)gf_list_enum(ilst->child_boxes, &i))) {
		GF_DataBox *data_box = NULL;
		if (gf_itags_find_by_itag(info->type)<0) {
			if (info->type==GF_ISOM_BOX_TYPE_UNKNOWN) {
				data_box = (GF_DataBox *) gf_isom_box_find_child(info->child_boxes, GF_ISOM_BOX_TYPE_DATA);
				if (!data_box) continue;
				tag_val = ((GF_UnknownBox *)info)->original_4cc;
			}
		} else {
			data_box = info->data;
			tag_val = info->type;
		}
		if (child_index==idx) {
			dbox = data_box;
			break;
		}
		child_index++;
	}
	if (!dbox) return GF_URL_ERROR;

	*out_flags = dbox->flags;
	*out_tag = tag_val;
	if (!dbox->data) {
		*data = NULL;
		*data_len = 1;
		return GF_OK;
	}

	tag_idx = gf_itags_find_by_itag(info->type);
	if (tag_idx<0) {
		*data = dbox->data;
		*data_len = dbox->dataSize;
		return GF_OK;
	}

	if ((tag_val == GF_ISOM_ITUNE_GENRE) && (dbox->flags == 0) && (dbox->dataSize>=2)) {
		u32 int_val = dbox->data[0];
		int_val <<= 8;
		int_val |= dbox->data[1];
		*data = NULL;
		*data_len = 0;
		*out_int_val = int_val;
		return GF_OK;
	}

	itype = gf_itags_get_type((u32) tag_idx);
	switch (itype) {
	case GF_ITAG_BOOL:
	case GF_ITAG_INT8:
		if (dbox->dataSize) *out_int_val = dbox->data[0];
		break;
	case GF_ITAG_INT16:
		if (dbox->dataSize>1) {
			u16 v = dbox->data[0];
			v<<=8;
			v |= dbox->data[1];
			*out_int_val = v;
		}
		break;
	case GF_ITAG_INT32:
		if (dbox->dataSize>3) {
			u32 v = dbox->data[0];
			v<<=8;
			v |= dbox->data[1];
			v<<=8;
			v |= dbox->data[2];
			v<<=8;
			v |= dbox->data[3];
			*out_int_val = v;
		}
		break;
	case GF_ITAG_INT64:
		if (dbox->dataSize>3) {
			u64 v = dbox->data[0];
			v<<=8;
			v |= dbox->data[1];
			v<<=8;
			v |= dbox->data[2];
			v<<=8;
			v |= dbox->data[3];
			v<<=8;
			v |= dbox->data[4];
			v<<=8;
			v |= dbox->data[5];
			v<<=8;
			v |= dbox->data[6];
			v<<=8;
			v |= dbox->data[7];
			*out_int_val = v;
		}
		break;
	case GF_ITAG_FRAC6:
	case GF_ITAG_FRAC8:
		if (dbox->dataSize>3) {
			u32 v = dbox->data[2];
			v<<=8;
			v |= dbox->data[3];
			*out_int_val = v;
			v = dbox->data[4];
			v<<=8;
			v |= dbox->data[5];
			*out_int_val2 = v;
		}
		break;
	default:
		*data = dbox->data;
		*data_len = dbox->dataSize;
		break;
	}
	return GF_OK;
}