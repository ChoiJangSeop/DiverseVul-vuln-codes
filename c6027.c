int cli_scanxar(cli_ctx *ctx)
{
    int rc = CL_SUCCESS;
    unsigned int cksum_fails = 0;
    unsigned int extract_errors = 0;
#if HAVE_LIBXML2
    int fd = -1;
    struct xar_header hdr;
    fmap_t *map = *ctx->fmap;
    long length, offset, size, at;
    int encoding;
    z_stream strm;
    char *toc, *tmpname;
    xmlTextReaderPtr reader = NULL;
    int a_hash, e_hash;
    unsigned char *a_cksum = NULL, *e_cksum = NULL;
    void *a_hash_ctx = NULL, *e_hash_ctx = NULL;
    char result[SHA1_HASH_SIZE];

    memset(&strm, 0x00, sizeof(z_stream));

    /* retrieve xar header */
    if (fmap_readn(*ctx->fmap, &hdr, 0, sizeof(hdr)) != sizeof(hdr)) {
        cli_dbgmsg("cli_scanxar: Invalid header, too short.\n");
        return CL_EFORMAT;
    }
    hdr.magic = be32_to_host(hdr.magic);

    if (hdr.magic == XAR_HEADER_MAGIC) {
        cli_dbgmsg("cli_scanxar: Matched magic\n");
    }
    else {
        cli_dbgmsg("cli_scanxar: Invalid magic\n");
        return CL_EFORMAT;
    }
    hdr.size = be16_to_host(hdr.size);
    hdr.version = be16_to_host(hdr.version);
    hdr.toc_length_compressed = be64_to_host(hdr.toc_length_compressed);
    hdr.toc_length_decompressed = be64_to_host(hdr.toc_length_decompressed);
    hdr.chksum_alg = be32_to_host(hdr.chksum_alg);

    /* cli_dbgmsg("hdr.magic %x\n", hdr.magic); */
    /* cli_dbgmsg("hdr.size %i\n", hdr.size); */
    /* cli_dbgmsg("hdr.version %i\n", hdr.version); */
    /* cli_dbgmsg("hdr.toc_length_compressed %lu\n", hdr.toc_length_compressed); */
    /* cli_dbgmsg("hdr.toc_length_decompressed %lu\n", hdr.toc_length_decompressed); */
    /* cli_dbgmsg("hdr.chksum_alg %i\n", hdr.chksum_alg); */
 
    /* Uncompress TOC */
    strm.next_in = (unsigned char *)fmap_need_off_once(*ctx->fmap, hdr.size, hdr.toc_length_compressed);
    if (strm.next_in == NULL) {
        cli_dbgmsg("cli_scanxar: fmap_need_off_once fails on TOC.\n");
        return CL_EREAD;
    }
    strm.avail_in = hdr.toc_length_compressed; 
    toc = cli_malloc(hdr.toc_length_decompressed+1);
    if (toc == NULL) {
        cli_dbgmsg("cli_scanxar: cli_malloc fails on TOC decompress buffer.\n");
        return CL_EMEM;
    }
    toc[hdr.toc_length_decompressed] = '\0';
    strm.avail_out = hdr.toc_length_decompressed;
    strm.next_out = (unsigned char *)toc;
    rc = inflateInit(&strm);
    if (rc != Z_OK) {
        cli_dbgmsg("cli_scanxar:inflateInit error %i \n", rc);
        rc = CL_EFORMAT;
        goto exit_toc;
    }    
    rc = inflate(&strm, Z_SYNC_FLUSH);
    if (rc != Z_OK && rc != Z_STREAM_END) {
        cli_dbgmsg("cli_scanxar:inflate error %i \n", rc);
        rc = CL_EFORMAT;
        goto exit_toc;
    }
    rc = inflateEnd(&strm);
    if (rc != Z_OK) {
        cli_dbgmsg("cli_scanxar:inflateEnd error %i \n", rc);
        rc = CL_EFORMAT;
        goto exit_toc;
    }

    /* cli_dbgmsg("cli_scanxar: TOC xml:\n%s\n", toc); */
    /* printf("cli_scanxar: TOC xml:\n%s\n", toc); */
    /* cli_dbgmsg("cli_scanxar: TOC end:\n"); */
    /* printf("cli_scanxar: TOC end:\n"); */

    /* scan the xml */
    cli_dbgmsg("cli_scanxar: scanning xar TOC xml in memory.\n"); 
    rc = cli_mem_scandesc(toc, hdr.toc_length_decompressed, ctx);
    if (rc != CL_SUCCESS) {
        if (rc != CL_VIRUS || !SCAN_ALL)
            goto exit_toc;        
    }

    /* make a file to leave if --leave-temps in effect */
    if(ctx->engine->keeptmp) {
        if ((rc = cli_gentempfd(ctx->engine->tmpdir, &tmpname, &fd)) != CL_SUCCESS) {
            cli_dbgmsg("cli_scanxar: Can't create temporary file for TOC.\n");
            goto exit_toc;
        }
        if (cli_writen(fd, toc, hdr.toc_length_decompressed) < 0) {
            cli_dbgmsg("cli_scanxar: cli_writen error writing TOC.\n");
            rc = CL_EWRITE;
            xar_cleanup_temp_file(ctx, fd, tmpname);
            goto exit_toc;
        }
        rc = xar_cleanup_temp_file(ctx, fd, tmpname);
        if (rc != CL_SUCCESS)
            goto exit_toc;
    }

    reader = xmlReaderForMemory(toc, hdr.toc_length_decompressed, "noname.xml", NULL, CLAMAV_MIN_XMLREADER_FLAGS);
    if (reader == NULL) {
        cli_dbgmsg("cli_scanxar: xmlReaderForMemory error for TOC\n");
        goto exit_toc;
    }

    rc = xar_scan_subdocuments(reader, ctx);
    if (rc != CL_SUCCESS) {
        cli_dbgmsg("xar_scan_subdocuments returns %i.\n", rc);
        goto exit_reader;
    }

    /* Walk the TOC XML and extract files */
    fd = -1;
    tmpname = NULL;
    while (CL_SUCCESS == (rc = xar_get_toc_data_values(reader, &length, &offset, &size, &encoding,
                                                       &a_cksum, &a_hash, &e_cksum, &e_hash))) {
        int do_extract_cksum = 1;
        unsigned char * blockp;
        void *a_sc, *e_sc;
        void *a_mc, *e_mc;
        char * expected;

        /* clean up temp file from previous loop iteration */
        if (fd > -1 && tmpname) {
            rc = xar_cleanup_temp_file(ctx, fd, tmpname);
            if (rc != CL_SUCCESS)
                goto exit_reader;
        }

        at = offset + hdr.toc_length_compressed + hdr.size;

        if ((rc = cli_gentempfd(ctx->engine->tmpdir, &tmpname, &fd)) != CL_SUCCESS) {
            cli_dbgmsg("cli_scanxar: Can't generate temporary file.\n");
            goto exit_reader;
        }

        cli_dbgmsg("cli_scanxar: decompress into temp file:\n%s, size %li,\n"
                   "from xar heap offset %li length %li\n",
                   tmpname, size, offset, length);


        a_hash_ctx = xar_hash_init(a_hash, &a_sc, &a_mc);
        e_hash_ctx = xar_hash_init(e_hash, &e_sc, &e_mc);

        switch (encoding) {
        case CL_TYPE_GZ:
            /* inflate gzip directly because file segments do not contain magic */
            memset(&strm, 0, sizeof(strm));
            if ((rc = inflateInit(&strm)) != Z_OK) {
                cli_dbgmsg("cli_scanxar: InflateInit failed: %d\n", rc);
                rc = CL_EFORMAT;
                extract_errors++;
                break;
            }
            
            while ((size_t)at < map->len && (unsigned long)at < offset+hdr.toc_length_compressed+hdr.size+length) {
                unsigned long avail_in;
                void * next_in;
                unsigned int bytes = MIN(map->len - at, map->pgsz);
                bytes = MIN(length, bytes);
                if(!(strm.next_in = next_in = (void*)fmap_need_off_once(map, at, bytes))) {
                    cli_dbgmsg("cli_scanxar: Can't read %u bytes @ %lu.\n", bytes, (long unsigned)at);
                    inflateEnd(&strm);
                    rc = CL_EREAD;
                    goto exit_tmpfile;
                }
                at += bytes;
                strm.avail_in = avail_in = bytes;
                do {
                    int inf, outsize = 0;
                    unsigned char buff[FILEBUFF];
                    strm.avail_out = sizeof(buff);
                    strm.next_out = buff;
                    inf = inflate(&strm, Z_SYNC_FLUSH);
                    if (inf != Z_OK && inf != Z_STREAM_END && inf != Z_BUF_ERROR) {
                        cli_dbgmsg("cli_scanxar: inflate error %i %s.\n", inf, strm.msg?strm.msg:"");
                        rc = CL_EFORMAT;
                        extract_errors++;
                        break;
                    }

                    bytes = sizeof(buff) - strm.avail_out;

                    if (e_hash_ctx != NULL)
                        xar_hash_update(e_hash_ctx, buff, bytes, e_hash);
                   
                    if (cli_writen(fd, buff, bytes) < 0) {
                        cli_dbgmsg("cli_scanxar: cli_writen error file %s.\n", tmpname);
                        inflateEnd(&strm);
                        rc = CL_EWRITE;
                        goto exit_tmpfile;
                    }
                    outsize += sizeof(buff) - strm.avail_out;
                    if (cli_checklimits("cli_scanxar", ctx, outsize, 0, 0) != CL_CLEAN) {
                        break;
                    }
                    if (inf == Z_STREAM_END) {
                        break;
                    }
                } while (strm.avail_out == 0);

                if (rc != CL_SUCCESS)
                    break;

                avail_in -= strm.avail_in;
                if (a_hash_ctx != NULL)
                    xar_hash_update(a_hash_ctx, next_in, avail_in, a_hash);
            }

            inflateEnd(&strm);
            break;
        case CL_TYPE_7Z:
#define CLI_LZMA_OBUF_SIZE 1024*1024
#define CLI_LZMA_HDR_SIZE LZMA_PROPS_SIZE+8
#define CLI_LZMA_IBUF_SIZE CLI_LZMA_OBUF_SIZE>>2 /* estimated compression ratio 25% */
            {
                struct CLI_LZMA lz;
                unsigned long in_remaining = length;
                unsigned long out_size = 0;
                unsigned char * buff = __lzma_wrap_alloc(NULL, CLI_LZMA_OBUF_SIZE);
                int lret;
                
                memset(&lz, 0, sizeof(lz));
                if (buff == NULL) {
                    cli_dbgmsg("cli_scanxar: memory request for lzma decompression buffer fails.\n");
                    rc = CL_EMEM;
                    goto exit_tmpfile;
                    
                }

                blockp = (void*)fmap_need_off_once(map, at, CLI_LZMA_HDR_SIZE);
                if (blockp == NULL) {
                    char errbuff[128];
                    cli_strerror(errno, errbuff, sizeof(errbuff));
                    cli_dbgmsg("cli_scanxar: Can't read %li bytes @ %li, errno:%s.\n",
                               length, at, errbuff);
                    rc = CL_EREAD;
                    __lzma_wrap_free(NULL, buff);
                    goto exit_tmpfile;
                }

                lz.next_in = blockp;
                lz.avail_in = CLI_LZMA_HDR_SIZE;

                if (a_hash_ctx != NULL)
                    xar_hash_update(a_hash_ctx, blockp, CLI_LZMA_HDR_SIZE, a_hash);

                lret = cli_LzmaInit(&lz, 0);
                if (lret != LZMA_RESULT_OK) {
                    cli_dbgmsg("cli_scanxar: cli_LzmaInit() fails: %i.\n", lret);
                    rc = CL_EFORMAT;
                    __lzma_wrap_free(NULL, buff);
                    extract_errors++;
                    break;
                }

                at += CLI_LZMA_HDR_SIZE;
                in_remaining -= CLI_LZMA_HDR_SIZE;
                while ((size_t)at < map->len && (unsigned long)at < offset+hdr.toc_length_compressed+hdr.size+length) {
                    SizeT avail_in;
                    SizeT avail_out;
                    void * next_in;
                    unsigned long in_consumed;

                    lz.next_out = buff;
                    lz.avail_out = CLI_LZMA_OBUF_SIZE;
                    lz.avail_in = avail_in = MIN(CLI_LZMA_IBUF_SIZE, in_remaining);
                    lz.next_in = next_in = (void*)fmap_need_off_once(map, at, lz.avail_in);
                    if (lz.next_in == NULL) {
                        char errbuff[128];
                        cli_strerror(errno, errbuff, sizeof(errbuff));
                        cli_dbgmsg("cli_scanxar: Can't read %li bytes @ %li, errno: %s.\n",
                                   length, at, errbuff);
                        rc = CL_EREAD;
                        __lzma_wrap_free(NULL, buff);
                        cli_LzmaShutdown(&lz);
                        goto exit_tmpfile;
                    }

                    lret = cli_LzmaDecode(&lz);
                    if (lret != LZMA_RESULT_OK && lret != LZMA_STREAM_END) {
                        cli_dbgmsg("cli_scanxar: cli_LzmaDecode() fails: %i.\n", lret);
                        rc = CL_EFORMAT;
                        extract_errors++;
                        break;
                    }

                    in_consumed = avail_in - lz.avail_in;
                    in_remaining -= in_consumed;
                    at += in_consumed;
                    avail_out = CLI_LZMA_OBUF_SIZE - lz.avail_out;
                    
                    if (avail_out == 0)
                        cli_dbgmsg("cli_scanxar: cli_LzmaDecode() produces no output for "
                                   "avail_in %llu, avail_out %llu.\n",
                                   (long long unsigned)avail_in, (long long unsigned)avail_out);

                    if (a_hash_ctx != NULL)
                        xar_hash_update(a_hash_ctx, next_in, in_consumed, a_hash);                    
                    if (e_hash_ctx != NULL)
                        xar_hash_update(e_hash_ctx, buff, avail_out, e_hash);

                    /* Write a decompressed block. */
                    /* cli_dbgmsg("Writing %li bytes to LZMA decompress temp file, " */
                    /*            "consumed %li of %li available compressed bytes.\n", */
                    /*            avail_out, in_consumed, avail_in); */

                    if (cli_writen(fd, buff, avail_out) < 0) {
                        cli_dbgmsg("cli_scanxar: cli_writen error writing lzma temp file for %llu bytes.\n",
                                   (long long unsigned)avail_out);
                        __lzma_wrap_free(NULL, buff);
                        cli_LzmaShutdown(&lz);
                        rc = CL_EWRITE;
                        goto exit_tmpfile;
                    }

                    /* Check file size limitation. */
                    out_size += avail_out;
                    if (cli_checklimits("cli_scanxar", ctx, out_size, 0, 0) != CL_CLEAN) {
                        break;
                    }

                    if (lret == LZMA_STREAM_END)
                        break;
                }

                cli_LzmaShutdown(&lz);
                __lzma_wrap_free(NULL, buff);
            }
            break; 
        case CL_TYPE_ANY:
        default:
        case CL_TYPE_BZ:
        case CL_TYPE_XZ:
            /* for uncompressed, bzip2, xz, and unknown, just pull the file, cli_magic_scandesc does the rest */
            do_extract_cksum = 0;
            {
                unsigned long write_len;
                
                if (ctx->engine->maxfilesize)
                    write_len = MIN((size_t)(ctx->engine->maxfilesize), (size_t)length);
                else
                    write_len = length;
                    
                if (!(blockp = (void*)fmap_need_off_once(map, at, length))) {
                    char errbuff[128];
                    cli_strerror(errno, errbuff, sizeof(errbuff));
                    cli_dbgmsg("cli_scanxar: Can't read %li bytes @ %li, errno:%s.\n",
                               length, at, errbuff);
                    rc = CL_EREAD;
                    goto exit_tmpfile;
                }
                
                if (a_hash_ctx != NULL)
                    xar_hash_update(a_hash_ctx, blockp, length, a_hash);
                
                if (cli_writen(fd, blockp, write_len) < 0) {
                    cli_dbgmsg("cli_scanxar: cli_writen error %li bytes @ %li.\n", length, at);
                    rc = CL_EWRITE;
                    goto exit_tmpfile;
                }
                /*break;*/
            }          
        }

        if (rc == CL_SUCCESS) {
            if (a_hash_ctx != NULL) {
                xar_hash_final(a_hash_ctx, result, a_hash);
                a_hash_ctx = NULL;
            } else {
                cli_dbgmsg("cli_scanxar: archived-checksum missing.\n");
                cksum_fails++;
            }
            if (a_cksum != NULL) {
                expected = cli_hex2str((char *)a_cksum);
                if (xar_hash_check(a_hash, result, expected) != 0) {
                    cli_dbgmsg("cli_scanxar: archived-checksum mismatch.\n");
                    cksum_fails++;
                } else {
                    cli_dbgmsg("cli_scanxar: archived-checksum matched.\n");                
                }
                free(expected);
            }

            if (e_hash_ctx != NULL) {
                xar_hash_final(e_hash_ctx, result, e_hash);
                e_hash_ctx = NULL;
            } else {
                cli_dbgmsg("cli_scanxar: extracted-checksum(unarchived-checksum) missing.\n");
                cksum_fails++;
            }
            if (e_cksum != NULL) {
                if (do_extract_cksum) {
                    expected = cli_hex2str((char *)e_cksum);
                    if (xar_hash_check(e_hash, result, expected) != 0) {
                        cli_dbgmsg("cli_scanxar: extracted-checksum mismatch.\n");
                        cksum_fails++;
                    } else {
                        cli_dbgmsg("cli_scanxar: extracted-checksum matched.\n");                
                    }
                    free(expected);
                }
            }
        
            rc = cli_magic_scandesc(fd, ctx);
            if (rc != CL_SUCCESS) {
                if (rc == CL_VIRUS) {
                    cli_dbgmsg("cli_scanxar: Infected with %s\n", cli_get_last_virus(ctx));
                    if (!SCAN_ALL)
                        goto exit_tmpfile;
                } else if (rc != CL_BREAK) {
                    cli_dbgmsg("cli_scanxar: cli_magic_scandesc error %i\n", rc);
                    goto exit_tmpfile;
                }
            }
        }
        
        if (a_cksum != NULL) {
            xmlFree(a_cksum);
            a_cksum = NULL;
        }
        if (e_cksum != NULL) {
            xmlFree(e_cksum);
            e_cksum = NULL;
        }
    }

 exit_tmpfile:
    xar_cleanup_temp_file(ctx, fd, tmpname);
    if (a_hash_ctx != NULL)
        xar_hash_final(a_hash_ctx, result, a_hash);
    if (e_hash_ctx != NULL)
        xar_hash_final(e_hash_ctx, result, e_hash);
 
 exit_reader:
    if (a_cksum != NULL)
        xmlFree(a_cksum);   
    if (e_cksum != NULL)
        xmlFree(e_cksum);
    xmlTextReaderClose(reader);
    xmlFreeTextReader(reader);

 exit_toc:
    free(toc);
    if (rc == CL_BREAK)
        rc = CL_SUCCESS;
#else
    cli_dbgmsg("cli_scanxar: can't scan xar files, need libxml2.\n");
#endif
    if (cksum_fails + extract_errors != 0) {
        cli_warnmsg("cli_scanxar: %u checksum errors and %u extraction errors, use --debug for more info.\n",
                    cksum_fails, extract_errors);
    }

    return rc;
}