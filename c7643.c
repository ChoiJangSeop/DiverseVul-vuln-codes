static int extract_or_test_entrylist(__G__ numchunk,
                pfilnum, pnum_bad_pwd, pold_extra_bytes,
#ifdef SET_DIR_ATTRIB
                pnum_dirs, pdirlist,
#endif
                error_in_archive)    /* return PK-type error code */
    __GDEF
    unsigned numchunk;
    ulg *pfilnum;
    ulg *pnum_bad_pwd;
    zoff_t *pold_extra_bytes;
#ifdef SET_DIR_ATTRIB
    unsigned *pnum_dirs;
    direntry **pdirlist;
#endif
    int error_in_archive;
{
    unsigned i;
    int renamed, query;
    int skip_entry;
    zoff_t bufstart, inbuf_offset, request;
    int error, errcode;

/* possible values for local skip_entry flag: */
#define SKIP_NO         0       /* do not skip this entry */
#define SKIP_Y_EXISTING 1       /* skip this entry, do not overwrite file */
#define SKIP_Y_NONEXIST 2       /* skip this entry, do not create new file */

    /*-----------------------------------------------------------------------
        Second loop:  process files in current block, extracting or testing
        each one.
      -----------------------------------------------------------------------*/

    for (i = 0; i < numchunk; ++i) {
        (*pfilnum)++;   /* *pfilnum = i + blknum*DIR_BLKSIZ + 1; */
        G.pInfo = &G.info[i];
#ifdef NOVELL_BUG_FAILSAFE
        G.dne = FALSE;  /* assume file exists until stat() says otherwise */
#endif

        /* if the target position is not within the current input buffer
         * (either haven't yet read far enough, or (maybe) skipping back-
         * ward), skip to the target position and reset readbuf(). */

        /* seek_zipf(__G__ pInfo->offset);  */
        request = G.pInfo->offset + G.extra_bytes;
        inbuf_offset = request % INBUFSIZ;
        bufstart = request - inbuf_offset;

        Trace((stderr, "\ndebug: request = %ld, inbuf_offset = %ld\n",
          (long)request, (long)inbuf_offset));
        Trace((stderr,
          "debug: bufstart = %ld, cur_zipfile_bufstart = %ld\n",
          (long)bufstart, (long)G.cur_zipfile_bufstart));
        if (request < 0) {
            Info(slide, 0x401, ((char *)slide, LoadFarStringSmall(SeekMsg),
              G.zipfn, LoadFarString(ReportMsg)));
            error_in_archive = PK_ERR;
            if (*pfilnum == 1 && G.extra_bytes != 0L) {
                Info(slide, 0x401, ((char *)slide,
                  LoadFarString(AttemptRecompensate)));
                *pold_extra_bytes = G.extra_bytes;
                G.extra_bytes = 0L;
                request = G.pInfo->offset;  /* could also check if != 0 */
                inbuf_offset = request % INBUFSIZ;
                bufstart = request - inbuf_offset;
                Trace((stderr, "debug: request = %ld, inbuf_offset = %ld\n",
                  (long)request, (long)inbuf_offset));
                Trace((stderr,
                  "debug: bufstart = %ld, cur_zipfile_bufstart = %ld\n",
                  (long)bufstart, (long)G.cur_zipfile_bufstart));
                /* try again */
                if (request < 0) {
                    Trace((stderr,
                      "debug: recompensated request still < 0\n"));
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarStringSmall(SeekMsg),
                      G.zipfn, LoadFarString(ReportMsg)));
                    error_in_archive = PK_BADERR;
                    continue;
                }
            } else {
                error_in_archive = PK_BADERR;
                continue;  /* this one hosed; try next */
            }
        }

        if (bufstart != G.cur_zipfile_bufstart) {
            Trace((stderr, "debug: bufstart != cur_zipfile_bufstart\n"));
#ifdef USE_STRM_INPUT
            zfseeko(G.zipfd, bufstart, SEEK_SET);
            G.cur_zipfile_bufstart = zftello(G.zipfd);
#else /* !USE_STRM_INPUT */
            G.cur_zipfile_bufstart =
              zlseek(G.zipfd, bufstart, SEEK_SET);
#endif /* ?USE_STRM_INPUT */
            if ((G.incnt = read(G.zipfd, (char *)G.inbuf, INBUFSIZ)) <= 0)
            {
                Info(slide, 0x401, ((char *)slide, LoadFarString(OffsetMsg),
                  *pfilnum, "lseek", (long)bufstart));
                error_in_archive = PK_BADERR;
                continue;   /* can still do next file */
            }
            G.inptr = G.inbuf + (int)inbuf_offset;
            G.incnt -= (int)inbuf_offset;
        } else {
            G.incnt += (int)(G.inptr-G.inbuf) - (int)inbuf_offset;
            G.inptr = G.inbuf + (int)inbuf_offset;
        }

        /* should be in proper position now, so check for sig */
        if (readbuf(__G__ G.sig, 4) == 0) {  /* bad offset */
            Info(slide, 0x401, ((char *)slide, LoadFarString(OffsetMsg),
              *pfilnum, "EOF", (long)request));
            error_in_archive = PK_BADERR;
            continue;   /* but can still try next one */
        }
        if (memcmp(G.sig, local_hdr_sig, 4)) {
            Info(slide, 0x401, ((char *)slide, LoadFarString(OffsetMsg),
              *pfilnum, LoadFarStringSmall(LocalHdrSig), (long)request));
            /*
                GRRDUMP(G.sig, 4)
                GRRDUMP(local_hdr_sig, 4)
             */
            error_in_archive = PK_ERR;
            if ((*pfilnum == 1 && G.extra_bytes != 0L) ||
                (G.extra_bytes == 0L && *pold_extra_bytes != 0L)) {
                Info(slide, 0x401, ((char *)slide,
                  LoadFarString(AttemptRecompensate)));
                if (G.extra_bytes) {
                    *pold_extra_bytes = G.extra_bytes;
                    G.extra_bytes = 0L;
                } else
                    G.extra_bytes = *pold_extra_bytes; /* third attempt */
                if (((error = seek_zipf(__G__ G.pInfo->offset)) != PK_OK) ||
                    (readbuf(__G__ G.sig, 4) == 0)) {  /* bad offset */
                    if (error != PK_BADERR)
                      Info(slide, 0x401, ((char *)slide,
                        LoadFarString(OffsetMsg), *pfilnum, "EOF",
                        (long)request));
                    error_in_archive = PK_BADERR;
                    continue;   /* but can still try next one */
                }
                if (memcmp(G.sig, local_hdr_sig, 4)) {
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(OffsetMsg), *pfilnum,
                      LoadFarStringSmall(LocalHdrSig), (long)request));
                    error_in_archive = PK_BADERR;
                    continue;
                }
            } else
                continue;  /* this one hosed; try next */
        }
        if ((error = process_local_file_hdr(__G)) != PK_COOL) {
            Info(slide, 0x421, ((char *)slide, LoadFarString(BadLocalHdr),
              *pfilnum));
            error_in_archive = error;   /* only PK_EOF defined */
            continue;   /* can still try next one */
        }
