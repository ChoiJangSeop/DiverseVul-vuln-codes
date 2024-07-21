decompress(packet_info* pinfo, tvbuff_t* tvb, gint offset, gint length)
{
	if(PINFO_FD_VISITED(pinfo)) {
		const slice_t* saved_data = (slice_t *)p_get_proto_data(wmem_file_scope(), pinfo, proto_blip, 0);
		tvbuff_t* decompressedChild = tvb_new_child_real_data(tvb, (guint8 *)saved_data->buf,
			(gint)saved_data->size, (gint)saved_data->size);
		add_new_data_source(pinfo, decompressedChild, "Decompressed Payload");
		return decompressedChild;
	}

	const guint8* buf = tvb_get_ptr(tvb, offset, length);
	z_stream* decompress_stream = get_decompress_stream(pinfo);
	static Byte trailer[4] = { 0x00, 0x00, 0xff, 0xff };
	if(!decompress_stream->next_out) {
		decompress_stream->zalloc = 0;
		decompress_stream->zfree = 0;
		decompress_stream->opaque = 0;
		int err = inflateInit2(decompress_stream, -MAX_WBITS);
		if(err != Z_OK) {
			decompress_stream->next_out = 0;
			REPORT_DISSECTOR_BUG("Unable to create INFLATE context to decompress messages");
			return NULL;
		}
	}

	Bytef* decompress_buffer = (Bytef*)wmem_alloc(wmem_file_scope(), BLIP_INFLATE_BUFFER_SIZE);
	decompress_stream->next_in = (Bytef*)buf;
	decompress_stream->avail_in = length;
	decompress_stream->next_out = decompress_buffer;
	decompress_stream->avail_out = BLIP_INFLATE_BUFFER_SIZE;
	uLong start = decompress_stream->total_out;
	int err = inflate(decompress_stream, Z_NO_FLUSH);
	if(err < 0) {
		return NULL;
	}

	decompress_stream->next_in = trailer;
	decompress_stream->avail_in = 4;
	err = inflate(decompress_stream, Z_SYNC_FLUSH);
	if(err < 0) {
		return NULL;
	}

	uLong bodyLength = decompress_stream->total_out - start;
	tvbuff_t* decompressedChild = tvb_new_child_real_data(tvb, decompress_buffer, (guint)bodyLength, (gint)bodyLength);
	add_new_data_source(pinfo, decompressedChild, "Decompressed Payload");
	slice_t* data_to_save = wmem_new0(wmem_file_scope(), slice_t);
	data_to_save->size = (size_t)bodyLength;
	data_to_save->buf = decompress_buffer;
	p_add_proto_data(wmem_file_scope(), pinfo, proto_blip, 0, data_to_save);

	return decompressedChild;
}