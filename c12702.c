char *gf_seng_get_base64_iod(GF_SceneEngine *seng)
{
	u32 size, size64;
	u8 *buffer, *buf64;
	u32 i=0;
	GF_StreamContext*sc = NULL;

	if (!seng->ctx->root_od) return NULL;

	while ((sc = (GF_StreamContext*)gf_list_enum(seng->ctx->streams, &i))) {
		if ((sc->streamType == GF_STREAM_SCENE) && (sc->codec_id != GF_CODECID_DIMS))
			break;
	}
	if (!sc) return NULL;

	size = 0;
	gf_odf_desc_write((GF_Descriptor *) seng->ctx->root_od, &buffer, &size);
	buf64 = gf_malloc(size*2);
	size64 = gf_base64_encode( buffer, size, buf64, size*2);
	buf64[size64] = 0;
	gf_free(buffer);
	return buf64;
}