int extract_or_test_files(__G)    /* return PK-type error code */
     __GDEF
{
    unsigned i, j;
    zoff_t cd_bufstart;
    uch *cd_inptr;
    int cd_incnt;
    ulg filnum=0L, blknum=0L;
    int reached_end;
#ifndef SFX
    int no_endsig_found;
#endif
    int error, error_in_archive=PK_COOL;
    int *fn_matched=NULL, *xn_matched=NULL;
    zucn_t members_processed;
    ulg num_skipped=0L, num_bad_pwd=0L;
    zoff_t old_extra_bytes = 0L;
#ifdef SET_DIR_ATTRIB
    unsigned num_dirs=0;
    direntry *dirlist=(direntry *)NULL, **sorted_dirlist=(direntry **)NULL;
#endif

    /*
     * First, two general initializations are applied. These have been moved
     * here from process_zipfiles() because they are only needed for accessing
     * and/or extracting the data content of the zip archive.
     */

    /* a) initialize the CRC table pointer (once) */
    if (CRC_32_TAB == NULL) {
        if ((CRC_32_TAB = get_crc_table()) == NULL) {
            return PK_MEM;
        }
    }

#if (!defined(SFX) || defined(SFX_EXDIR))
    /* b) check out if specified extraction root directory exists */
    if (uO.exdir != (char *)NULL && G.extract_flag) {
        G.create_dirs = !uO.fflag;
        if ((error = checkdir(__G__ uO.exdir, ROOT)) > MPN_INF_SKIP) {
            /* out of memory, or file in way */
            return (error == MPN_NOMEM ? PK_MEM : PK_ERR);
        }
    }
#endif /* !SFX || SFX_EXDIR */

    /* One more: initialize cover structure for bomb detection. Start with a
       span that covers the central directory though the end of the file. */
    if (G.cover == NULL) {
        G.cover = malloc(sizeof(cover_t));
        if (G.cover == NULL) {
            Info(slide, 0x401, ((char *)slide,
              LoadFarString(NotEnoughMemCover)));
            return PK_MEM;
        }
        ((cover_t *)G.cover)->span = NULL;
        ((cover_t *)G.cover)->max = 0;
    }
    ((cover_t *)G.cover)->num = 0;
    if ((G.extra_bytes != 0 &&
         cover_add((cover_t *)G.cover, 0, G.extra_bytes) != 0) ||
        cover_add((cover_t *)G.cover,
                  G.extra_bytes + G.ecrec.offset_start_central_directory,
                  G.ziplen) != 0) {
        Info(slide, 0x401, ((char *)slide,
          LoadFarString(NotEnoughMemCover)));
        return PK_MEM;
    }

/*---------------------------------------------------------------------------
    The basic idea of this function is as follows.  Since the central di-
    rectory lies at the end of the zipfile and the member files lie at the
    beginning or middle or wherever, it is not very desirable to simply
    read a central directory entry, jump to the member and extract it, and
    then jump back to the central directory.  In the case of a large zipfile
    this would lead to a whole lot of disk-grinding, especially if each mem-
    ber file is small.  Instead, we read from the central directory the per-
    tinent information for a block of files, then go extract/test the whole
    block.  Thus this routine contains two small(er) loops within a very
    large outer loop:  the first of the small ones reads a block of files
    from the central directory; the second extracts or tests each file; and
    the outer one loops over blocks.  There's some file-pointer positioning
    stuff in between, but that's about it.  Btw, it's because of this jump-
    ing around that we can afford to be lenient if an error occurs in one of
    the member files:  we should still be able to go find the other members,
    since we know the offset of each from the beginning of the zipfile.
  ---------------------------------------------------------------------------*/

    G.pInfo = G.info;

#if CRYPT
    G.newzip = TRUE;
#endif
#ifndef SFX
    G.reported_backslash = FALSE;
#endif

    /* malloc space for check on unmatched filespecs (OK if one or both NULL) */
    if (G.filespecs > 0  &&
        (fn_matched=(int *)malloc(G.filespecs*sizeof(int))) != (int *)NULL)
        for (i = 0;  i < G.filespecs;  ++i)
            fn_matched[i] = FALSE;
    if (G.xfilespecs > 0  &&
        (xn_matched=(int *)malloc(G.xfilespecs*sizeof(int))) != (int *)NULL)
        for (i = 0;  i < G.xfilespecs;  ++i)
            xn_matched[i] = FALSE;

/*---------------------------------------------------------------------------
    Begin main loop over blocks of member files.  We know the entire central
    directory is on this disk:  we would not have any of this information un-
    less the end-of-central-directory record was on this disk, and we would
    not have gotten to this routine unless this is also the disk on which
    the central directory starts.  In practice, this had better be the ONLY
    disk in the archive, but we'll add multi-disk support soon.
  ---------------------------------------------------------------------------*/

    members_processed = 0;
#ifndef SFX
    no_endsig_found = FALSE;
#endif
    reached_end = FALSE;
    while (!reached_end) {
        j = 0;
#ifdef AMIGA
        memzero(G.filenotes, DIR_BLKSIZ * sizeof(char *));
#endif

        /*
         * Loop through files in central directory, storing offsets, file
         * attributes, case-conversion and text-conversion flags until block
         * size is reached.
         */

        while ((j < DIR_BLKSIZ)) {
            G.pInfo = &G.info[j];

            if (readbuf(__G__ G.sig, 4) == 0) {
                error_in_archive = PK_EOF;
                reached_end = TRUE;     /* ...so no more left to do */
                break;
            }
            if (memcmp(G.sig, central_hdr_sig, 4)) {  /* is it a new entry? */
                /* no new central directory entry
                 * -> is the number of processed entries compatible with the
                 *    number of entries as stored in the end_central record?
                 */
                if ((members_processed
                     & (G.ecrec.have_ecr64 ? MASK_ZUCN64 : MASK_ZUCN16))
                    == G.ecrec.total_entries_central_dir) {
#ifndef SFX
                    /* yes, so look if we ARE back at the end_central record
                     */
                    no_endsig_found =
                      ( (memcmp(G.sig,
                                (G.ecrec.have_ecr64 ?
                                 end_central64_sig : end_central_sig),
                                4) != 0)
                       && (!G.ecrec.is_zip64_archive)
                       && (memcmp(G.sig, end_central_sig, 4) != 0)
                      );
#endif /* !SFX */
                } else {
                    /* no; we have found an error in the central directory
                     * -> report it and stop searching for more Zip entries
                     */
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(CentSigMsg), j + blknum*DIR_BLKSIZ + 1));
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(ReportMsg)));
                    error_in_archive = PK_BADERR;
                }
                reached_end = TRUE;     /* ...so no more left to do */
                break;
            }
            /* process_cdir_file_hdr() sets pInfo->hostnum, pInfo->lcflag */
            if ((error = process_cdir_file_hdr(__G)) != PK_COOL) {
                error_in_archive = error;   /* only PK_EOF defined */
                reached_end = TRUE;     /* ...so no more left to do */
                break;
            }
            if ((error = do_string(__G__ G.crec.filename_length, DS_FN)) !=
                 PK_COOL)
            {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > PK_WARN) {  /* fatal:  no more left to do */
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(FilNamMsg),
                      FnFilter1(G.filename), "central"));
                    reached_end = TRUE;
                    break;
                }
            }
            if ((error = do_string(__G__ G.crec.extra_field_length,
                EXTRA_FIELD)) != 0)
            {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > PK_WARN) {  /* fatal */
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(ExtFieldMsg),
                      FnFilter1(G.filename), "central"));
                    reached_end = TRUE;
                    break;
                }
            }
