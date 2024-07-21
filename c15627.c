static int pad_pkcs1(bn_t m, int *p_len, int m_len, int k_len, int operation) {
	uint8_t *id, pad = 0;
	int len, result = RLC_OK;
	bn_t t;

	bn_null(t);

	RLC_TRY {
		bn_new(t);

		switch (operation) {
			case RSA_ENC:
				/* EB = 00 | 02 | PS | 00 | D. */
				bn_zero(m);
				bn_lsh(m, m, 8);
				bn_add_dig(m, m, RSA_PUB);

				*p_len = k_len - 3 - m_len;
				for (int i = 0; i < *p_len; i++) {
					bn_lsh(m, m, 8);
					do {
						rand_bytes(&pad, 1);
					} while (pad == 0);
					bn_add_dig(m, m, pad);
				}
				bn_lsh(m, m, 8);
				bn_add_dig(m, m, 0);
				/* Make room for the real message. */
				bn_lsh(m, m, m_len * 8);
				break;
			case RSA_DEC:
				m_len = k_len - 1;
				bn_rsh(t, m, 8 * m_len);
				if (!bn_is_zero(t)) {
					result = RLC_ERR;
				}

				*p_len = m_len;
				m_len--;
				bn_rsh(t, m, 8 * m_len);
				pad = (uint8_t)t->dp[0];
				if (pad != RSA_PUB) {
					result = RLC_ERR;
				}
				do {
					m_len--;
					bn_rsh(t, m, 8 * m_len);
					pad = (uint8_t)t->dp[0];
				} while (pad != 0 && m_len > 0);
				/* Remove padding and trailing zero. */
				*p_len -= (m_len - 1);
				bn_mod_2b(m, m, (k_len - *p_len) * 8);
				break;
			case RSA_SIG:
				/* EB = 00 | 01 | PS | 00 | D. */
				id = hash_id(MD_MAP, &len);
				bn_zero(m);
				bn_lsh(m, m, 8);
				bn_add_dig(m, m, RSA_PRV);

				*p_len = k_len - 3 - m_len - len;
				for (int i = 0; i < *p_len; i++) {
					bn_lsh(m, m, 8);
					bn_add_dig(m, m, RSA_PAD);
				}
				bn_lsh(m, m, 8);
				bn_add_dig(m, m, 0);
				bn_lsh(m, m, 8 * len);
				bn_read_bin(t, id, len);
				bn_add(m, m, t);
				/* Make room for the real message. */
				bn_lsh(m, m, m_len * 8);
				break;
			case RSA_SIG_HASH:
				/* EB = 00 | 01 | PS | 00 | D. */
				bn_zero(m);
				bn_lsh(m, m, 8);
				bn_add_dig(m, m, RSA_PRV);

				*p_len = k_len - 3 - m_len;
				for (int i = 0; i < *p_len; i++) {
					bn_lsh(m, m, 8);
					bn_add_dig(m, m, RSA_PAD);
				}
				bn_lsh(m, m, 8);
				bn_add_dig(m, m, 0);
				/* Make room for the real message. */
				bn_lsh(m, m, m_len * 8);
				break;
			case RSA_VER:
				m_len = k_len - 1;
				bn_rsh(t, m, 8 * m_len);
				if (!bn_is_zero(t)) {
					result = RLC_ERR;
				}
				m_len--;
				bn_rsh(t, m, 8 * m_len);
				pad = (uint8_t)t->dp[0];
				if (pad != RSA_PRV) {
					result = RLC_ERR;
				}
				do {
					m_len--;
					bn_rsh(t, m, 8 * m_len);
					pad = (uint8_t)t->dp[0];
				} while (pad != 0 && m_len > 0);
				if (m_len == 0) {
					result = RLC_ERR;
				}
				/* Remove padding and trailing zero. */
				id = hash_id(MD_MAP, &len);
				m_len -= len;

				bn_rsh(t, m, m_len * 8);
				int r = 0;
				for (int i = 0; i < len; i++) {
					pad = (uint8_t)t->dp[0];
					r |= pad - id[len - i - 1];
					bn_rsh(t, t, 8);
				}
				*p_len = k_len - m_len;
				bn_mod_2b(m, m, m_len * 8);
				result = (r == 0 ? RLC_OK : RLC_ERR);
				break;
			case RSA_VER_HASH:
				m_len = k_len - 1;
				bn_rsh(t, m, 8 * m_len);
				if (!bn_is_zero(t)) {
					result = RLC_ERR;
				}
				m_len--;
				bn_rsh(t, m, 8 * m_len);
				pad = (uint8_t)t->dp[0];
				if (pad != RSA_PRV) {
					result = RLC_ERR;
				}
				do {
					m_len--;
					bn_rsh(t, m, 8 * m_len);
					pad = (uint8_t)t->dp[0];
				} while (pad != 0 && m_len > 0);
				if (m_len == 0) {
					result = RLC_ERR;
				}
				/* Remove padding and trailing zero. */
				*p_len = k_len - m_len;
				bn_mod_2b(m, m, m_len * 8);
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