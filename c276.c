static int ms_adpcm_decode_block (ms_adpcm_data *msadpcm, uint8_t *encoded,
	int16_t *decoded)
{
	int		i, outputLength, samplesRemaining;
	int		channelCount;
	int16_t		*coefficient[2];
	ms_adpcm_state	decoderState[2];
	ms_adpcm_state	*state[2];

	/* Calculate the number of bytes needed for decoded data. */
	outputLength = msadpcm->samplesPerBlock * sizeof (int16_t) *
		msadpcm->track->f.channelCount;

	channelCount = msadpcm->track->f.channelCount;

	state[0] = &decoderState[0];
	if (channelCount == 2)
		state[1] = &decoderState[1];
	else
		state[1] = &decoderState[0];

	/* Initialize predictor. */
	for (i=0; i<channelCount; i++)
	{
		state[i]->predictor = *encoded++;
		assert(state[i]->predictor < msadpcm->numCoefficients);
	}

	/* Initialize delta. */
	for (i=0; i<channelCount; i++)
	{
		state[i]->delta = (encoded[1]<<8) | encoded[0];
		encoded += sizeof (uint16_t);
	}

	/* Initialize first two samples. */
	for (i=0; i<channelCount; i++)
	{
		state[i]->sample1 = (encoded[1]<<8) | encoded[0];
		encoded += sizeof (uint16_t);
	}

	for (i=0; i<channelCount; i++)
	{
		state[i]->sample2 = (encoded[1]<<8) | encoded[0];
		encoded += sizeof (uint16_t);
	}

	coefficient[0] = msadpcm->coefficients[state[0]->predictor];
	coefficient[1] = msadpcm->coefficients[state[1]->predictor];

	for (i=0; i<channelCount; i++)
		*decoded++ = state[i]->sample2;

	for (i=0; i<channelCount; i++)
		*decoded++ = state[i]->sample1;

	/*
		The first two samples have already been 'decoded' in
		the block header.
	*/
	samplesRemaining = (msadpcm->samplesPerBlock - 2) *
		msadpcm->track->f.channelCount;

	while (samplesRemaining > 0)
	{
		uint8_t code;
		int16_t newSample;

		code = *encoded >> 4;
		newSample = ms_adpcm_decode_sample(state[0], code,
			coefficient[0]);
		*decoded++ = newSample;

		code = *encoded & 0x0f;
		newSample = ms_adpcm_decode_sample(state[1], code,
			coefficient[1]);
		*decoded++ = newSample;

		encoded++;
		samplesRemaining -= 2;
	}

	return outputLength;
}