#ifdef AMIGA
            G.filenote_slot = j;
            if ((error = do_string(__G__ G.crec.file_comment_length,
                                   uO.N_flag ? FILENOTE : SKIP)) != PK_COOL)
#else
            if ((error = do_string(__G__ G.crec.file_comment_length, SKIP))
                != PK_COOL)
#endif
            {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > PK_WARN) {  /* fatal */
                    Info(slide, 0x421, ((char *)slide,
                      LoadFarString(BadFileCommLength),
                      FnFilter1(G.filename)));
                    reached_end = TRUE;
                    break;
                }
            }
            if (G.process_all_files) {
                if (store_info(__G))
                    ++j;  /* file is OK; info[] stored; continue with next */
                else
                    ++num_skipped;
            } else {
                int   do_this_file;

                if (G.filespecs == 0)
                    do_this_file = TRUE;
                else {  /* check if this entry matches an `include' argument */
                    do_this_file = FALSE;
                    for (i = 0; i < G.filespecs; i++)
                        if (match(G.filename, G.pfnames[i], uO.C_flag WISEP)) {
                            do_this_file = TRUE;  /* ^-- ignore case or not? */
                            if (fn_matched)
                                fn_matched[i] = TRUE;
                            break;       /* found match, so stop looping */
                        }
                }
                if (do_this_file) {  /* check if this is an excluded file */
                    for (i = 0; i < G.xfilespecs; i++)
                        if (match(G.filename, G.pxnames[i], uO.C_flag WISEP)) {
                            do_this_file = FALSE; /* ^-- ignore case or not? */
                            if (xn_matched)
                                xn_matched[i] = TRUE;
                            break;
                        }
                }
                if (do_this_file) {
                    if (store_info(__G))
                        ++j;            /* file is OK */
                    else
                        ++num_skipped;  /* unsupp. compression or encryption */
                }
            } /* end if (process_all_files) */

            members_processed++;

        } /* end while-loop (adding files to current block) */

        /* save position in central directory so can come back later */
        cd_bufstart = G.cur_zipfile_bufstart;
        cd_inptr = G.inptr;
        cd_incnt = G.incnt;

    /*-----------------------------------------------------------------------
        Second loop:  process files in current block, extracting or testing
        each one.
      -----------------------------------------------------------------------*/

        error = extract_or_test_entrylist(__G__ j,
                        &filnum, &num_bad_pwd, &old_extra_bytes,
#ifdef SET_DIR_ATTRIB
                        &num_dirs, &dirlist,
#endif
                        error_in_archive);
        if (error != PK_COOL) {
            if (error > error_in_archive)
                error_in_archive = error;
            /* ...and keep going (unless disk full or user break) */
            if (G.disk_full > 1 || error_in_archive == IZ_CTRLC ||
                error == PK_BOMB) {
                /* clear reached_end to signal premature stop ... */
                reached_end = FALSE;
                /* ... and cancel scanning the central directory */
                break;
            }
        }


        /*
         * Jump back to where we were in the central directory, then go and do
         * the next batch of files.
         */