#if (!defined(SFX) && defined(UNICODE_SUPPORT))
        if (((G.lrec.general_purpose_bit_flag & (1 << 11)) == (1 << 11))
            != (G.pInfo->GPFIsUTF8 != 0)) {
            if (QCOND2) {
#  ifdef SMALL_MEM
                char *temp_cfilnam = slide + (7 * (WSIZE>>3));

                zfstrcpy((char Far *)temp_cfilnam, G.pInfo->cfilname);
#    define  cFile_PrintBuf  temp_cfilnam
#  else
#    define  cFile_PrintBuf  G.pInfo->cfilname
#  endif
                Info(slide, 0x421, ((char *)slide,
                  LoadFarStringSmall2(GP11FlagsDiffer),
                  *pfilnum, FnFilter1(cFile_PrintBuf), G.pInfo->GPFIsUTF8));
#  undef    cFile_PrintBuf
            }
            if (error_in_archive < PK_WARN)
                error_in_archive = PK_WARN;
        }
#endif /* !SFX && UNICODE_SUPPORT */
        if ((error = do_string(__G__ G.lrec.filename_length, DS_FN_L)) !=
             PK_COOL)
        {
            if (error > error_in_archive)
                error_in_archive = error;
            if (error > PK_WARN) {
                Info(slide, 0x401, ((char *)slide, LoadFarString(FilNamMsg),
                  FnFilter1(G.filename), "local"));
                continue;   /* go on to next one */
            }
        }
        if (G.extra_field != (uch *)NULL) {
            free(G.extra_field);
            G.extra_field = (uch *)NULL;
        }
        if ((error =
             do_string(__G__ G.lrec.extra_field_length, EXTRA_FIELD)) != 0)
        {
            if (error > error_in_archive)
                error_in_archive = error;
            if (error > PK_WARN) {
                Info(slide, 0x401, ((char *)slide,
                  LoadFarString(ExtFieldMsg),
                  FnFilter1(G.filename), "local"));
                continue;   /* go on */
            }
        }
