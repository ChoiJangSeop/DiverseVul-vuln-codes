static int pad_basic(bn_t m, int *p_len, int m_len, int k_len, int operation) {
	uint8_t pad = 0;
	int result = RLC_OK;
	bn_t t;

	RLC_TRY {
		bn_null(t);
		bn_new(t);

		switch (operation) {
			case RSA_ENC:
			case RSA_SIG:
			case RSA_SIG_HASH:
				/* EB = 00 | FF | D. */
				bn_zero(m);
				bn_lsh(m, m, 8);
				bn_add_dig(m, m, RSA_PAD);
				/* Make room for the real message. */
				bn_lsh(m, m, m_len * 8);
				break;
			case RSA_DEC:
			case RSA_VER:
			case RSA_VER_HASH:
				/* EB = 00 | FF | D. */
				m_len = k_len - 1;
				bn_rsh(t, m, 8 * m_len);
				if (!bn_is_zero(t)) {
					result = RLC_ERR;
				}
				*p_len = 1;
				do {
					(*p_len)++;
					m_len--;
					bn_rsh(t, m, 8 * m_len);
					pad = (uint8_t)t->dp[0];
				} while (pad == 0 && m_len > 0);
				if (pad != RSA_PAD) {
					result = RLC_ERR;
				}
				bn_mod_2b(m, m, (k_len - *p_len) * 8);
				break;
		}
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(t);
	}
	return result;
}