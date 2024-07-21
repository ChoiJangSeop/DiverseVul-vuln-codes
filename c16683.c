static int aesni_cbc_hmac_sha1_cipher(EVP_CIPHER_CTX *ctx, unsigned char *out,
                                      const unsigned char *in, size_t len)
{
    EVP_AES_HMAC_SHA1 *key = data(ctx);
    unsigned int l;
    size_t plen = key->payload_length, iv = 0, /* explicit IV in TLS 1.1 and
                                                * later */
        sha_off = 0;
#  if defined(STITCHED_CALL)
    size_t aes_off = 0, blocks;

    sha_off = SHA_CBLOCK - key->md.num;
#  endif

    key->payload_length = NO_PAYLOAD_LENGTH;

    if (len % AES_BLOCK_SIZE)
        return 0;

    if (ctx->encrypt) {
        if (plen == NO_PAYLOAD_LENGTH)
            plen = len;
        else if (len !=
                 ((plen + SHA_DIGEST_LENGTH +
                   AES_BLOCK_SIZE) & -AES_BLOCK_SIZE))
            return 0;
        else if (key->aux.tls_ver >= TLS1_1_VERSION)
            iv = AES_BLOCK_SIZE;

#  if defined(STITCHED_CALL)
        if (plen > (sha_off + iv)
            && (blocks = (plen - (sha_off + iv)) / SHA_CBLOCK)) {
            SHA1_Update(&key->md, in + iv, sha_off);

            aesni_cbc_sha1_enc(in, out, blocks, &key->ks,
                               ctx->iv, &key->md, in + iv + sha_off);
            blocks *= SHA_CBLOCK;
            aes_off += blocks;
            sha_off += blocks;
            key->md.Nh += blocks >> 29;
            key->md.Nl += blocks <<= 3;
            if (key->md.Nl < (unsigned int)blocks)
                key->md.Nh++;
        } else {
            sha_off = 0;
        }
#  endif
        sha_off += iv;
        SHA1_Update(&key->md, in + sha_off, plen - sha_off);

        if (plen != len) {      /* "TLS" mode of operation */
            if (in != out)
                memcpy(out + aes_off, in + aes_off, plen - aes_off);

            /* calculate HMAC and append it to payload */
            SHA1_Final(out + plen, &key->md);
            key->md = key->tail;
            SHA1_Update(&key->md, out + plen, SHA_DIGEST_LENGTH);
            SHA1_Final(out + plen, &key->md);

            /* pad the payload|hmac */
            plen += SHA_DIGEST_LENGTH;
            for (l = len - plen - 1; plen < len; plen++)
                out[plen] = l;
            /* encrypt HMAC|padding at once */
            aesni_cbc_encrypt(out + aes_off, out + aes_off, len - aes_off,
                              &key->ks, ctx->iv, 1);
        } else {
            aesni_cbc_encrypt(in + aes_off, out + aes_off, len - aes_off,
                              &key->ks, ctx->iv, 1);
        }
    } else {
        union {
            unsigned int u[SHA_DIGEST_LENGTH / sizeof(unsigned int)];
            unsigned char c[32 + SHA_DIGEST_LENGTH];
        } mac, *pmac;

        /* arrange cache line alignment */
        pmac = (void *)(((size_t)mac.c + 31) & ((size_t)0 - 32));

        if (plen != NO_PAYLOAD_LENGTH) { /* "TLS" mode of operation */
            size_t inp_len, mask, j, i;
            unsigned int res, maxpad, pad, bitlen;
            int ret = 1;
            union {
                unsigned int u[SHA_LBLOCK];
                unsigned char c[SHA_CBLOCK];
            } *data = (void *)key->md.data;
#  if defined(STITCHED_DECRYPT_CALL)
            unsigned char tail_iv[AES_BLOCK_SIZE];
            int stitch = 0;
#  endif

            if ((key->aux.tls_aad[plen - 4] << 8 | key->aux.tls_aad[plen - 3])
                >= TLS1_1_VERSION) {
                if (len < (AES_BLOCK_SIZE + SHA_DIGEST_LENGTH + 1))
                    return 0;

                /* omit explicit iv */
                memcpy(ctx->iv, in, AES_BLOCK_SIZE);
                in += AES_BLOCK_SIZE;
                out += AES_BLOCK_SIZE;
                len -= AES_BLOCK_SIZE;
            } else if (len < (SHA_DIGEST_LENGTH + 1))
                return 0;

#  if defined(STITCHED_DECRYPT_CALL)
            if (len >= 1024 && ctx->key_len == 32) {
                /* decrypt last block */
                memcpy(tail_iv, in + len - 2 * AES_BLOCK_SIZE,
                       AES_BLOCK_SIZE);
                aesni_cbc_encrypt(in + len - AES_BLOCK_SIZE,
                                  out + len - AES_BLOCK_SIZE, AES_BLOCK_SIZE,
                                  &key->ks, tail_iv, 0);
                stitch = 1;
            } else
#  endif
                /* decrypt HMAC|padding at once */
                aesni_cbc_encrypt(in, out, len, &key->ks, ctx->iv, 0);

            /* figure out payload length */
            pad = out[len - 1];
            maxpad = len - (SHA_DIGEST_LENGTH + 1);
            maxpad |= (255 - maxpad) >> (sizeof(maxpad) * 8 - 8);
            maxpad &= 255;

            inp_len = len - (SHA_DIGEST_LENGTH + pad + 1);
            mask = (0 - ((inp_len - len) >> (sizeof(inp_len) * 8 - 1)));
            inp_len &= mask;
            ret &= (int)mask;

            key->aux.tls_aad[plen - 2] = inp_len >> 8;
            key->aux.tls_aad[plen - 1] = inp_len;

            /* calculate HMAC */
            key->md = key->head;
            SHA1_Update(&key->md, key->aux.tls_aad, plen);

#  if defined(STITCHED_DECRYPT_CALL)
            if (stitch) {
                blocks = (len - (256 + 32 + SHA_CBLOCK)) / SHA_CBLOCK;
                aes_off = len - AES_BLOCK_SIZE - blocks * SHA_CBLOCK;
                sha_off = SHA_CBLOCK - plen;

                aesni_cbc_encrypt(in, out, aes_off, &key->ks, ctx->iv, 0);

                SHA1_Update(&key->md, out, sha_off);
                aesni256_cbc_sha1_dec(in + aes_off,
                                      out + aes_off, blocks, &key->ks,
                                      ctx->iv, &key->md, out + sha_off);

                sha_off += blocks *= SHA_CBLOCK;
                out += sha_off;
                len -= sha_off;
                inp_len -= sha_off;

                key->md.Nl += (blocks << 3); /* at most 18 bits */
                memcpy(ctx->iv, tail_iv, AES_BLOCK_SIZE);
            }
#  endif

#  if 1
            len -= SHA_DIGEST_LENGTH; /* amend mac */
            if (len >= (256 + SHA_CBLOCK)) {
                j = (len - (256 + SHA_CBLOCK)) & (0 - SHA_CBLOCK);
                j += SHA_CBLOCK - key->md.num;
                SHA1_Update(&key->md, out, j);
                out += j;
                len -= j;
                inp_len -= j;
            }

            /* but pretend as if we hashed padded payload */
            bitlen = key->md.Nl + (inp_len << 3); /* at most 18 bits */
#   ifdef BSWAP4
            bitlen = BSWAP4(bitlen);
#   else
            mac.c[0] = 0;
            mac.c[1] = (unsigned char)(bitlen >> 16);
            mac.c[2] = (unsigned char)(bitlen >> 8);
            mac.c[3] = (unsigned char)bitlen;
            bitlen = mac.u[0];
#   endif

            pmac->u[0] = 0;
            pmac->u[1] = 0;
            pmac->u[2] = 0;
            pmac->u[3] = 0;
            pmac->u[4] = 0;

            for (res = key->md.num, j = 0; j < len; j++) {
                size_t c = out[j];
                mask = (j - inp_len) >> (sizeof(j) * 8 - 8);
                c &= mask;
                c |= 0x80 & ~mask & ~((inp_len - j) >> (sizeof(j) * 8 - 8));
                data->c[res++] = (unsigned char)c;

                if (res != SHA_CBLOCK)
                    continue;

                /* j is not incremented yet */
                mask = 0 - ((inp_len + 7 - j) >> (sizeof(j) * 8 - 1));
                data->u[SHA_LBLOCK - 1] |= bitlen & mask;
                sha1_block_data_order(&key->md, data, 1);
                mask &= 0 - ((j - inp_len - 72) >> (sizeof(j) * 8 - 1));
                pmac->u[0] |= key->md.h0 & mask;
                pmac->u[1] |= key->md.h1 & mask;
                pmac->u[2] |= key->md.h2 & mask;
                pmac->u[3] |= key->md.h3 & mask;
                pmac->u[4] |= key->md.h4 & mask;
                res = 0;
            }

            for (i = res; i < SHA_CBLOCK; i++, j++)
                data->c[i] = 0;

            if (res > SHA_CBLOCK - 8) {
                mask = 0 - ((inp_len + 8 - j) >> (sizeof(j) * 8 - 1));
                data->u[SHA_LBLOCK - 1] |= bitlen & mask;
                sha1_block_data_order(&key->md, data, 1);
                mask &= 0 - ((j - inp_len - 73) >> (sizeof(j) * 8 - 1));
                pmac->u[0] |= key->md.h0 & mask;
                pmac->u[1] |= key->md.h1 & mask;
                pmac->u[2] |= key->md.h2 & mask;
                pmac->u[3] |= key->md.h3 & mask;
                pmac->u[4] |= key->md.h4 & mask;

                memset(data, 0, SHA_CBLOCK);
                j += 64;
            }
            data->u[SHA_LBLOCK - 1] = bitlen;
            sha1_block_data_order(&key->md, data, 1);
            mask = 0 - ((j - inp_len - 73) >> (sizeof(j) * 8 - 1));
            pmac->u[0] |= key->md.h0 & mask;
            pmac->u[1] |= key->md.h1 & mask;
            pmac->u[2] |= key->md.h2 & mask;
            pmac->u[3] |= key->md.h3 & mask;
            pmac->u[4] |= key->md.h4 & mask;

#   ifdef BSWAP4
            pmac->u[0] = BSWAP4(pmac->u[0]);
            pmac->u[1] = BSWAP4(pmac->u[1]);
            pmac->u[2] = BSWAP4(pmac->u[2]);
            pmac->u[3] = BSWAP4(pmac->u[3]);
            pmac->u[4] = BSWAP4(pmac->u[4]);
#   else
            for (i = 0; i < 5; i++) {
                res = pmac->u[i];
                pmac->c[4 * i + 0] = (unsigned char)(res >> 24);
                pmac->c[4 * i + 1] = (unsigned char)(res >> 16);
                pmac->c[4 * i + 2] = (unsigned char)(res >> 8);
                pmac->c[4 * i + 3] = (unsigned char)res;
            }
#   endif
            len += SHA_DIGEST_LENGTH;
#  else
            SHA1_Update(&key->md, out, inp_len);
            res = key->md.num;
            SHA1_Final(pmac->c, &key->md);

            {
                unsigned int inp_blocks, pad_blocks;

                /* but pretend as if we hashed padded payload */
                inp_blocks =
                    1 + ((SHA_CBLOCK - 9 - res) >> (sizeof(res) * 8 - 1));
                res += (unsigned int)(len - inp_len);
                pad_blocks = res / SHA_CBLOCK;
                res %= SHA_CBLOCK;
                pad_blocks +=
                    1 + ((SHA_CBLOCK - 9 - res) >> (sizeof(res) * 8 - 1));
                for (; inp_blocks < pad_blocks; inp_blocks++)
                    sha1_block_data_order(&key->md, data, 1);
            }
#  endif
            key->md = key->tail;
            SHA1_Update(&key->md, pmac->c, SHA_DIGEST_LENGTH);
            SHA1_Final(pmac->c, &key->md);

            /* verify HMAC */
            out += inp_len;
            len -= inp_len;
#  if 1
            {
                unsigned char *p = out + len - 1 - maxpad - SHA_DIGEST_LENGTH;
                size_t off = out - p;
                unsigned int c, cmask;

                maxpad += SHA_DIGEST_LENGTH;
                for (res = 0, i = 0, j = 0; j < maxpad; j++) {
                    c = p[j];
                    cmask =
                        ((int)(j - off - SHA_DIGEST_LENGTH)) >> (sizeof(int) *
                                                                 8 - 1);
                    res |= (c ^ pad) & ~cmask; /* ... and padding */
                    cmask &= ((int)(off - 1 - j)) >> (sizeof(int) * 8 - 1);
                    res |= (c ^ pmac->c[i]) & cmask;
                    i += 1 & cmask;
                }
                maxpad -= SHA_DIGEST_LENGTH;

                res = 0 - ((0 - res) >> (sizeof(res) * 8 - 1));
                ret &= (int)~res;
            }
#  else
            for (res = 0, i = 0; i < SHA_DIGEST_LENGTH; i++)
                res |= out[i] ^ pmac->c[i];
            res = 0 - ((0 - res) >> (sizeof(res) * 8 - 1));
            ret &= (int)~res;

            /* verify padding */
            pad = (pad & ~res) | (maxpad & res);
            out = out + len - 1 - pad;
            for (res = 0, i = 0; i < pad; i++)
                res |= out[i] ^ pad;

            res = (0 - res) >> (sizeof(res) * 8 - 1);
            ret &= (int)~res;
#  endif
            return ret;
        } else {
#  if defined(STITCHED_DECRYPT_CALL)
            if (len >= 1024 && ctx->key_len == 32) {
                if (sha_off %= SHA_CBLOCK)
                    blocks = (len - 3 * SHA_CBLOCK) / SHA_CBLOCK;
                else
                    blocks = (len - 2 * SHA_CBLOCK) / SHA_CBLOCK;
                aes_off = len - blocks * SHA_CBLOCK;

                aesni_cbc_encrypt(in, out, aes_off, &key->ks, ctx->iv, 0);
                SHA1_Update(&key->md, out, sha_off);
                aesni256_cbc_sha1_dec(in + aes_off,
                                      out + aes_off, blocks, &key->ks,
                                      ctx->iv, &key->md, out + sha_off);

                sha_off += blocks *= SHA_CBLOCK;
                out += sha_off;
                len -= sha_off;

                key->md.Nh += blocks >> 29;
                key->md.Nl += blocks <<= 3;
                if (key->md.Nl < (unsigned int)blocks)
                    key->md.Nh++;
            } else
#  endif
                /* decrypt HMAC|padding at once */
                aesni_cbc_encrypt(in, out, len, &key->ks, ctx->iv, 0);

            SHA1_Update(&key->md, out, len);
        }
    }

    return 1;
}