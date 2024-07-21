_AFmoduleinst _af_ms_adpcm_init_decompress (_Track *track, AFvirtualfile *fh,
	bool seekok, bool headerless, AFframecount *chunkframes)
{
	_AFmoduleinst	ret = _AFnewmodinst(&ms_adpcm_decompress);
	ms_adpcm_data	*d;
	AUpvlist	pv;
	long		l;
	void		*v;

	assert(af_ftell(fh) == track->fpos_first_frame);

	d = (ms_adpcm_data *) _af_malloc(sizeof (ms_adpcm_data));

	d->track = track;
	d->fh = fh;

	d->track->frames2ignore = 0;
	d->track->fpos_next_frame = d->track->fpos_first_frame;

	pv = d->track->f.compressionParams;
	if (_af_pv_getlong(pv, _AF_MS_ADPCM_NUM_COEFFICIENTS, &l))
		d->numCoefficients = l;
	else
		_af_error(AF_BAD_CODEC_CONFIG, "number of coefficients not set");

	if (_af_pv_getptr(pv, _AF_MS_ADPCM_COEFFICIENTS, &v))
		memcpy(d->coefficients, v, sizeof (int16_t) * 256 * 2);
	else
		_af_error(AF_BAD_CODEC_CONFIG, "coefficient array not set");

	if (_af_pv_getlong(pv, _AF_SAMPLES_PER_BLOCK, &l))
		d->samplesPerBlock = l;
	else
		_af_error(AF_BAD_CODEC_CONFIG, "samples per block not set");

	if (_af_pv_getlong(pv, _AF_BLOCK_SIZE, &l))
		d->blockAlign = l;
	else
		_af_error(AF_BAD_CODEC_CONFIG, "block size not set");

	*chunkframes = d->samplesPerBlock / d->track->f.channelCount;

	ret.modspec = d;
	return ret;
}