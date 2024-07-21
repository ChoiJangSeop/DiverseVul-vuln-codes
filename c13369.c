find_by_thp(struct tang_keys_info* tki, const char* target)
{
    if (!tki) {
        return NULL;
    }

    json_auto_t* keys = json_deep_copy(tki->m_keys);
    json_array_extend(keys, tki->m_rotated_keys);

    size_t idx;
    json_t* jwk;
    const char** hashes = supported_hashes();
    json_array_foreach(keys, idx, jwk) {
        for (int i = 0; hashes[i]; i++) {
            __attribute__ ((__cleanup__(cleanup_str))) char* thumbprint = jwk_thumbprint(jwk, hashes[i]);
            if (!thumbprint || strcmp(thumbprint, target) != 0) {
                continue;
            }

            if (jwk_valid_for_deriving_keys(jwk)) {
                return json_incref(jwk);
            } else if (jwk_valid_for_signing(jwk)) {
                json_auto_t* sign = json_deep_copy(tki->m_sign);
                if (json_array_append(sign, jwk) == -1) {
                    return NULL;
                }
                json_auto_t* jws = jwk_sign(tki->m_payload, sign);
                if (!jws) {
                    return NULL;
                }
                return json_incref(jws);
            }
        }
    }
    return NULL;
}