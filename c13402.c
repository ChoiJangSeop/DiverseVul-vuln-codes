int encrypt_stream(FILE *infp, FILE *outfp, unsigned char* passwd, int passlen)
{
    aes_context aes_ctx;
    sha256_context sha_ctx;
    aescrypt_hdr aeshdr;
    sha256_t digest;
    unsigned char IV[16];
    unsigned char iv_key[48];
    unsigned i, j;
    size_t bytes_read;
    unsigned char buffer[32];
    unsigned char ipad[64], opad[64];
    time_t current_time;
    pid_t process_id;
    void *aesrand;
    unsigned char tag_buffer[256];

    /*
     * Open the source for random data.  Note that while the entropy
     * might be lower with /dev/urandom than /dev/random, it will not
     * fail to produce something.  Also, we're going to hash the result
     * anyway.
     */
    if ((aesrand = aesrandom_open()) == NULL)
    {
        perror("Error open random:");
        return -1;
    }

    /*
     * Create the 16-octet IV and 32-octet encryption key
     * used for encrypting the plaintext file.  We do
     * not trust the rand() function, so we improve on
     * that by also hashing the random digits and using
     * only a portion of the hash.  This IV and key
     * generation could be replaced with any good random
     * source of data.
     */
    memset(iv_key, 0, 48);
    for (i=0; i<48; i+=16)
    {
        memset(buffer, 0, 32);
        sha256_starts(&sha_ctx);
        for(j=0; j<256; j++)
        {
            if ((bytes_read = aesrandom_read(aesrand, buffer, 32)) != 32)
            {
                fprintf(stderr, "Error: Couldn't read from random : %u\n",
                        (unsigned) bytes_read);
                aesrandom_close(aesrand);
                return -1;
            }
            sha256_update(&sha_ctx, buffer, 32);
        }
        sha256_finish(&sha_ctx, digest);
        memcpy(iv_key+i, digest, 16);
    }

    /*
     * Write an AES signature at the head of the file, along
     * with the AES file format version number.
     */
    buffer[0] = 'A';
    buffer[1] = 'E';
    buffer[2] = 'S';
    buffer[3] = (unsigned char) 0x02;   /* Version 2 */
    buffer[4] = '\0';                   /* Reserved for version 0 */
    if (fwrite(buffer, 1, 5, outfp) != 5)
    {
        fprintf(stderr, "Error: Could not write out header data\n");
        aesrandom_close(aesrand);
        return -1;
    }

    /* Write out the CREATED-BY tag */
    j = 11 +                      /* "CREATED-BY\0" */
        strlen(PACKAGE_NAME) +    /* Program name */
        1 +                       /* Space */
        strlen(PACKAGE_VERSION);  /* Program version ID */

    /*
     * Our extension buffer is only 256 octets long, so
     * let's not write an extension if it is too big
     */
    if (j < 256)
    {
        buffer[0] = '\0';
        buffer[1] = (unsigned char) (j & 0xff);
        if (fwrite(buffer, 1, 2, outfp) != 2)
        {
            fprintf(stderr, "Error: Could not write tag to AES file (1)\n");
            aesrandom_close(aesrand);
            return -1;
        }

        strncpy((char *)tag_buffer, "CREATED_BY", 255);
        tag_buffer[255] = '\0';
        if (fwrite(tag_buffer, 1, 11, outfp) != 11)
        {
            fprintf(stderr, "Error: Could not write tag to AES file (2)\n");
            aesrandom_close(aesrand);
            return -1;
        }

        sprintf((char *)tag_buffer, "%s %s", PACKAGE_NAME, PACKAGE_VERSION);
        j = strlen((char *)tag_buffer);
        if (fwrite(tag_buffer, 1, j, outfp) != j)
        {
            fprintf(stderr, "Error: Could not write tag to AES file (3)\n");
            aesrandom_close(aesrand);
            return -1;
        }
    }

    /* Write out the "container" extension */
    buffer[0] = '\0';
    buffer[1] = (unsigned char) 128;
    if (fwrite(buffer, 1, 2, outfp) != 2)
    {
        fprintf(stderr, "Error: Could not write tag to AES file (4)\n");
        aesrandom_close(aesrand);
        return -1;
    }
    memset(tag_buffer, 0, 128);
    if (fwrite(tag_buffer, 1, 128, outfp) != 128)
    {
        fprintf(stderr, "Error: Could not write tag to AES file (5)\n");
        aesrandom_close(aesrand);
        return -1;
    }

    /* Write out 0x0000 to indicate that no more extensions exist */
    buffer[0] = '\0';
    buffer[1] = '\0';
    if (fwrite(buffer, 1, 2, outfp) != 2)
    {
        fprintf(stderr, "Error: Could not write tag to AES file (6)\n");
        aesrandom_close(aesrand);
        return -1;
    }

    /*
     * We will use an initialization vector comprised of the current time
     * process ID, and random data, all hashed together with SHA-256.
     */
    sha256_starts(  &sha_ctx);
    current_time = time(NULL);
    sha256_update(  &sha_ctx, (unsigned char *)&time, sizeof(current_time));
    process_id = getpid();
    sha256_update(  &sha_ctx, (unsigned char *)&process_id, sizeof(process_id));

    for (i=0; i<256; i++)
    {
        if (aesrandom_read(aesrand, buffer, 32) != 32)
        {
            fprintf(stderr, "Error: Couldn't read from /dev/random\n");
            aesrandom_close(aesrand);
            return -1;
        }
        sha256_update(  &sha_ctx,
                        buffer,
                        32);
    }

    sha256_finish(  &sha_ctx, digest);

    memcpy(IV, digest, 16);

    /* We're finished collecting random data */
    aesrandom_close(aesrand);

    /* Write the initialization vector to the file */
    if (fwrite(IV, 1, 16, outfp) != 16)
    {
        fprintf(stderr, "Error: Could not write out initialization vector\n");
        return -1;
    }
    
    /* Hash the IV and password 8192 times */
    memset(digest, 0, 32);
    memcpy(digest, IV, 16);
    for(i=0; i<8192; i++)
    {
        sha256_starts(  &sha_ctx);
        sha256_update(  &sha_ctx, digest, 32);
        sha256_update(  &sha_ctx,
                        passwd,
                        (unsigned long)passlen);
        sha256_finish(  &sha_ctx,
                        digest);
    }

    /* Set the AES encryption key */
    aes_set_key(&aes_ctx, digest, 256);

    /*
     * Set the ipad and opad arrays with values as
     * per RFC 2104 (HMAC).  HMAC is defined as
     *   H(K XOR opad, H(K XOR ipad, text))
     */
    memset(ipad, 0x36, 64);
    memset(opad, 0x5C, 64);

    for(i=0; i<32; i++)
    {
        ipad[i] ^= digest[i];
        opad[i] ^= digest[i];
    }

    sha256_starts(&sha_ctx);
    sha256_update(&sha_ctx, ipad, 64);

    /*
     * Encrypt the IV and key used to encrypt the plaintext file,
     * writing that encrypted text to the output file.
     */
    for(i=0; i<48; i+=16)
    {
        /*
         * Place the next 16 octets of IV and key buffer into
         * the input buffer.
         */
        memcpy(buffer, iv_key+i, 16);

        /*
         * XOR plain text block with previous encrypted
         * output (i.e., use CBC)
         */
        for(j=0; j<16; j++)
        {
            buffer[j] ^= IV[j];
        }

        /* Encrypt the contents of the buffer */
        aes_encrypt(&aes_ctx, buffer, buffer);
        
        /* Concatenate the "text" as we compute the HMAC */
        sha256_update(&sha_ctx, buffer, 16);

        /* Write the encrypted block */
        if (fwrite(buffer, 1, 16, outfp) != 16)
        {
            fprintf(stderr, "Error: Could not write iv_key data\n");
            return -1;
        }
        
        /* Update the IV (CBC mode) */
        memcpy(IV, buffer, 16);
    }

    /* Write the HMAC */
    sha256_finish(&sha_ctx, digest);
    sha256_starts(&sha_ctx);
    sha256_update(&sha_ctx, opad, 64);
    sha256_update(&sha_ctx, digest, 32);
    sha256_finish(&sha_ctx, digest);
    /* Write the encrypted block */
    if (fwrite(digest, 1, 32, outfp) != 32)
    {
        fprintf(stderr, "Error: Could not write iv_key HMAC\n");
        return -1;
    }

    /* Re-load the IV and encryption key with the IV and
     * key to now encrypt the datafile.  Also, reset the HMAC
     * computation.
     */
    memcpy(IV, iv_key, 16);

    /* Set the AES encryption key */
    aes_set_key(&aes_ctx, iv_key+16, 256);

    /*
     * Set the ipad and opad arrays with values as
     * per RFC 2104 (HMAC).  HMAC is defined as
     *   H(K XOR opad, H(K XOR ipad, text))
     */
    memset(ipad, 0x36, 64);
    memset(opad, 0x5C, 64);

    for(i=0; i<32; i++)
    {
        ipad[i] ^= iv_key[i+16];
        opad[i] ^= iv_key[i+16];
    }

    /* Wipe the IV and encryption key from memory */
    memset_secure(iv_key, 0, 48);

    sha256_starts(&sha_ctx);
    sha256_update(&sha_ctx, ipad, 64);

    /* Initialize the last_block_size value to 0 */
    aeshdr.last_block_size = 0;

    while ((bytes_read = fread(buffer, 1, 16, infp)) > 0)
    {
        /*
         * XOR plain text block with previous encrypted
         * output (i.e., use CBC)
         */
        for(i=0; i<16; i++)
        {
            buffer[i] ^= IV[i];
        }

        /* Encrypt the contents of the buffer */
        aes_encrypt(&aes_ctx, buffer, buffer);
        
        /* Concatenate the "text" as we compute the HMAC */
        sha256_update(&sha_ctx, buffer, 16);

        /* Write the encrypted block */
        if (fwrite(buffer, 1, 16, outfp) != 16)
        {
            fprintf(stderr, "Error: Could not write to output file\n");
            return -1;
        }
        
        /* Update the IV (CBC mode) */
        memcpy(IV, buffer, 16);

        /* Assume this number of octets is the file modulo */
        aeshdr.last_block_size = bytes_read;
    }

    /* Check to see if we had a read error */
    if (ferror(infp))
    {
        fprintf(stderr, "Error: Couldn't read input file\n");
        return -1;
    }

    /* Write the file size modulo */
    buffer[0] = (char) (aeshdr.last_block_size & 0x0F);
    if (fwrite(buffer, 1, 1, outfp) != 1)
    {
        fprintf(stderr, "Error: Could not write the file size modulo\n");
        return -1;
    }

    /* Write the HMAC */
    sha256_finish(&sha_ctx, digest);
    sha256_starts(&sha_ctx);
    sha256_update(&sha_ctx, opad, 64);
    sha256_update(&sha_ctx, digest, 32);
    sha256_finish(&sha_ctx, digest);
    if (fwrite(digest, 1, 32, outfp) != 32)
    {
        fprintf(stderr, "Error: Could not write the file HMAC\n");
        return -1;
    }

    /* Flush the output buffer to ensure all data is written to disk */
    if (fflush(outfp))
    {
        fprintf(stderr, "Error: Could not flush output file buffer\n");
        return -1;
    }

    return 0;
}