jpc_streamlist_t *jpc_streamlist_create()
{
	jpc_streamlist_t *streamlist;
	int i;

	if (!(streamlist = jas_malloc(sizeof(jpc_streamlist_t)))) {
		return 0;
	}
	streamlist->numstreams = 0;
	streamlist->maxstreams = 100;
	if (!(streamlist->streams = jas_malloc(streamlist->maxstreams *
	  sizeof(jas_stream_t *)))) {
		jas_free(streamlist);
		return 0;
	}
	for (i = 0; i < streamlist->maxstreams; ++i) {
		streamlist->streams[i] = 0;
	}
	return streamlist;
}