#ifdef USE_STRM_INPUT
        zfseeko(G.zipfd, cd_bufstart, SEEK_SET);
        G.cur_zipfile_bufstart = zftello(G.zipfd);
#else /* !USE_STRM_INPUT */
        G.cur_zipfile_bufstart =
          zlseek(G.zipfd, cd_bufstart, SEEK_SET);
#endif /* ?USE_STRM_INPUT */
        read(G.zipfd, (char *)G.inbuf, INBUFSIZ);  /* been here before... */
        G.inptr = cd_inptr;
        G.incnt = cd_incnt;
        ++blknum;

#ifdef TEST
        printf("\ncd_bufstart = %ld (%.8lXh)\n", cd_bufstart, cd_bufstart);
        printf("cur_zipfile_bufstart = %ld (%.8lXh)\n", cur_zipfile_bufstart,
          cur_zipfile_bufstart);
        printf("inptr-inbuf = %d\n", G.inptr-G.inbuf);
        printf("incnt = %d\n\n", G.incnt);
#endif

    } /* end while-loop (blocks of files in central directory) */

/*---------------------------------------------------------------------------
    Process the list of deferred symlink extractions and finish up
    the symbolic links.
  ---------------------------------------------------------------------------*/

#ifdef SYMLINKS
    if (G.slink_last != NULL) {
        if (QCOND2)
            Info(slide, 0, ((char *)slide, LoadFarString(SymLnkDeferred)));
        while (G.slink_head != NULL) {
           set_deferred_symlink(__G__ G.slink_head);
           /* remove the processed entry from the chain and free its memory */
           G.slink_last = G.slink_head;
           G.slink_head = G.slink_last->next;
           free(G.slink_last);
       }
       G.slink_last = NULL;
    }
#endif /* SYMLINKS */

/*---------------------------------------------------------------------------
    Go back through saved list of directories, sort and set times/perms/UIDs
    and GIDs from the deepest level on up.
  ---------------------------------------------------------------------------*/

#ifdef SET_DIR_ATTRIB
    if (num_dirs > 0) {
        sorted_dirlist = (direntry **)malloc(num_dirs*sizeof(direntry *));
        if (sorted_dirlist == (direntry **)NULL) {
            Info(slide, 0x401, ((char *)slide,
              LoadFarString(DirlistSortNoMem)));
            while (dirlist != (direntry *)NULL) {
                direntry *d = dirlist;

                dirlist = dirlist->next;
                free(d);
            }
        } else {
            ulg ndirs_fail = 0;

            if (num_dirs == 1)
                sorted_dirlist[0] = dirlist;
            else {
                for (i = 0;  i < num_dirs;  ++i) {
                    sorted_dirlist[i] = dirlist;
                    dirlist = dirlist->next;
                }
                qsort((char *)sorted_dirlist, num_dirs, sizeof(direntry *),
                  dircomp);
            }

            Trace((stderr, "setting directory times/perms/attributes\n"));
            for (i = 0;  i < num_dirs;  ++i) {
                direntry *d = sorted_dirlist[i];

                Trace((stderr, "dir = %s\n", d->fn));
                if ((error = set_direc_attribs(__G__ d)) != PK_OK) {
                    ndirs_fail++;
                    Info(slide, 0x201, ((char *)slide,
                      LoadFarString(DirlistSetAttrFailed), d->fn));
                    if (!error_in_archive)
                        error_in_archive = error;
                }
                free(d);
            }
            free(sorted_dirlist);
            if (!uO.tflag && QCOND2) {
                if (ndirs_fail > 0)
                    Info(slide, 0, ((char *)slide,
                      LoadFarString(DirlistFailAttrSum), ndirs_fail));
            }
        }
    }