#ifndef SFX
        /* Filename consistency checks must come after reading in the local
         * extra field, so that a UTF-8 entry name e.f. block has already
         * been processed.
         */
        if (G.pInfo->cfilname != (char Far *)NULL) {
            if (zfstrcmp(G.pInfo->cfilname, G.filename) != 0) {
#  ifdef SMALL_MEM
                char *temp_cfilnam = slide + (7 * (WSIZE>>3));

                zfstrcpy((char Far *)temp_cfilnam, G.pInfo->cfilname);
#    define  cFile_PrintBuf  temp_cfilnam
#  else
#    define  cFile_PrintBuf  G.pInfo->cfilname
#  endif
                Info(slide, 0x401, ((char *)slide,
                  LoadFarStringSmall2(LvsCFNamMsg),
                  FnFilter2(cFile_PrintBuf), FnFilter1(G.filename)));
#  undef    cFile_PrintBuf
                zfstrcpy(G.filename, G.pInfo->cfilname);
                if (error_in_archive < PK_WARN)
                    error_in_archive = PK_WARN;
            }
            zffree(G.pInfo->cfilname);
            G.pInfo->cfilname = (char Far *)NULL;
        }
#endif /* !SFX */
        /* Size consistency checks must come after reading in the local extra
         * field, so that any Zip64 extension local e.f. block has already
         * been processed.
         */
        if (G.lrec.compression_method == STORED) {
            zusz_t csiz_decrypted = G.lrec.csize;

            if (G.pInfo->encrypted)
                csiz_decrypted -= 12;
            if (G.lrec.ucsize != csiz_decrypted) {
                Info(slide, 0x401, ((char *)slide,
                  LoadFarStringSmall2(WrnStorUCSizCSizDiff),
                  FnFilter1(G.filename),
                  FmZofft(G.lrec.ucsize, NULL, "u"),
                  FmZofft(csiz_decrypted, NULL, "u")));
                G.lrec.ucsize = csiz_decrypted;
                if (error_in_archive < PK_WARN)
                    error_in_archive = PK_WARN;
            }
        }

#if CRYPT
        if (G.pInfo->encrypted &&
            (error = decrypt(__G__ uO.pwdarg)) != PK_COOL) {
            if (error == PK_WARN) {
                if (!((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2)))
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(SkipIncorrectPasswd),
                      FnFilter1(G.filename)));
                ++(*pnum_bad_pwd);
            } else {  /* (error > PK_WARN) */
                if (error > error_in_archive)
                    error_in_archive = error;
                Info(slide, 0x401, ((char *)slide,
                  LoadFarString(SkipCannotGetPasswd),
                  FnFilter1(G.filename)));
            }
            continue;   /* go on to next file */
        }
#endif /* CRYPT */

        /*
         * just about to extract file:  if extracting to disk, check if
         * already exists, and if so, take appropriate action according to
         * fflag/uflag/overwrite_all/etc. (we couldn't do this in upper
         * loop because we don't store the possibly renamed filename[] in
         * info[])
         */
#ifdef DLL
        if (!uO.tflag && !uO.cflag && !G.redirect_data)
#else
        if (!uO.tflag && !uO.cflag)
