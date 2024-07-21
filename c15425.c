GF_Err stbl_AppendTime(GF_SampleTableBox *stbl, u32 duration, u32 nb_pack)
{
	GF_TimeToSampleBox *stts = stbl->TimeToSample;

	if (!nb_pack) nb_pack = 1;
	if (stts->nb_entries) {
		if (stts->entries[stts->nb_entries-1].sampleDelta == duration) {
			stts->entries[stts->nb_entries-1].sampleCount += nb_pack;
			return GF_OK;
		}
	}
	if (stts->nb_entries==stts->alloc_size) {
		ALLOC_INC(stts->alloc_size);
		stts->entries = gf_realloc(stts->entries, sizeof(GF_SttsEntry)*stts->alloc_size);
		if (!stts->entries) return GF_OUT_OF_MEM;
		memset(&stts->entries[stts->nb_entries], 0, sizeof(GF_SttsEntry)*(stts->alloc_size-stts->nb_entries) );
	}
	stts->entries[stts->nb_entries].sampleCount = nb_pack;
	stts->entries[stts->nb_entries].sampleDelta = duration;
	stts->nb_entries++;
	if (stts->max_ts_delta < duration ) stts->max_ts_delta = duration;
	return GF_OK;
}