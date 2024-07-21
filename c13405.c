int decrypt_stream(FILE *infp, FILE *outfp, unsigned char* passwd, int passlen)
{
    aes_context aes_ctx;
    sha256_context sha_ctx;
    aescrypt_hdr aeshdr;
    sha256_t digest;
    unsigned char IV[16];
    unsigned char iv_key[48];
    unsigned i, j, n;
    size_t bytes_read;
    unsigned char buffer[64], buffer2[32];
    unsigned char *head, *tail;
    unsigned char ipad[64], opad[64];
    int reached_eof = 0;
    
    /* Read the file header */
    if ((bytes_read = fread(&aeshdr, 1, sizeof(aeshdr), infp)) !=
         sizeof(aescrypt_hdr))
    {
        if (feof(infp))
        {
            fprintf(stderr, "Error: Input file is too short.\n");
        }
        else
        {
            perror("Error reading the file header:");
        }
        return -1;
    }

    if (!(aeshdr.aes[0] == 'A' && aeshdr.aes[1] == 'E' &&
          aeshdr.aes[2] == 'S'))
    {
        fprintf(stderr, "Error: Bad file header (not aescrypt file or is corrupted? [%x, %x, %x])\n", aeshdr.aes[0], aeshdr.aes[1], aeshdr.aes[2]);
        return -1;
    }

    /* Validate the version number and take any version-specific actions */
    if (aeshdr.version == 0)
    {
        /*
         * Let's just consider the least significant nibble to determine
         * the size of the last block
         */
        aeshdr.last_block_size = (aeshdr.last_block_size & 0x0F);
    }
    else if (aeshdr.version > 0x02)
    {
        fprintf(stderr, "Error: Unsupported AES file version: %d\n",
                aeshdr.version);
        return -1;
    }

    /* Skip over extensions present v2 and later files */
    if (aeshdr.version >= 0x02)
    {
        do
        {
            if ((bytes_read = fread(buffer, 1, 2, infp)) != 2)
            {
                if (feof(infp))
                {
                    fprintf(stderr, "Error: Input file is too short.\n");
                }
                else
                {
                    perror("Error reading the file extensions:");
                }
                return -1;
            }
            /* Determine the extension length, zero means no more extensions */
            i = j = (((int)buffer[0]) << 8) | (int)buffer[1];
            while (i--)
            {
                if ((bytes_read = fread(buffer, 1, 1, infp)) != 1)
                {
                    if (feof(infp))
                    {
                        fprintf(stderr, "Error: Input file is too short.\n");
                    }
                    else
                    {
                        perror("Error reading the file extensions:");
                    }
                    return -1;
                }
            }
        } while(j);
    }

    /* Read the initialization vector from the file */
    if ((bytes_read = fread(IV, 1, 16, infp)) != 16)
    {
        if (feof(infp))
        {
            fprintf(stderr, "Error: Input file is too short.\n");
        }
        else
        {
            perror("Error reading the initialization vector:");
        }
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
                        passlen);
        sha256_finish(  &sha_ctx,
                        digest);
    }

    /* Set the AES encryption key */
    aes_set_key(&aes_ctx, digest, 256);

    /* Set the ipad and opad arrays with values as
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

    /* If this is a version 1 or later file, then read the IV and key
     * for decrypting the bulk of the file.
     */
    if (aeshdr.version >= 0x01)
    {
        for(i=0; i<48; i+=16)
        {
            if ((bytes_read = fread(buffer, 1, 16, infp)) != 16)
            {
                if (feof(infp))
                {
                    fprintf(stderr, "Error: Input file is too short.\n");
                }
                else
                {
                    perror("Error reading input file IV and key:");
                }
                return -1;
            }

            memcpy(buffer2, buffer, 16);

            sha256_update(&sha_ctx, buffer, 16);
            aes_decrypt(&aes_ctx, buffer, buffer);

            /*
             * XOR plain text block with previous encrypted
             * output (i.e., use CBC)
             */
            for(j=0; j<16; j++)
            {
                iv_key[i+j] = (buffer[j] ^ IV[j]);
            }

            /* Update the IV (CBC mode) */
            memcpy(IV, buffer2, 16);
        }

        /* Verify that the HMAC is correct */
        sha256_finish(&sha_ctx, digest);
        sha256_starts(&sha_ctx);
        sha256_update(&sha_ctx, opad, 64);
        sha256_update(&sha_ctx, digest, 32);
        sha256_finish(&sha_ctx, digest);

        if ((bytes_read = fread(buffer, 1, 32, infp)) != 32)
        {
            if (feof(infp))
            {
                fprintf(stderr, "Error: Input file is too short.\n");
            }
            else
            {
                perror("Error reading input file digest:");
            }
            return -1;
        }

        if (memcmp(digest, buffer, 32))
        {
            fprintf(stderr, "Error: Message has been altered or password is incorrect\n");
            return -1;
        }

        /*
         * Re-load the IV and encryption key with the IV and
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
    }
    
    /*
     * Decrypt the balance of the file
     *
     * Attempt to initialize the ring buffer with contents from the file.
     * Attempt to read 48 octets of the file into the ring buffer.
     */
    if ((bytes_read = fread(buffer, 1, 48, infp)) < 48)
    {
        if (!feof(infp))
        {
            perror("Error reading input file ring:");
            return -1;
        }
        else
        {
            /*
             * If there are less than 48 octets, the only valid count
             * is 32 for version 0 (HMAC) and 33 for version 1 or
             * greater files ( file size modulo + HMAC)
             */
            if ((aeshdr.version == 0x00 && bytes_read != 32) ||
                (aeshdr.version >= 0x01 && bytes_read != 33))
            {
                fprintf(stderr, "Error: Input file is corrupt (1:%u).\n",
                        (unsigned) bytes_read);
                return -1;
            }
            else
            {
                /*
                 * Version 0 files would have the last block size
                 * read as part of the header, so let's grab that
                 * value now for version 1 files.
                 */
                if (aeshdr.version >= 0x01)
                {
                    /*
                     * The first octet must be the indicator of the
                     * last block size.
                     */
                    aeshdr.last_block_size = (buffer[0] & 0x0F);
                }
                /*
                 * If this initial read indicates there is no encrypted
                 * data, then there should be 0 in the last_block_size field
                 */
                if (aeshdr.last_block_size != 0)
                {
                    fprintf(stderr, "Error: Input file is corrupt (2).\n");
                    return -1;
                }
            }
            reached_eof = 1;
        }
    }
    head = buffer + 48;
    tail = buffer;

    while(!reached_eof)
    {
        /* Check to see if the head of the buffer is past the ring buffer */
        if (head == (buffer + 64))
        {
            head = buffer;
        }

        if ((bytes_read = fread(head, 1, 16, infp)) < 16)
        {
            if (!feof(infp))
            {
                perror("Error reading input file:");
                return -1;
            }
            else
            {
                /* The last block for v0 must be 16 and for v1 it must be 1 */
                if ((aeshdr.version == 0x00 && bytes_read > 0) ||
                    (aeshdr.version >= 0x01 && bytes_read != 1))
                {
                    fprintf(stderr, "Error: Input file is corrupt (3:%u).\n",
                            (unsigned) bytes_read);
                    return -1;
                }

                /*
                 * If this is a v1 file, then the file modulo is located
                 * in the ring buffer at tail + 16 (with consideration
                 * given to wrapping around the ring, in which case
                 * it would be at buffer[0])
                 */
                if (aeshdr.version >= 0x01)
                {
                    if ((tail + 16) < (buffer + 64))
                    {
                        aeshdr.last_block_size = (tail[16] & 0x0F);
                    }
                    else
                    {
                        aeshdr.last_block_size = (buffer[0] & 0x0F);
                    }
                }

                /* Indicate that we've reached the end of the file */
                reached_eof = 1;
            }
        }

        /*
         * Process data that has been read.  Note that if the last
         * read operation returned no additional data, there is still
         * one one ciphertext block for us to process if this is a v0 file.
         */
        if ((bytes_read > 0) || (aeshdr.version == 0x00))
        {
            /* Advance the head of the buffer forward */
            if (bytes_read > 0)
            {
                head += 16;
            }

            memcpy(buffer2, tail, 16);

            sha256_update(&sha_ctx, tail, 16);
            aes_decrypt(&aes_ctx, tail, tail);

            /*
             * XOR plain text block with previous encrypted
             * output (i.e., use CBC)
             */
            for(i=0; i<16; i++)
            {
                tail[i] ^= IV[i];
            }

            /* Update the IV (CBC mode) */
            memcpy(IV, buffer2, 16);

            /*
             * If this is the final block, then we may
             * write less than 16 octets
             */
            n = ((!reached_eof) ||
                 (aeshdr.last_block_size == 0)) ? 16 : aeshdr.last_block_size;

            /* Write the decrypted block */
            if ((i = fwrite(tail, 1, n, outfp)) != n)
            {
                perror("Error writing decrypted block:");
                return -1;
            }
            
            /* Move the tail of the ring buffer forward */
            tail += 16;
            if (tail == (buffer+64))
            {
                tail = buffer;
            }
        }
    }

    /* Verify that the HMAC is correct */
    sha256_finish(&sha_ctx, digest);
    sha256_starts(&sha_ctx);
    sha256_update(&sha_ctx, opad, 64);
    sha256_update(&sha_ctx, digest, 32);
    sha256_finish(&sha_ctx, digest);

    /* Copy the HMAC read from the file into buffer2 */
    if (aeshdr.version == 0x00)
    {
        memcpy(buffer2, tail, 16);
        tail += 16;
        if (tail == (buffer + 64))
        {
            tail = buffer;
        }
        memcpy(buffer2+16, tail, 16);
    }
    else
    {
        memcpy(buffer2, tail+1, 15);
        tail += 16;
        if (tail == (buffer + 64))
        {
            tail = buffer;
        }
        memcpy(buffer2+15, tail, 16);
        tail += 16;
        if (tail == (buffer + 64))
        {
            tail = buffer;
        }
        memcpy(buffer2+31, tail, 1);
    }

    if (memcmp(digest, buffer2, 32))
    {
        if (aeshdr.version == 0x00)
        {
            fprintf(stderr, "Error: Message has been altered or password is incorrect\n");
        }
        else
        {
            fprintf(stderr, "Error: Message has been altered and should not be trusted\n");
        }

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