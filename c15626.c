static int pad_pkcs2(bn_t m, int *p_len, int m_len, int k_len, int operation) {
        uint8_t pad, h1[RLC_MD_LEN], h2[RLC_MD_LEN];
        /* MSVC does not allow dynamic stack arrays */
        uint8_t *mask = RLC_ALLOCA(uint8_t, k_len);
	int result = RLC_OK;
	bn_t t;

	bn_null(t);

	RLC_TRY {
		bn_new(t);

		switch (operation) {
			case RSA_ENC:
				/* DB = lHash | PS | 01 | D. */
				md_map(h1, NULL, 0);
				bn_read_bin(m, h1, RLC_MD_LEN);
				*p_len = k_len - 2 * RLC_MD_LEN - 2 - m_len;
				bn_lsh(m, m, *p_len * 8);
				bn_lsh(m, m, 8);
				bn_add_dig(m, m, 0x01);
				/* Make room for the real message. */
				bn_lsh(m, m, m_len * 8);
				break;
			case RSA_ENC_FIN:
				/* EB = 00 | maskedSeed | maskedDB. */
				rand_bytes(h1, RLC_MD_LEN);
				md_mgf(mask, k_len - RLC_MD_LEN - 1, h1, RLC_MD_LEN);
				bn_read_bin(t, mask, k_len - RLC_MD_LEN - 1);
				for (int i = 0; i < t->used; i++) {
					m->dp[i] ^= t->dp[i];
				}
				bn_write_bin(mask, k_len - RLC_MD_LEN - 1, m);
				md_mgf(h2, RLC_MD_LEN, mask, k_len - RLC_MD_LEN - 1);
				for (int i = 0; i < RLC_MD_LEN; i++) {
					h1[i] ^= h2[i];
				}
				bn_read_bin(t, h1, RLC_MD_LEN);
				bn_lsh(t, t, 8 * (k_len - RLC_MD_LEN - 1));
				bn_add(t, t, m);
				bn_copy(m, t);
				break;
			case RSA_DEC:
				m_len = k_len - 1;
				bn_rsh(t, m, 8 * m_len);
				if (!bn_is_zero(t)) {
					result = RLC_ERR;
				}
				m_len -= RLC_MD_LEN;
				bn_rsh(t, m, 8 * m_len);
				bn_write_bin(h1, RLC_MD_LEN, t);
				bn_mod_2b(m, m, 8 * m_len);
				bn_write_bin(mask, m_len, m);
				md_mgf(h2, RLC_MD_LEN, mask, m_len);
				for (int i = 0; i < RLC_MD_LEN; i++) {
					h1[i] ^= h2[i];
				}
				md_mgf(mask, k_len - RLC_MD_LEN - 1, h1, RLC_MD_LEN);
				bn_read_bin(t, mask, k_len - RLC_MD_LEN - 1);
				for (int i = 0; i < t->used; i++) {
					m->dp[i] ^= t->dp[i];
				}
				m_len -= RLC_MD_LEN;
				bn_rsh(t, m, 8 * m_len);
				bn_write_bin(h2, RLC_MD_LEN, t);
				md_map(h1, NULL, 0);
				pad = 0;
				for (int i = 0; i < RLC_MD_LEN; i++) {
					pad |= h1[i] - h2[i];
				}
				if (result == RLC_OK) {
					result = (pad ? RLC_ERR : RLC_OK);
				}
				bn_mod_2b(m, m, 8 * m_len);
				*p_len = bn_size_bin(m);
				(*p_len)--;
				bn_rsh(t, m, *p_len * 8);
				if (bn_cmp_dig(t, 1) != RLC_EQ) {
					result = RLC_ERR;
				}
				bn_mod_2b(m, m, *p_len * 8);
				*p_len = k_len - *p_len;
				break;
			case RSA_SIG:
			case RSA_SIG_HASH:
				/* M' = 00 00 00 00 00 00 00 00 | H(M). */
				bn_zero(m);
				bn_lsh(m, m, 64);
				/* Make room for the real message. */
				bn_lsh(m, m, RLC_MD_LEN * 8);
				break;
			case RSA_SIG_FIN:
				memset(mask, 0, 8);
				bn_write_bin(mask + 8, RLC_MD_LEN, m);
				md_map(h1, mask, RLC_MD_LEN + 8);
				bn_read_bin(m, h1, RLC_MD_LEN);
				md_mgf(mask, k_len - RLC_MD_LEN - 1, h1, RLC_MD_LEN);
				bn_read_bin(t, mask, k_len - RLC_MD_LEN - 1);
				t->dp[0] ^= 0x01;
				/* m_len is now the size in bits of the modulus. */
				bn_lsh(t, t, 8 * RLC_MD_LEN);
				bn_add(m, t, m);
				bn_lsh(m, m, 8);
				bn_add_dig(m, m, RSA_PSS);
				for (int i = m_len - 1; i < 8 * k_len; i++) {
					bn_set_bit(m, i, 0);
				}
				break;
			case RSA_VER:
			case RSA_VER_HASH:
				bn_mod_2b(t, m, 8);
				if (bn_cmp_dig(t, RSA_PSS) != RLC_EQ) {
					result = RLC_ERR;
				} else {
					for (int i = m_len; i < 8 * k_len; i++) {
						if (bn_get_bit(m, i) != 0) {
							result = RLC_ERR;
						}
					}
					bn_rsh(m, m, 8);
					bn_mod_2b(t, m, 8 * RLC_MD_LEN);
					bn_write_bin(h2, RLC_MD_LEN, t);
					bn_rsh(m, m, 8 * RLC_MD_LEN);
					bn_write_bin(h1, RLC_MD_LEN, t);
					md_mgf(mask, k_len - RLC_MD_LEN - 1, h1, RLC_MD_LEN);
					bn_read_bin(t, mask, k_len - RLC_MD_LEN - 1);
					for (int i = 0; i < t->used; i++) {
						m->dp[i] ^= t->dp[i];
					}
					m->dp[0] ^= 0x01;
					for (int i = m_len - 1; i < 8 * k_len; i++) {
						bn_set_bit(m, i - ((RLC_MD_LEN + 1) * 8), 0);
					}
					if (!bn_is_zero(m)) {
						result = RLC_ERR;
					}
					bn_read_bin(m, h2, RLC_MD_LEN);
					*p_len = k_len - RLC_MD_LEN;
				}
				break;
		}
	}
	RLC_CATCH_ANY {
		result = RLC_ERR;
	}
	RLC_FINALLY {
		bn_free(t);
	}

        RLC_FREE(mask);

	return result;
}