static void naludmx_set_dolby_vision(GF_NALUDmxCtx *ctx)
{
	u8 dv_cfg[24];

	switch (ctx->dv_mode) {
	case DVMODE_NONE:
	case DVMODE_CLEAN:
		return;
	//auto mode, wait until we have RPU or EL to signal profile
	case DVMODE_AUTO:
		if (!ctx->nb_dv_rpu && !ctx->nb_dv_el) return;
		break;
	}

	u32 dv_level = gf_dolby_vision_level(ctx->width, ctx->height, ctx->cur_fps.num, ctx->cur_fps.den, ctx->codecid);

	if (ctx->dv_profile==8) {
		if (!ctx->dv_compatid) {
			GF_LOG(GF_LOG_WARNING, GF_LOG_MEDIA, ("[%s] DV profile 8 used but dv_compatid not set, defaulting to bt709 (=2)\n", ctx->log_name));
			ctx->dv_compatid = 2;
		}
	}

	memset(dv_cfg, 0, sizeof(u8)*24);
	GF_BitStream *bs = gf_bs_new(dv_cfg, 24, GF_BITSTREAM_WRITE);
	gf_bs_write_u8(bs, 1); //version major
	gf_bs_write_u8(bs, 0); //version minor
	gf_bs_write_int(bs, ctx->dv_profile, 7);
	gf_bs_write_int(bs, dv_level, 6);
	gf_bs_write_int(bs, ctx->nb_dv_rpu ? 1 : 0, 1); //rpu present
	gf_bs_write_int(bs, ctx->nb_dv_el ? 1 : 0, 1); //el present
	gf_bs_write_int(bs, 1, 1); //bl_present_flag always true, we don't split streams
	gf_bs_write_int(bs, ctx->dv_compatid, 4);
	//the rest is zero-reserved
	gf_bs_write_int(bs, 0, 28);
	gf_bs_write_u32(bs, 0);
	gf_bs_write_u32(bs, 0);
	gf_bs_write_u32(bs, 0);
	gf_bs_write_u32(bs, 0);
	gf_bs_del(bs);
	gf_filter_pid_set_property(ctx->opid, GF_PROP_PID_DOLBY_VISION, &PROP_DATA(dv_cfg, 24));
}