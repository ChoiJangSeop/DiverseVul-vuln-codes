GF_Err gf_isom_get_sample_for_movie_time(GF_ISOFile *the_file, u32 trackNumber, u64 movieTime, u32 *StreamDescriptionIndex, GF_ISOSearchMode SearchMode, GF_ISOSample **sample, u32 *sampleNumber, u64 *data_offset)
{
	Double tsscale;
	GF_Err e;
	GF_TrackBox *trak;
	u64 mediaTime, nextMediaTime;
	s64 segStartTime, mediaOffset;
	u32 sampNum;
	u8 useEdit;

	trak = gf_isom_get_track_from_file(the_file, trackNumber);
	if (!trak) return GF_BAD_PARAM;

	//only check duration if initially set - do not check duration as updated after fragment merge since that duration does not take
	//into account tfdt
	if (trak->Header->initial_duration
		&& gf_timestamp_greater(movieTime, trak->Media->mediaHeader->timeScale, trak->Header->initial_duration, trak->moov->mvhd->timeScale)
	) {
		if (sampleNumber) *sampleNumber = 0;
		*StreamDescriptionIndex = 0;
		return GF_EOS;
	}

	//get the media time for this movie time...
	mediaTime = segStartTime = 0;
	*StreamDescriptionIndex = 0;
	nextMediaTime = 0;

	e = GetMediaTime(trak, (SearchMode==GF_ISOM_SEARCH_SYNC_FORWARD) ? GF_TRUE : GF_FALSE, movieTime, &mediaTime, &segStartTime, &mediaOffset, &useEdit, &nextMediaTime);
	if (e) return e;

	/*here we check if we were playing or not and return no sample in normal search modes*/
	if (useEdit && mediaOffset == -1) {
		if ((SearchMode==GF_ISOM_SEARCH_FORWARD) || (SearchMode==GF_ISOM_SEARCH_BACKWARD)) {
			/*get next sample time in MOVIE timescale*/
			if (SearchMode==GF_ISOM_SEARCH_FORWARD)
				e = GetNextMediaTime(trak, movieTime, &mediaTime);
			else
				e = GetPrevMediaTime(trak, movieTime, &mediaTime);
			if (e) return e;
			return gf_isom_get_sample_for_movie_time(the_file, trackNumber, (u32) mediaTime, StreamDescriptionIndex, GF_ISOM_SEARCH_SYNC_FORWARD, sample, sampleNumber, data_offset);
		}
		if (sampleNumber) *sampleNumber = 0;
		if (sample) {
			if (! (*sample)) {
				*sample = gf_isom_sample_new();
				if (! *sample) return GF_OUT_OF_MEM;
			}
			(*sample)->DTS = movieTime;
			(*sample)->dataLength = 0;
			(*sample)->CTS_Offset = 0;
		}
		return GF_OK;
	}
	/*dwell edit in non-sync mode, fetch next/prev sample depending on mode.
	Otherwise return the dwell entry*/
	if (useEdit==2) {
		if ((SearchMode==GF_ISOM_SEARCH_FORWARD) || (SearchMode==GF_ISOM_SEARCH_BACKWARD)) {
			/*get next sample time in MOVIE timescale*/
			if (SearchMode==GF_ISOM_SEARCH_FORWARD)
				e = GetNextMediaTime(trak, movieTime, &mediaTime);
			else
				e = GetPrevMediaTime(trak, movieTime, &mediaTime);
			if (e) return e;
			return gf_isom_get_sample_for_movie_time(the_file, trackNumber, (u32) mediaTime, StreamDescriptionIndex, GF_ISOM_SEARCH_SYNC_FORWARD, sample, sampleNumber, data_offset);
		}
	}

	tsscale = trak->Media->mediaHeader->timeScale;
	tsscale /= trak->moov->mvhd->timeScale;

	//OK, we have a sample so fetch it
	e = gf_isom_get_sample_for_media_time(the_file, trackNumber, mediaTime, StreamDescriptionIndex, SearchMode, sample, &sampNum, data_offset);
	if (e) {
		if (e==GF_EOS) {
#ifndef GPAC_DISABLE_ISOM_FRAGMENTS
			//movie is fragmented and samples not yet received, return EOS
			if (the_file->moov->mvex && !trak->Media->information->sampleTable->SampleSize->sampleCount)
				return e;
#endif

			if (nextMediaTime && (nextMediaTime-1 != movieTime))
				return gf_isom_get_sample_for_movie_time(the_file, trackNumber, nextMediaTime-1, StreamDescriptionIndex, SearchMode, sample, sampleNumber, data_offset);
		}
		return e;
	}

	//OK, now the trick: we have to rebuild the time stamps, according
	//to the media time scale (used by SLConfig) - add the edit start time but stay in
	//the track TS
	if (sample && useEdit) {
		u64 _ts = (u64)(segStartTime * tsscale);

		(*sample)->DTS += _ts;
		/*watchout, the sample fetched may be before the first sample in the edit list (when seeking)*/
		if ( (*sample)->DTS > (u64) mediaOffset) {
			(*sample)->DTS -= (u64) mediaOffset;
		} else {
			(*sample)->DTS = 0;
		}
	}
	if (sampleNumber) *sampleNumber = sampNum;
#ifndef	GPAC_DISABLE_ISOM_FRAGMENTS
	if (sample && (*sample) ) (*sample)->DTS += trak->dts_at_seg_start;
#endif

	return GF_OK;
}