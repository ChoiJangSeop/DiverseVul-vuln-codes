
static JSValue js_sys_basecode_ex(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, Bool is_dec, Bool is_16)
{
	u32 out_size=0;
	u8 *out_ptr = NULL;
	JSValue res;
	GF_Err e;
	if (!argc) return GF_JS_EXCEPTION(ctx);

	if (is_dec) {
		u32 len;
		const char *str = JS_ToCString(ctx, argv[0]);
		if (!str) return GF_JS_EXCEPTION(ctx);
		len = (u32) strlen(str);
		out_ptr = gf_malloc(sizeof(u8) * len);
		if (!out_ptr) {
			e = GF_OUT_OF_MEM;
		} else if (is_16) {
			out_size = gf_base16_decode((u8*) str, (u32) len, out_ptr, len);
			e = out_size ? GF_OK : GF_NON_COMPLIANT_BITSTREAM;
		} else {
			out_size = gf_base64_decode((u8*) str, (u32) len, out_ptr, len);
			e = out_size ? GF_OK : GF_NON_COMPLIANT_BITSTREAM;
		}
		JS_FreeCString(ctx, str);
	} else {
		const u8 *data;
		size_t data_size;
		data = JS_GetArrayBuffer(ctx, &data_size, argv[0] );
		if (!data) return GF_JS_EXCEPTION(ctx);
		out_ptr = gf_malloc(sizeof(u8) * (1 + data_size * 2) );
		if (!out_ptr) {
			e = GF_OUT_OF_MEM;
		} else if (is_16) {
			out_size = gf_base16_encode((u8*) data, (u32) data_size, out_ptr, 1 + (u32) data_size * 2);
			e = out_size ? GF_OK : GF_NON_COMPLIANT_BITSTREAM;
		} else {
			out_size = gf_base64_encode((u8*) data, (u32) data_size, out_ptr, 1 + (u32) data_size * 2);
			e = out_size ? GF_OK : GF_NON_COMPLIANT_BITSTREAM;
		}
	}

	if (e) return js_throw_err(ctx, e);
	if (is_dec) {
		res = JS_NewArrayBuffer(ctx, out_ptr, out_size, js_gpac_free, NULL, 0);
	} else {
		out_ptr[out_size] = 0;
		res = JS_NewString(ctx, out_ptr);
		gf_free(out_ptr);
	}
	return res;