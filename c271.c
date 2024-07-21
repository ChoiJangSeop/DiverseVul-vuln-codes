static int ima_adpcm_decode_block (ima_adpcm_data *ima, uint8_t *encoded,
	int16_t *decoded)
{
	int outputLength;

	struct adpcm_state state;

	outputLength = ima->samplesPerBlock * sizeof (int16_t) *
		ima->track->f.channelCount;

	state.valprev = (encoded[1]<<8) | encoded[0];
	if (encoded[1] & 0x80)
		state.valprev -= 0x10000;

	state.index = encoded[2];

	*decoded++ = state.valprev;

	encoded += 4;

	_af_adpcm_decoder(encoded, decoded, ima->samplesPerBlock - 1, &state);

	return outputLength;
}