static wStream* rdg_receive_packet(rdpRdg* rdg)
{
	wStream* s;
	const size_t header = sizeof(RdgPacketHeader);
	size_t packetLength;
	assert(header <= INT_MAX);
	s = Stream_New(NULL, 1024);

	if (!s)
		return NULL;

	if (!rdg_read_all(rdg->tlsOut, Stream_Buffer(s), header))
	{
		Stream_Free(s, TRUE);
		return NULL;
	}

	Stream_Seek(s, 4);
	Stream_Read_UINT32(s, packetLength);

	if ((packetLength > INT_MAX) || !Stream_EnsureCapacity(s, packetLength))
	{
		Stream_Free(s, TRUE);
		return NULL;
	}

	if (!rdg_read_all(rdg->tlsOut, Stream_Buffer(s) + header, (int)packetLength - (int)header))
	{
		Stream_Free(s, TRUE);
		return NULL;
	}

	Stream_SetLength(s, packetLength);
	return s;
}