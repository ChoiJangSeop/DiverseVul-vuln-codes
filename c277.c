static void ms_adpcm_reset1 (_AFmoduleinst *i)
{
	ms_adpcm_data	*d = (ms_adpcm_data *) i->modspec;
	AFframecount	nextTrackFrame;
	int		framesPerBlock;

	framesPerBlock = d->samplesPerBlock / d->track->f.channelCount;

	nextTrackFrame = d->track->nextfframe;
	d->track->nextfframe = (nextTrackFrame / framesPerBlock) *
		framesPerBlock;

	d->framesToIgnore = nextTrackFrame - d->track->nextfframe;
	/* postroll = frames2ignore */
}