GF_Err gf_isom_text_get_encoded_tx3g(GF_ISOFile *file, u32 track, u32 sidx, u32 sidx_offset, u8 **tx3g, u32 *tx3g_size)
{
	GF_BitStream *bs;
	GF_TrackBox *trak;
	GF_Tx3gSampleEntryBox *a;

	trak = gf_isom_get_track_from_file(file, track);
	if (!trak) return GF_BAD_PARAM;

	a = (GF_Tx3gSampleEntryBox *) gf_list_get(trak->Media->information->sampleTable->SampleDescription->child_boxes, sidx-1);
	if (!a) return GF_BAD_PARAM;
	if ((a->type != GF_ISOM_BOX_TYPE_TX3G) && (a->type != GF_ISOM_BOX_TYPE_TEXT)) return GF_BAD_PARAM;

	bs = gf_bs_new(NULL, 0, GF_BITSTREAM_WRITE);
	gf_isom_write_tx3g(a, bs, sidx, sidx_offset);
	*tx3g = NULL;
	*tx3g_size = 0;
	gf_bs_get_content(bs, tx3g, tx3g_size);
	gf_bs_del(bs);
	return GF_OK;
}