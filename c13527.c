GF_Box *encs_box_new()
{
	ISOM_DECL_BOX_ALLOC(GF_MPEGSampleEntryBox, GF_ISOM_BOX_TYPE_ENCS);
	gf_isom_sample_entry_init((GF_SampleEntryBox*)tmp);
	tmp->internal_type = GF_ISOM_SAMPLE_ENTRY_MP4S;
	return (GF_Box *)tmp;
}