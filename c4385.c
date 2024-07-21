int read_file(SSL_CTX* ctx, const char* file, int format, CertType type)
{
    int ret = SSL_SUCCESS;

    if (format != SSL_FILETYPE_ASN1 && format != SSL_FILETYPE_PEM)
        return SSL_BAD_FILETYPE;

    if (file == NULL || !file[0])
      return SSL_BAD_FILE;

    FILE* input = fopen(file, "rb");
    if (!input)
        return SSL_BAD_FILE;

    if (type == CA) {
        // may have a bunch of CAs
        x509* ptr;
        while ( (ptr = PemToDer(input, Cert)) )
            ctx->AddCA(ptr);

        if (!feof(input)) {
            fclose(input);
            return SSL_BAD_FILE;
        }
    }
    else {
        x509*& x = (type == Cert) ? ctx->certificate_ : ctx->privateKey_;

        if (format == SSL_FILETYPE_ASN1) {
            fseek(input, 0, SEEK_END);
            long sz = ftell(input);
            rewind(input);
            x = NEW_YS x509(sz); // takes ownership
            size_t bytes = fread(x->use_buffer(), sz, 1, input);
            if (bytes != 1) {
                fclose(input);
                return SSL_BAD_FILE;
            }
        }
        else {
            EncryptedInfo info;
            x = PemToDer(input, type, &info);
            if (!x) {
                fclose(input);
                return SSL_BAD_FILE;
            }
            if (info.set) {
                // decrypt
                char password[80];
                pem_password_cb cb = ctx->GetPasswordCb();
                if (!cb) {
                    fclose(input);
                    return SSL_BAD_FILE;
                }
                int passwordSz = cb(password, sizeof(password), 0,
                                    ctx->GetUserData());
                byte key[AES_256_KEY_SZ];  // max sizes
                byte iv[AES_IV_SZ];
                
                // use file's salt for key derivation, but not real iv
                TaoCrypt::Source source(info.iv, info.ivSz);
                TaoCrypt::HexDecoder dec(source);
                memcpy(info.iv, source.get_buffer(), min((uint)sizeof(info.iv),
                                                         source.size()));
                EVP_BytesToKey(info.name, "MD5", info.iv, (byte*)password,
                               passwordSz, 1, key, iv);

                mySTL::auto_ptr<BulkCipher> cipher;
                if (strncmp(info.name, "DES-CBC", 7) == 0)
                    cipher.reset(NEW_YS DES);
                else if (strncmp(info.name, "DES-EDE3-CBC", 13) == 0)
                    cipher.reset(NEW_YS DES_EDE);
                else if (strncmp(info.name, "AES-128-CBC", 13) == 0)
                    cipher.reset(NEW_YS AES(AES_128_KEY_SZ));
                else if (strncmp(info.name, "AES-192-CBC", 13) == 0)
                    cipher.reset(NEW_YS AES(AES_192_KEY_SZ));
                else if (strncmp(info.name, "AES-256-CBC", 13) == 0)
                    cipher.reset(NEW_YS AES(AES_256_KEY_SZ));
                else {
                    fclose(input);
                    return SSL_BAD_FILE;
                }
                cipher->set_decryptKey(key, info.iv);
                mySTL::auto_ptr<x509> newx(NEW_YS x509(x->get_length()));   
                cipher->decrypt(newx->use_buffer(), x->get_buffer(),
                                x->get_length());
                ysDelete(x);
                x = newx.release();
            }
        }
    }

    if (type == PrivateKey && ctx->privateKey_) {
        // see if key is valid early
        TaoCrypt::Source rsaSource(ctx->privateKey_->get_buffer(),
                                   ctx->privateKey_->get_length());
        TaoCrypt::RSA_PrivateKey rsaKey;
        rsaKey.Initialize(rsaSource);

        if (rsaSource.GetError().What()) {
            // rsa failed see if DSA works

            TaoCrypt::Source dsaSource(ctx->privateKey_->get_buffer(),
                                       ctx->privateKey_->get_length());
            TaoCrypt::DSA_PrivateKey dsaKey;
            dsaKey.Initialize(dsaSource);

            if (rsaSource.GetError().What()) {
                // neither worked
                ret = SSL_FAILURE;
            }
        }
    }

    fclose(input);
    return ret;
}