#endif
        {
            renamed = FALSE;   /* user hasn't renamed output file yet */

startover:
            query = FALSE;
            skip_entry = SKIP_NO;
            /* for files from DOS FAT, check for use of backslash instead
             *  of slash as directory separator (bug in some zipper(s); so
             *  far, not a problem in HPFS, NTFS or VFAT systems)
             */
#ifndef SFX
            if (G.pInfo->hostnum == FS_FAT_ && !MBSCHR(G.filename, '/')) {
                char *p=G.filename;

                if (*p) do {
                    if (*p == '\\') {
                        if (!G.reported_backslash) {
                            Info(slide, 0x21, ((char *)slide,
                              LoadFarString(BackslashPathSep), G.zipfn));
                            G.reported_backslash = TRUE;
                            if (!error_in_archive)
                                error_in_archive = PK_WARN;
                        }
                        *p = '/';
                    }
                } while (*PREINCSTR(p));
            }
#endif /* !SFX */

            if (!renamed) {
               /* remove absolute path specs */
               if (G.filename[0] == '/') {
                   Info(slide, 0x401, ((char *)slide,
                        LoadFarString(AbsolutePathWarning),
                        FnFilter1(G.filename)));
                   if (!error_in_archive)
                       error_in_archive = PK_WARN;
                   do {
                       char *p = G.filename + 1;
                       do {
                           *(p-1) = *p;
                       } while (*p++ != '\0');
                   } while (G.filename[0] == '/');
               }
            }

            /* mapname can create dirs if not freshening or if renamed */
            error = mapname(__G__ renamed);
            if ((errcode = error & ~MPN_MASK) != PK_OK &&
                error_in_archive < errcode)
                error_in_archive = errcode;
            if ((errcode = error & MPN_MASK) > MPN_INF_TRUNC) {
                if (errcode == MPN_CREATED_DIR) {
#ifdef SET_DIR_ATTRIB
                    direntry *d_entry;

                    error = defer_dir_attribs(__G__ &d_entry);
                    if (d_entry == (direntry *)NULL) {
                        /* There may be no dir_attribs info available, or
                         * we have encountered a mem allocation error.
                         * In case of an error, report it and set program
                         * error state to warning level.
                         */
                        if (error) {
                            Info(slide, 0x401, ((char *)slide,
                                 LoadFarString(DirlistEntryNoMem)));
                            if (!error_in_archive)
                                error_in_archive = PK_WARN;
                        }
                    } else {
                        d_entry->next = (*pdirlist);
                        (*pdirlist) = d_entry;
                        ++(*pnum_dirs);
                    }
#endif /* SET_DIR_ATTRIB */
                } else if (errcode == MPN_VOL_LABEL) {
#ifdef DOS_OS2_W32
                    Info(slide, 0x401, ((char *)slide,
                      LoadFarString(SkipVolumeLabel),
                      FnFilter1(G.filename),
                      uO.volflag? "hard disk " : ""));
#else
                    Info(slide, 1, ((char *)slide,
                      LoadFarString(SkipVolumeLabel),
                      FnFilter1(G.filename), ""));
#endif
                } else if (errcode > MPN_INF_SKIP &&
                           error_in_archive < PK_ERR)
                    error_in_archive = PK_ERR;
                Trace((stderr, "mapname(%s) returns error code = %d\n",
                  FnFilter1(G.filename), error));
                continue;   /* go on to next file */
            }

#ifdef QDOS
            QFilename(__G__ G.filename);
#endif
            switch (check_for_newer(__G__ G.filename)) {
                case DOES_NOT_EXIST:
#ifdef NOVELL_BUG_FAILSAFE
                    G.dne = TRUE;   /* stat() says file DOES NOT EXIST */
#endif
                    /* freshen (no new files): skip unless just renamed */
                    if (uO.fflag && !renamed)
                        skip_entry = SKIP_Y_NONEXIST;
                    break;
                case EXISTS_AND_OLDER:
#ifdef UNIXBACKUP
                    if (!uO.B_flag)
#endif
                    {
                        if (IS_OVERWRT_NONE)
                            /* never overwrite:  skip file */
                            skip_entry = SKIP_Y_EXISTING;
                        else if (!IS_OVERWRT_ALL)
                            query = TRUE;
                    }
                    break;
                case EXISTS_AND_NEWER:             /* (or equal) */
#ifdef UNIXBACKUP
                    if ((!uO.B_flag && IS_OVERWRT_NONE) ||
#else
                    if (IS_OVERWRT_NONE ||
#endif
                        (uO.uflag && !renamed)) {
                        /* skip if update/freshen & orig name */
                        skip_entry = SKIP_Y_EXISTING;
                    } else {
#ifdef UNIXBACKUP
                        if (!IS_OVERWRT_ALL && !uO.B_flag)
#else
                        if (!IS_OVERWRT_ALL)
#endif
                            query = TRUE;
                    }
                    break;
            }
#ifdef VMS
            /* 2008-07-24 SMS.
             * On VMS, if the file name includes a version number,
             * and "-V" ("retain VMS version numbers", V_flag) is in
             * effect, then the VMS-specific code will handle any
             * conflicts with an existing file, making this query
             * redundant.  (Implicit "y" response here.)
             */
            if (query && uO.V_flag) {
                /* Not discarding file versions.  Look for one. */
                int cndx = strlen(G.filename) - 1;

                while ((cndx > 0) && (isdigit(G.filename[cndx])))
                    cndx--;
                if (G.filename[cndx] == ';')
                    /* File version found; skip the generic query,
                     * proceeding with its default response "y".
                     */
                    query = FALSE;
            }
#endif /* VMS */
            if (query) {
#ifdef WINDLL
                switch (G.lpUserFunctions->replace != NULL ?
                        (*G.lpUserFunctions->replace)(G.filename, FILNAMSIZ) :
                        IDM_REPLACE_NONE) {
                    case IDM_REPLACE_RENAME:
                        _ISO_INTERN(G.filename);
                        renamed = TRUE;
                        goto startover;
                    case IDM_REPLACE_ALL:
                        G.overwrite_mode = OVERWRT_ALWAYS;
                        /* FALL THROUGH, extract */
                    case IDM_REPLACE_YES:
                        break;
                    case IDM_REPLACE_NONE:
                        G.overwrite_mode = OVERWRT_NEVER;
                        /* FALL THROUGH, skip */
                    case IDM_REPLACE_NO:
                        skip_entry = SKIP_Y_EXISTING;
                        break;
                }
#else /* !WINDLL */
                extent fnlen;
reprompt:
                Info(slide, 0x81, ((char *)slide,
                  LoadFarString(ReplaceQuery),
                  FnFilter1(G.filename)));
                if (fgets(G.answerbuf, sizeof(G.answerbuf), stdin)
                    == (char *)NULL) {
                    Info(slide, 1, ((char *)slide,
                      LoadFarString(AssumeNone)));
                    *G.answerbuf = 'N';
                    if (!error_in_archive)
                        error_in_archive = 1;  /* not extracted:  warning */
                }
                switch (*G.answerbuf) {
                    case 'r':
                    case 'R':
                        do {
                            Info(slide, 0x81, ((char *)slide,
                              LoadFarString(NewNameQuery)));
                            fgets(G.filename, FILNAMSIZ, stdin);
                            /* usually get \n here:  better check for it */
                            fnlen = strlen(G.filename);
                            if (lastchar(G.filename, fnlen) == '\n')
                                G.filename[--fnlen] = '\0';
                        } while (fnlen == 0);
#ifdef WIN32  /* WIN32 fgets( ... , stdin) returns OEM coded strings */
                        _OEM_INTERN(G.filename);
#endif
                        renamed = TRUE;
                        goto startover;   /* sorry for a goto */
                    case 'A':   /* dangerous option:  force caps */
                        G.overwrite_mode = OVERWRT_ALWAYS;
                        /* FALL THROUGH, extract */
                    case 'y':
                    case 'Y':
                        break;
                    case 'N':
                        G.overwrite_mode = OVERWRT_NEVER;
                        /* FALL THROUGH, skip */
                    case 'n':
                        /* skip file */
                        skip_entry = SKIP_Y_EXISTING;
                        break;
                    case '\n':
                    case '\r':
                        /* Improve echo of '\n' and/or '\r'
                           (sizeof(G.answerbuf) == 10 (see globals.h), so
                           there is enough space for the provided text...) */
                        strcpy(G.answerbuf, "{ENTER}");
                        /* fall through ... */
                    default:
                        /* usually get \n here:  remove it for nice display
                           (fnlen can be re-used here, we are outside the
                           "enter new filename" loop) */
                        fnlen = strlen(G.answerbuf);
                        if (lastchar(G.answerbuf, fnlen) == '\n')
                            G.answerbuf[--fnlen] = '\0';
                        Info(slide, 1, ((char *)slide,
                          LoadFarString(InvalidResponse), G.answerbuf));
                        goto reprompt;   /* yet another goto? */
                } /* end switch (*answerbuf) */
#endif /* ?WINDLL */
            } /* end if (query) */
            if (skip_entry != SKIP_NO) {
#ifdef WINDLL
                if (skip_entry == SKIP_Y_EXISTING) {
                    /* report skipping of an existing entry */
                    Info(slide, 0, ((char *)slide,
                      ((IS_OVERWRT_NONE || !uO.uflag || renamed) ?
                       "Target file exists.  Skipping %s\n" :
                       "Target file newer.  Skipping %s\n"),
                      FnFilter1(G.filename)));
                }
#endif /* WINDLL */
                continue;
            }
        } /* end if (extracting to disk) */

#ifdef DLL
        if ((G.statreportcb != NULL) &&
            (*G.statreportcb)(__G__ UZ_ST_START_EXTRACT, G.zipfn,
                              G.filename, NULL)) {
            return IZ_CTRLC;        /* cancel operation by user request */
        }
#endif
#ifdef MACOS  /* MacOS is no preemptive OS, thus call event-handling by hand */
        UserStop();
#endif
#ifdef AMIGA
        G.filenote_slot = i;
#endif
        G.disk_full = 0;
        if ((error = extract_or_test_member(__G)) != PK_COOL) {
            if (error > error_in_archive)
                error_in_archive = error;       /* ...and keep going */
#ifdef DLL
            if (G.disk_full > 1 || error_in_archive == IZ_CTRLC) {
#else
            if (G.disk_full > 1) {
#endif
                return error_in_archive;        /* (unless disk full) */
            }
        }
#ifdef DLL
        if ((G.statreportcb != NULL) &&
            (*G.statreportcb)(__G__ UZ_ST_FINISH_MEMBER, G.zipfn,
                              G.filename, (zvoid *)&G.lrec.ucsize)) {
            return IZ_CTRLC;        /* cancel operation by user request */
        }
#endif
#ifdef MACOS  /* MacOS is no preemptive OS, thus call event-handling by hand */
        UserStop();
#endif
    } /* end for-loop (i:  files in current block) */

    return error_in_archive;

} /* end function extract_or_test_entrylist() */





