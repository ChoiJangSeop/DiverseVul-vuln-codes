static void ima_adpcm_reset2 (_AFmoduleinst *i)
{
	ima_adpcm_data	*d = (ima_adpcm_data *) i->modspec;
	int		framesPerBlock;

	framesPerBlock = d->samplesPerBlock / d->track->f.channelCount;

	d->track->fpos_next_frame = d->track->fpos_first_frame +
		d->blockAlign * (d->track->nextfframe / framesPerBlock);
	d->track->frames2ignore += d->framesToIgnore;

	assert(d->track->nextfframe % framesPerBlock == 0);
}