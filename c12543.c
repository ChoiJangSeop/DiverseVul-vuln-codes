static GSList *caps_to_list(uint8_t *data, int size,
				struct avdtp_service_capability **codec,
				gboolean *delay_reporting)
{
	GSList *caps;
	int processed;

	if (delay_reporting)
		*delay_reporting = FALSE;

	for (processed = 0, caps = NULL; processed + 2 <= size;) {
		struct avdtp_service_capability *cap;
		uint8_t length, category;

		category = data[0];
		length = data[1];

		if (processed + 2 + length > size) {
			error("Invalid capability data in getcap resp");
			break;
		}

		cap = g_malloc(sizeof(struct avdtp_service_capability) +
					length);
		memcpy(cap, data, 2 + length);

		processed += 2 + length;
		data += 2 + length;

		caps = g_slist_append(caps, cap);

		if (category == AVDTP_MEDIA_CODEC &&
				length >=
				sizeof(struct avdtp_media_codec_capability))
			*codec = cap;
		else if (category == AVDTP_DELAY_REPORTING && delay_reporting)
			*delay_reporting = TRUE;
	}

	return caps;
}