/* wsize is used in extract_or_test_member() and UZbunzip2() */
#if (defined(DLL) && !defined(NO_SLIDE_REDIR))
#  define wsize G._wsize    /* wsize is a variable */
#else
#  define wsize WSIZE       /* wsize is a constant */
#endif

/***************************************/
/*  Function extract_or_test_member()  */
/***************************************/

static int extract_or_test_member(__G)    /* return PK-type error code */
     __GDEF
{
    char *nul="[empty] ", *txt="[text]  ", *bin="[binary]";
#ifdef CMS_MVS
    char *ebc="[ebcdic]";
#endif
    register int b;
    int r, error=PK_COOL;


/*---------------------------------------------------------------------------
    Initialize variables, buffers, etc.
  ---------------------------------------------------------------------------*/

    G.bits_left = 0;
    G.bitbuf = 0L;       /* unreduce and unshrink only */
    G.zipeof = 0;
    G.newfile = TRUE;
    G.crc32val = CRCVAL_INITIAL;

#ifdef SYMLINKS
    /* If file is a (POSIX-compatible) symbolic link and we are extracting
     * to disk, prepare to restore the link. */
    G.symlnk = (G.pInfo->symlink &&
                !uO.tflag && !uO.cflag && (G.lrec.ucsize > 0));
#endif /* SYMLINKS */

    if (uO.tflag) {
        if (!uO.qflag)
            Info(slide, 0, ((char *)slide, LoadFarString(ExtractMsg), "test",
              FnFilter1(G.filename), "", ""));
    } else {
#ifdef DLL
        if (uO.cflag && !G.redirect_data)
#else
        if (uO.cflag)
#endif
        {
#if (defined(OS2) && defined(__IBMC__) && (__IBMC__ >= 200))
            G.outfile = freopen("", "wb", stdout);   /* VAC++ ignores setmode */
#else
            G.outfile = stdout;
#endif
#ifdef DOS_FLX_NLM_OS2_W32
#if (defined(__HIGHC__) && !defined(FLEXOS))
            setmode(G.outfile, _BINARY);
#else /* !(defined(__HIGHC__) && !defined(FLEXOS)) */
            setmode(fileno(G.outfile), O_BINARY);
#endif /* ?(defined(__HIGHC__) && !defined(FLEXOS)) */
#           define NEWLINE "\r\n"
#else /* !DOS_FLX_NLM_OS2_W32 */
#           define NEWLINE "\n"
#endif /* ?DOS_FLX_NLM_OS2_W32 */
#ifdef VMS
            /* VMS:  required even for stdout! */
            if ((r = open_outfile(__G)) != 0)
                switch (r) {
                  case OPENOUT_SKIPOK:
                    return PK_OK;
                  case OPENOUT_SKIPWARN:
                    return PK_WARN;
                  default:
                    return PK_DISK;
                }
        } else if ((r = open_outfile(__G)) != 0)
            switch (r) {
              case OPENOUT_SKIPOK:
                return PK_OK;
              case OPENOUT_SKIPWARN:
                return PK_WARN;
              default:
                return PK_DISK;
            }
#else /* !VMS */
        } else if (open_outfile(__G))
            return PK_DISK;
#endif /* ?VMS */
    }