#endif /* SET_DIR_ATTRIB */

/*---------------------------------------------------------------------------
    Check for unmatched filespecs on command line and print warning if any
    found.  Free allocated memory.  (But suppress check when central dir
    scan was interrupted prematurely.)
  ---------------------------------------------------------------------------*/

    if (fn_matched) {
        if (reached_end) for (i = 0;  i < G.filespecs;  ++i)
            if (!fn_matched[i]) {
#ifdef DLL
                if (!G.redirect_data && !G.redirect_text)
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(FilenameNotMatched), G.pfnames[i]));
                else
                    setFileNotFound(__G);
#else
                Info(slide, 1, ((char *)slide,
                  LoadFarString(FilenameNotMatched), G.pfnames[i]));
#endif
                if (error_in_archive <= PK_WARN)
                    error_in_archive = PK_FIND;   /* some files not found */
            }
        free((zvoid *)fn_matched);
    }
    if (xn_matched) {
        if (reached_end) for (i = 0;  i < G.xfilespecs;  ++i)
            if (!xn_matched[i])
                Info(slide, 0x401, ((char *)slide,
                  LoadFarString(ExclFilenameNotMatched), G.pxnames[i]));
        free((zvoid *)xn_matched);
    }

/*---------------------------------------------------------------------------
    Now, all locally allocated memory has been released.  When the central
    directory processing has been interrupted prematurely, it is safe to
    return immediately.  All completeness checks and summary messages are
    skipped in this case.
  ---------------------------------------------------------------------------*/
    if (!reached_end)
        return error_in_archive;

/*---------------------------------------------------------------------------
    Double-check that we're back at the end-of-central-directory record, and
    print quick summary of results, if we were just testing the archive.  We
    send the summary to stdout so that people doing the testing in the back-
    ground and redirecting to a file can just do a "tail" on the output file.
  ---------------------------------------------------------------------------*/

#ifndef SFX
    if (no_endsig_found) {                      /* just to make sure */
        Info(slide, 0x401, ((char *)slide, LoadFarString(EndSigMsg)));
        Info(slide, 0x401, ((char *)slide, LoadFarString(ReportMsg)));
        if (!error_in_archive)       /* don't overwrite stronger error */
            error_in_archive = PK_WARN;
    }
#endif /* !SFX */
    if (uO.tflag) {
        ulg num = filnum - num_bad_pwd;

        if (uO.qflag < 2) {        /* GRR 930710:  was (uO.qflag == 1) */
            if (error_in_archive)
                Info(slide, 0, ((char *)slide, LoadFarString(ErrorInArchive),
                  (error_in_archive == PK_WARN)? "warning-" : "", G.zipfn));
            else if (num == 0L)
                Info(slide, 0, ((char *)slide, LoadFarString(ZeroFilesTested),
                  G.zipfn));
            else if (G.process_all_files && (num_skipped+num_bad_pwd == 0L))
                Info(slide, 0, ((char *)slide, LoadFarString(NoErrInCompData),
                  G.zipfn));
            else
                Info(slide, 0, ((char *)slide, LoadFarString(NoErrInTestedFiles)
                  , G.zipfn, num, (num==1L)? "":"s"));
            if (num_skipped > 0L)
                Info(slide, 0, ((char *)slide, LoadFarString(FilesSkipped),
                  num_skipped, (num_skipped==1L)? "":"s"));
#if CRYPT
            if (num_bad_pwd > 0L)
                Info(slide, 0, ((char *)slide, LoadFarString(FilesSkipBadPasswd)
                  , num_bad_pwd, (num_bad_pwd==1L)? "":"s"));
#endif /* CRYPT */
        }
    }

    /* give warning if files not tested or extracted (first condition can still
     * happen if zipfile is empty and no files specified on command line) */

    if ((filnum == 0) && error_in_archive <= PK_WARN) {
        if (num_skipped > 0L)
            error_in_archive = IZ_UNSUP; /* unsupport. compression/encryption */
        else
            error_in_archive = PK_FIND;  /* no files found at all */
    }
#if CRYPT
    else if ((filnum == num_bad_pwd) && error_in_archive <= PK_WARN)
        error_in_archive = IZ_BADPWD;    /* bad passwd => all files skipped */
#endif
    else if ((num_skipped > 0L) && error_in_archive <= PK_WARN)
        error_in_archive = IZ_UNSUP;     /* was PK_WARN; Jean-loup complained */
#if CRYPT
    else if ((num_bad_pwd > 0L) && !error_in_archive)
        error_in_archive = PK_WARN;
#endif

    return error_in_archive;

} /* end function extract_or_test_files() */