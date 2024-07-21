static void ms_adpcm_run_pull (_AFmoduleinst *module)
{
	ms_adpcm_data	*d = (ms_adpcm_data *) module->modspec;
	AFframecount	frames2read = module->outc->nframes;
	AFframecount	nframes = 0;
	int		i, framesPerBlock, blockCount;
	ssize_t		blocksRead, bytesDecoded;

	framesPerBlock = d->samplesPerBlock / d->track->f.channelCount;
	assert(module->outc->nframes % framesPerBlock == 0);
	blockCount = module->outc->nframes / framesPerBlock;

	/* Read the compressed frames. */
	blocksRead = af_fread(module->inc->buf, d->blockAlign, blockCount, d->fh);

	/* Decompress into module->outc. */
	for (i=0; i<blockCount; i++)
	{
		bytesDecoded = ms_adpcm_decode_block(d,
			(uint8_t *) module->inc->buf + i * d->blockAlign,
			(int16_t *) module->outc->buf + i * d->samplesPerBlock);

		nframes += framesPerBlock;
	}

	d->track->nextfframe += nframes;

	if (blocksRead > 0)
		d->track->fpos_next_frame += blocksRead * d->blockAlign;

	assert(af_ftell(d->fh) == d->track->fpos_next_frame);

	/*
		If we got EOF from read, then we return the actual amount read.

		Complain only if there should have been more frames in the file.
	*/

	if (d->track->totalfframes != -1 && nframes != frames2read)
	{
		/* Report error if we haven't already */
		if (d->track->filemodhappy)
		{
			_af_error(AF_BAD_READ,
				"file missing data -- read %d frames, should be %d",
				d->track->nextfframe,
				d->track->totalfframes);
			d->track->filemodhappy = AF_FALSE;
		}
	}

	module->outc->nframes = nframes;
}