/*---------------------------------------------------------------------------
    Unpack the file.
  ---------------------------------------------------------------------------*/

    defer_leftover_input(__G);    /* so NEXTBYTE bounds check will work */
    switch (G.lrec.compression_method) {
        case STORED:
            if (!uO.tflag && QCOND2) {
#ifdef SYMLINKS
                if (G.symlnk)   /* can also be deflated, but rarer... */
                    Info(slide, 0, ((char *)slide, LoadFarString(ExtractMsg),
                      "link", FnFilter1(G.filename), "", ""));
                else
#endif /* SYMLINKS */
                Info(slide, 0, ((char *)slide, LoadFarString(ExtractMsg),
                  "extract", FnFilter1(G.filename),
                  (uO.aflag != 1 /* && G.pInfo->textfile==G.pInfo->textmode */)?
                  "" : (G.lrec.ucsize == 0L? nul : (G.pInfo->textfile? txt :
                  bin)), uO.cflag? NEWLINE : ""));
            }
#if (defined(DLL) && !defined(NO_SLIDE_REDIR))
            if (G.redirect_slide) {
                wsize = G.redirect_size; redirSlide = G.redirect_buffer;
            } else {
                wsize = WSIZE; redirSlide = slide;
            }
#endif
            G.outptr = redirSlide;
            G.outcnt = 0L;
            while ((b = NEXTBYTE) != EOF) {
                *G.outptr++ = (uch)b;
                if (++G.outcnt == wsize) {
                    error = flush(__G__ redirSlide, G.outcnt, 0);
                    G.outptr = redirSlide;
                    G.outcnt = 0L;
                    if (error != PK_COOL || G.disk_full) break;
                }
            }
            if (G.outcnt) {        /* flush final (partial) buffer */
                r = flush(__G__ redirSlide, G.outcnt, 0);
                if (error < r) error = r;
            }
            break;

#ifndef SFX
#ifndef LZW_CLEAN
        case SHRUNK:
            if (!uO.tflag && QCOND2) {
                Info(slide, 0, ((char *)slide, LoadFarString(ExtractMsg),
                  LoadFarStringSmall(Unshrink), FnFilter1(G.filename),
                  (uO.aflag != 1 /* && G.pInfo->textfile==G.pInfo->textmode */)?
                  "" : (G.pInfo->textfile? txt : bin), uO.cflag? NEWLINE : ""));
            }
            if ((r = unshrink(__G)) != PK_COOL) {
                if (r < PK_DISK) {
                    if ((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2))
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarStringSmall(ErrUnzipFile), r == PK_MEM3 ?
                          LoadFarString(NotEnoughMem) :
                          LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(Unshrink),
                          FnFilter1(G.filename)));
                    else
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarStringSmall(ErrUnzipNoFile), r == PK_MEM3 ?
                          LoadFarString(NotEnoughMem) :
                          LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(Unshrink)));
                }
                error = r;
            }
            break;
