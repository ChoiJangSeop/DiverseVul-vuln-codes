static gint conv_euctojis(gchar *outbuf, gint outlen, const gchar *inbuf)
{
	const guchar *in = inbuf;
	guchar *out = outbuf;
	JISState state = JIS_ASCII;

	while (*in != '\0') {
		if (IS_ASCII(*in)) {
			K_OUT();
			*out++ = *in++;
		} else if (iseuckanji(*in)) {
			if (iseuckanji(*(in + 1))) {
				K_IN();
				*out++ = *in++ & 0x7f;
				*out++ = *in++ & 0x7f;
			} else {
				K_OUT();
				*out++ = SUBST_CHAR;
				in++;
				if (*in != '\0' && !IS_ASCII(*in)) {
					*out++ = SUBST_CHAR;
					in++;
				}
			}
		} else if (iseuchwkana1(*in)) {
			if (iseuchwkana2(*(in + 1))) {
				if (prefs_common.allow_jisx0201_kana) {
					HW_IN();
					in++;
					*out++ = *in++ & 0x7f;
				} else {
					guchar jis_ch[2];
					gint len;

					if (iseuchwkana1(*(in + 2)) &&
					    iseuchwkana2(*(in + 3)))
						len = conv_jis_hantozen
							(jis_ch,
							 *(in + 1), *(in + 3));
					else
						len = conv_jis_hantozen
							(jis_ch,
							 *(in + 1), '\0');
					if (len == 0)
						in += 2;
					else {
						K_IN();
						in += len * 2;
						*out++ = jis_ch[0];
						*out++ = jis_ch[1];
					}
				}
			} else {
				K_OUT();
				in++;
				if (*in != '\0' && !IS_ASCII(*in)) {
					*out++ = SUBST_CHAR;
					in++;
				}
			}
		} else if (iseucaux(*in)) {
			in++;
			if (iseuckanji(*in) && iseuckanji(*(in + 1))) {
				AUX_IN();
				*out++ = *in++ & 0x7f;
				*out++ = *in++ & 0x7f;
			} else {
				K_OUT();
				if (*in != '\0' && !IS_ASCII(*in)) {
					*out++ = SUBST_CHAR;
					in++;
					if (*in != '\0' && !IS_ASCII(*in)) {
						*out++ = SUBST_CHAR;
						in++;
					}
				}
			}
		} else {
			K_OUT();
			*out++ = SUBST_CHAR;
			in++;
		}
	}

	K_OUT();
	*out = '\0';
	return 0;
}