#endif /* !LZW_CLEAN */

#ifndef COPYRIGHT_CLEAN
        case REDUCED1:
        case REDUCED2:
        case REDUCED3:
        case REDUCED4:
            if (!uO.tflag && QCOND2) {
                Info(slide, 0, ((char *)slide, LoadFarString(ExtractMsg),
                  "unreduc", FnFilter1(G.filename),
                  (uO.aflag != 1 /* && G.pInfo->textfile==G.pInfo->textmode */)?
                  "" : (G.pInfo->textfile? txt : bin), uO.cflag? NEWLINE : ""));
            }
            if ((r = unreduce(__G)) != PK_COOL) {
                /* unreduce() returns only PK_COOL, PK_DISK, or IZ_CTRLC */
                error = r;
            }
            break;
#endif /* !COPYRIGHT_CLEAN */

        case IMPLODED:
            if (!uO.tflag && QCOND2) {
                Info(slide, 0, ((char *)slide, LoadFarString(ExtractMsg),
                  "explod", FnFilter1(G.filename),
                  (uO.aflag != 1 /* && G.pInfo->textfile==G.pInfo->textmode */)?
                  "" : (G.pInfo->textfile? txt : bin), uO.cflag? NEWLINE : ""));
            }
            if ((r = explode(__G)) != 0) {
                if (r == 5) { /* treat 5 specially */
                    int warning = ((zusz_t)G.used_csize <= G.lrec.csize);

                    if ((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2))
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarString(LengthMsg),
                          "", warning ? "warning" : "error",
                          FmZofft(G.used_csize, NULL, NULL),
                          FmZofft(G.lrec.ucsize, NULL, "u"),
                          warning ? "  " : "",
                          FmZofft(G.lrec.csize, NULL, "u"),
                          " [", FnFilter1(G.filename), "]"));
                    else
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarString(LengthMsg),
                          "\n", warning ? "warning" : "error",
                          FmZofft(G.used_csize, NULL, NULL),
                          FmZofft(G.lrec.ucsize, NULL, "u"),
                          warning ? "  " : "",
                          FmZofft(G.lrec.csize, NULL, "u"),
                          "", "", "."));
                    error = warning ? PK_WARN : PK_ERR;
                } else if (r < PK_DISK) {
                    if ((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2))
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarStringSmall(ErrUnzipFile), r == 3?
                          LoadFarString(NotEnoughMem) :
                          LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(Explode),
                          FnFilter1(G.filename)));
                    else
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarStringSmall(ErrUnzipNoFile), r == 3?
                          LoadFarString(NotEnoughMem) :
                          LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(Explode)));
                    error = ((r == 3) ? PK_MEM3 : PK_ERR);
                } else {
                    error = r;
                }
            }
            break;
#endif /* !SFX */

        case DEFLATED:
#ifdef USE_DEFLATE64
        case ENHDEFLATED:
#endif
            if (!uO.tflag && QCOND2) {
                Info(slide, 0, ((char *)slide, LoadFarString(ExtractMsg),
                  "inflat", FnFilter1(G.filename),
                  (uO.aflag != 1 /* && G.pInfo->textfile==G.pInfo->textmode */)?
                  "" : (G.pInfo->textfile? txt : bin), uO.cflag? NEWLINE : ""));
            }
#ifndef USE_ZLIB  /* zlib's function is called inflate(), too */
#  define UZinflate inflate
#endif
            if ((r = UZinflate(__G__
                               (G.lrec.compression_method == ENHDEFLATED)))
                != 0) {
                if (r < PK_DISK) {
                    if ((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2))
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarStringSmall(ErrUnzipFile), r == 3?
                          LoadFarString(NotEnoughMem) :
                          LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(Inflate),
                          FnFilter1(G.filename)));
                    else
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarStringSmall(ErrUnzipNoFile), r == 3?
                          LoadFarString(NotEnoughMem) :
                          LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(Inflate)));
                    error = ((r == 3) ? PK_MEM3 : PK_ERR);
                } else {
                    error = r;
                }
            }
            break;

#ifdef USE_BZIP2
        case BZIPPED:
            if (!uO.tflag && QCOND2) {
                Info(slide, 0, ((char *)slide, LoadFarString(ExtractMsg),
                  "bunzipp", FnFilter1(G.filename),
                  (uO.aflag != 1 /* && G.pInfo->textfile==G.pInfo->textmode */)?
                  "" : (G.pInfo->textfile? txt : bin), uO.cflag? NEWLINE : ""));
            }
            if ((r = UZbunzip2(__G)) != 0) {
                if (r < PK_DISK) {
                    if ((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2))
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarStringSmall(ErrUnzipFile), r == 3?
                          LoadFarString(NotEnoughMem) :
                          LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(BUnzip),
                          FnFilter1(G.filename)));
                    else
                        Info(slide, 0x401, ((char *)slide,
                          LoadFarStringSmall(ErrUnzipNoFile), r == 3?
                          LoadFarString(NotEnoughMem) :
                          LoadFarString(InvalidComprData),
                          LoadFarStringSmall2(BUnzip)));
                    error = ((r == 3) ? PK_MEM3 : PK_ERR);
                } else {
                    error = r;
                }
            }
            break;
#endif /* USE_BZIP2 */

        default:   /* should never get to this point */
            Info(slide, 0x401, ((char *)slide,
              LoadFarString(FileUnknownCompMethod), FnFilter1(G.filename)));
            /* close and delete file before return? */
            undefer_input(__G);
            return PK_WARN;

    } /* end switch (compression method) */

/*---------------------------------------------------------------------------
    Close the file and set its date and time (not necessarily in that order),
    and make sure the CRC checked out OK.  Logical-AND the CRC for 64-bit
    machines (redundant on 32-bit machines).
  ---------------------------------------------------------------------------*/

#ifdef VMS                  /* VMS:  required even for stdout! (final flush) */
    if (!uO.tflag)           /* don't close NULL file */
        close_outfile(__G);
#else
#ifdef DLL
    if (!uO.tflag && (!uO.cflag || G.redirect_data)) {
        if (G.redirect_data)
            FINISH_REDIRECT();
        else
            close_outfile(__G);
    }
#else
    if (!uO.tflag && !uO.cflag)   /* don't close NULL file or stdout */
        close_outfile(__G);
#endif
#endif /* VMS */

            /* GRR: CONVERT close_outfile() TO NON-VOID:  CHECK FOR ERRORS! */


    if (G.disk_full) {            /* set by flush() */
        if (G.disk_full > 1) {
#if (defined(DELETE_IF_FULL) && defined(HAVE_UNLINK))
            /* delete the incomplete file if we can */
            if (unlink(G.filename) != 0)
                Trace((stderr, "extract.c:  could not delete %s\n",
                  FnFilter1(G.filename)));
#else
            /* warn user about the incomplete file */
            Info(slide, 0x421, ((char *)slide, LoadFarString(FileTruncated),
              FnFilter1(G.filename)));
#endif
            error = PK_DISK;
        } else {
            error = PK_WARN;
        }
    }

    if (error > PK_WARN) {/* don't print redundant CRC error if error already */
        undefer_input(__G);
        return error;
    }
    if (G.crc32val != G.lrec.crc32) {
        /* if quiet enough, we haven't output the filename yet:  do it */
        if ((uO.tflag && uO.qflag) || (!uO.tflag && !QCOND2))
            Info(slide, 0x401, ((char *)slide, "%-22s ",
              FnFilter1(G.filename)));
        Info(slide, 0x401, ((char *)slide, LoadFarString(BadCRC), G.crc32val,
          G.lrec.crc32));
#if CRYPT
        if (G.pInfo->encrypted)
            Info(slide, 0x401, ((char *)slide, LoadFarString(MaybeBadPasswd)));
#endif
        error = PK_ERR;
    } else if (uO.tflag) {
#ifndef SFX
        if (G.extra_field) {
            if ((r = TestExtraField(__G__ G.extra_field,
                                    G.lrec.extra_field_length)) > error)
                error = r;
        } else
#endif /* !SFX */
        if (!uO.qflag)
            Info(slide, 0, ((char *)slide, " OK\n"));
    } else {
        if (QCOND2 && !error)   /* GRR:  is stdout reset to text mode yet? */
            Info(slide, 0, ((char *)slide, "\n"));
    }

    undefer_input(__G);
    return error;

} /* end function extract_or_test_member() */