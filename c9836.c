static int parse_cmdline_encoder(int argc, char **argv,
                                 opj_cparameters_t *parameters,
                                 img_fol_t *img_fol, raw_cparameters_t *raw_cp, char *indexfilename,
                                 size_t indexfilename_size,
                                 int* pOutFramerate)
{
    OPJ_UINT32 i, j;
    int totlen, c;
    opj_option_t long_option[] = {
        {"cinema2K", REQ_ARG, NULL, 'w'},
        {"cinema4K", NO_ARG, NULL, 'y'},
        {"ImgDir", REQ_ARG, NULL, 'z'},
        {"TP", REQ_ARG, NULL, 'u'},
        {"SOP", NO_ARG, NULL, 'S'},
        {"EPH", NO_ARG, NULL, 'E'},
        {"OutFor", REQ_ARG, NULL, 'O'},
        {"POC", REQ_ARG, NULL, 'P'},
        {"ROI", REQ_ARG, NULL, 'R'},
        {"jpip", NO_ARG, NULL, 'J'},
        {"mct", REQ_ARG, NULL, 'Y'},
        {"IMF", REQ_ARG, NULL, 'Z'}
    };

    /* parse the command line */
    const char optlist[] = "i:o:r:q:n:b:c:t:p:s:SEM:x:R:d:T:If:P:C:F:u:JY:"
#ifdef USE_JPWL
                           "W:"
#endif /* USE_JPWL */
                           "h";

    totlen = sizeof(long_option);
    img_fol->set_out_format = 0;
    raw_cp->rawWidth = 0;

    do {
        c = opj_getopt_long(argc, argv, optlist, long_option, totlen);
        if (c == -1) {
            break;
        }
        switch (c) {
        case 'i': {         /* input file */
            char *infile = opj_optarg;
            parameters->decod_format = get_file_format(infile);
            switch (parameters->decod_format) {
            case PGX_DFMT:
            case PXM_DFMT:
            case BMP_DFMT:
            case TIF_DFMT:
            case RAW_DFMT:
            case RAWL_DFMT:
            case TGA_DFMT:
            case PNG_DFMT:
                break;
            default:
                fprintf(stderr,
                        "[ERROR] Unknown input file format: %s \n"
                        "        Known file formats are *.pnm, *.pgm, *.ppm, *.pgx, *png, *.bmp, *.tif, *.raw or *.tga\n",
                        infile);
                return 1;
            }
            if (opj_strcpy_s(parameters->infile, sizeof(parameters->infile), infile) != 0) {
                return 1;
            }
        }
        break;

        /* ----------------------------------------------------- */

        case 'o': {         /* output file */
            char *outfile = opj_optarg;
            parameters->cod_format = get_file_format(outfile);
            switch (parameters->cod_format) {
            case J2K_CFMT:
            case JP2_CFMT:
                break;
            default:
                fprintf(stderr,
                        "Unknown output format image %s [only *.j2k, *.j2c or *.jp2]!! \n", outfile);
                return 1;
            }
            if (opj_strcpy_s(parameters->outfile, sizeof(parameters->outfile),
                             outfile) != 0) {
                return 1;
            }
        }
        break;

        /* ----------------------------------------------------- */
        case 'O': {         /* output format */
            char outformat[50];
            char *of = opj_optarg;
            sprintf(outformat, ".%s", of);
            img_fol->set_out_format = 1;
            parameters->cod_format = get_file_format(outformat);
            switch (parameters->cod_format) {
            case J2K_CFMT:
            case JP2_CFMT:
                img_fol->out_format = opj_optarg;
                break;
            default:
                fprintf(stderr, "Unknown output format image [only j2k, j2c, jp2]!! \n");
                return 1;
            }
        }
        break;


        /* ----------------------------------------------------- */


        case 'r': {         /* rates rates/distorsion */
            char *s = opj_optarg;
            parameters->tcp_numlayers = 0;
            while (sscanf(s, "%f", &parameters->tcp_rates[parameters->tcp_numlayers]) ==
                    1) {
                parameters->tcp_numlayers++;
                while (*s && *s != ',') {
                    s++;
                }
                if (!*s) {
                    break;
                }
                s++;
            }
            parameters->cp_disto_alloc = 1;
        }
        break;

        /* ----------------------------------------------------- */


        case 'F': {         /* Raw image format parameters */
            OPJ_BOOL wrong = OPJ_FALSE;
            char *substr1;
            char *substr2;
            char *sep;
            char signo;
            int width, height, bitdepth, ncomp;
            OPJ_UINT32 len;
            OPJ_BOOL raw_signed = OPJ_FALSE;
            substr2 = strchr(opj_optarg, '@');
            if (substr2 == NULL) {
                len = (OPJ_UINT32) strlen(opj_optarg);
            } else {
                len = (OPJ_UINT32)(substr2 - opj_optarg);
                substr2++; /* skip '@' character */
            }
            substr1 = (char*) malloc((len + 1) * sizeof(char));
            if (substr1 == NULL) {
                return 1;
            }
            memcpy(substr1, opj_optarg, len);
            substr1[len] = '\0';
            if (sscanf(substr1, "%d,%d,%d,%d,%c", &width, &height, &ncomp, &bitdepth,
                       &signo) == 5) {
                if (signo == 's') {
                    raw_signed = OPJ_TRUE;
                } else if (signo == 'u') {
                    raw_signed = OPJ_FALSE;
                } else {
                    wrong = OPJ_TRUE;
                }
            } else {
                wrong = OPJ_TRUE;
            }
            if (!wrong) {
                int compno;
                int lastdx = 1;
                int lastdy = 1;
                raw_cp->rawWidth = width;
                raw_cp->rawHeight = height;
                raw_cp->rawComp = ncomp;
                raw_cp->rawBitDepth = bitdepth;
                raw_cp->rawSigned  = raw_signed;
                raw_cp->rawComps = (raw_comp_cparameters_t*) malloc(((OPJ_UINT32)(
                                       ncomp)) * sizeof(raw_comp_cparameters_t));
                if (raw_cp->rawComps == NULL) {
                    free(substr1);
                    return 1;
                }
                for (compno = 0; compno < ncomp && !wrong; compno++) {
                    if (substr2 == NULL) {
                        raw_cp->rawComps[compno].dx = lastdx;
                        raw_cp->rawComps[compno].dy = lastdy;
                    } else {
                        int dx, dy;
                        sep = strchr(substr2, ':');
                        if (sep == NULL) {
                            if (sscanf(substr2, "%dx%d", &dx, &dy) == 2) {
                                lastdx = dx;
                                lastdy = dy;
                                raw_cp->rawComps[compno].dx = dx;
                                raw_cp->rawComps[compno].dy = dy;
                                substr2 = NULL;
                            } else {
                                wrong = OPJ_TRUE;
                            }
                        } else {
                            if (sscanf(substr2, "%dx%d:%s", &dx, &dy, substr2) == 3) {
                                raw_cp->rawComps[compno].dx = dx;
                                raw_cp->rawComps[compno].dy = dy;
                            } else {
                                wrong = OPJ_TRUE;
                            }
                        }
                    }
                }
            }
            free(substr1);
            if (wrong) {
                fprintf(stderr, "\nError: invalid raw image parameters\n");
                fprintf(stderr, "Please use the Format option -F:\n");
                fprintf(stderr,
                        "-F <width>,<height>,<ncomp>,<bitdepth>,{s,u}@<dx1>x<dy1>:...:<dxn>x<dyn>\n");
                fprintf(stderr,
                        "If subsampling is omitted, 1x1 is assumed for all components\n");
                fprintf(stderr,
                        "Example: -i image.raw -o image.j2k -F 512,512,3,8,u@1x1:2x2:2x2\n");
                fprintf(stderr, "         for raw 512x512 image with 4:2:0 subsampling\n");
                fprintf(stderr, "Aborting.\n");
                return 1;
            }
        }
        break;

        /* ----------------------------------------------------- */

        case 'q': {         /* add fixed_quality */
            char *s = opj_optarg;
            while (sscanf(s, "%f", &parameters->tcp_distoratio[parameters->tcp_numlayers])
                    == 1) {
                parameters->tcp_numlayers++;
                while (*s && *s != ',') {
                    s++;
                }
                if (!*s) {
                    break;
                }
                s++;
            }
            parameters->cp_fixed_quality = 1;
        }
        break;

        /* dda */
        /* ----------------------------------------------------- */

        case 'f': {         /* mod fixed_quality (before : -q) */
            int *row = NULL, *col = NULL;
            OPJ_UINT32 numlayers = 0, numresolution = 0, matrix_width = 0;

            char *s = opj_optarg;
            sscanf(s, "%u", &numlayers);
            s++;
            if (numlayers > 9) {
                s++;
            }

            parameters->tcp_numlayers = (int)numlayers;
            numresolution = (OPJ_UINT32)parameters->numresolution;
            matrix_width = numresolution * 3;
            parameters->cp_matrice = (int *) malloc(sizeof(int) * numlayers * matrix_width);
            if (parameters->cp_matrice == NULL) {
                return 1;
            }
            s = s + 2;

            for (i = 0; i < numlayers; i++) {
                row = &parameters->cp_matrice[i * matrix_width];
                col = row;
                parameters->tcp_rates[i] = 1;
                sscanf(s, "%d,", &col[0]);
                s += 2;
                if (col[0] > 9) {
                    s++;
                }
                col[1] = 0;
                col[2] = 0;
                for (j = 1; j < numresolution; j++) {
                    col += 3;
                    sscanf(s, "%d,%d,%d", &col[0], &col[1], &col[2]);
                    s += 6;
                    if (col[0] > 9) {
                        s++;
                    }
                    if (col[1] > 9) {
                        s++;
                    }
                    if (col[2] > 9) {
                        s++;
                    }
                }
                if (i < numlayers - 1) {
                    s++;
                }
            }
            parameters->cp_fixed_alloc = 1;
        }
        break;

        /* ----------------------------------------------------- */

        case 't': {         /* tiles */
            sscanf(opj_optarg, "%d,%d", &parameters->cp_tdx, &parameters->cp_tdy);
            parameters->tile_size_on = OPJ_TRUE;
        }
        break;

        /* ----------------------------------------------------- */

        case 'n': {         /* resolution */
            sscanf(opj_optarg, "%d", &parameters->numresolution);
        }
        break;

        /* ----------------------------------------------------- */
        case 'c': {         /* precinct dimension */
            char sep;
            int res_spec = 0;

            char *s = opj_optarg;
            int ret;
            do {
                sep = 0;
                ret = sscanf(s, "[%d,%d]%c", &parameters->prcw_init[res_spec],
                             &parameters->prch_init[res_spec], &sep);
                if (!(ret == 2 && sep == 0) && !(ret == 3 && sep == ',')) {
                    fprintf(stderr, "\nError: could not parse precinct dimension: '%s' %x\n", s,
                            sep);
                    fprintf(stderr, "Example: -i lena.raw -o lena.j2k -c [128,128],[128,128]\n");
                    return 1;
                }
                parameters->csty |= 0x01;
                res_spec++;
                s = strpbrk(s, "]") + 2;
            } while (sep == ',');
            parameters->res_spec = res_spec;
        }
        break;

        /* ----------------------------------------------------- */

        case 'b': {         /* code-block dimension */
            int cblockw_init = 0, cblockh_init = 0;
            sscanf(opj_optarg, "%d,%d", &cblockw_init, &cblockh_init);
            if (cblockw_init > 1024 || cblockw_init < 4 ||
                    cblockh_init > 1024 || cblockh_init < 4 ||
                    cblockw_init * cblockh_init > 4096) {
                fprintf(stderr,
                        "!! Size of code_block error (option -b) !!\n\nRestriction :\n"
                        "    * width*height<=4096\n    * 4<=width,height<= 1024\n\n");
                return 1;
            }
            parameters->cblockw_init = cblockw_init;
            parameters->cblockh_init = cblockh_init;
        }
        break;

        /* ----------------------------------------------------- */

        case 'x': {         /* creation of index file */
            if (opj_strcpy_s(indexfilename, indexfilename_size, opj_optarg) != 0) {
                return 1;
            }
            /* FIXME ADE INDEX >> */
            fprintf(stderr,
                    "[WARNING] Index file generation is currently broken.\n"
                    "          '-x' option ignored.\n");
            /* << FIXME ADE INDEX */
        }
        break;

        /* ----------------------------------------------------- */

        case 'p': {         /* progression order */
            char progression[4];

            strncpy(progression, opj_optarg, 4);
            parameters->prog_order = give_progression(progression);
            if (parameters->prog_order == -1) {
                fprintf(stderr, "Unrecognized progression order "
                        "[LRCP, RLCP, RPCL, PCRL, CPRL] !!\n");
                return 1;
            }
        }
        break;

        /* ----------------------------------------------------- */

        case 's': {         /* subsampling factor */
            if (sscanf(opj_optarg, "%d,%d", &parameters->subsampling_dx,
                       &parameters->subsampling_dy) != 2) {
                fprintf(stderr, "'-s' sub-sampling argument error !  [-s dx,dy]\n");
                return 1;
            }
        }
        break;

        /* ----------------------------------------------------- */

        case 'd': {         /* coordonnate of the reference grid */
            if (sscanf(opj_optarg, "%d,%d", &parameters->image_offset_x0,
                       &parameters->image_offset_y0) != 2) {
                fprintf(stderr, "-d 'coordonnate of the reference grid' argument "
                        "error !! [-d x0,y0]\n");
                return 1;
            }
        }
        break;

        /* ----------------------------------------------------- */

        case 'h':           /* display an help description */
            encode_help_display();
            return 1;

        /* ----------------------------------------------------- */

        case 'P': {         /* POC */
            int numpocs = 0;        /* number of progression order change (POC) default 0 */
            opj_poc_t *POC = NULL;  /* POC : used in case of Progression order change */

            char *s = opj_optarg;
            POC = parameters->POC;

            while (sscanf(s, "T%u=%u,%u,%u,%u,%u,%4s", &POC[numpocs].tile,
                          &POC[numpocs].resno0, &POC[numpocs].compno0,
                          &POC[numpocs].layno1, &POC[numpocs].resno1,
                          &POC[numpocs].compno1, POC[numpocs].progorder) == 7) {
                POC[numpocs].prg1 = give_progression(POC[numpocs].progorder);
                numpocs++;
                while (*s && *s != '/') {
                    s++;
                }
                if (!*s) {
                    break;
                }
                s++;
            }
            parameters->numpocs = (OPJ_UINT32)numpocs;
        }
        break;

        /* ------------------------------------------------------ */

        case 'S': {         /* SOP marker */
            parameters->csty |= 0x02;
        }
        break;

        /* ------------------------------------------------------ */

        case 'E': {         /* EPH marker */
            parameters->csty |= 0x04;
        }
        break;

        /* ------------------------------------------------------ */

        case 'M': {         /* Mode switch pas tous au point !! */
            int value = 0;
            if (sscanf(opj_optarg, "%d", &value) == 1) {
                for (i = 0; i <= 5; i++) {
                    int cache = value & (1 << i);
                    if (cache) {
                        parameters->mode |= (1 << i);
                    }
                }
            }
        }
        break;

        /* ------------------------------------------------------ */

        case 'R': {         /* ROI */
            if (sscanf(opj_optarg, "c=%d,U=%d", &parameters->roi_compno,
                       &parameters->roi_shift) != 2) {
                fprintf(stderr, "ROI error !! [-ROI c='compno',U='shift']\n");
                return 1;
            }
        }
        break;

        /* ------------------------------------------------------ */

        case 'T': {         /* Tile offset */
            if (sscanf(opj_optarg, "%d,%d", &parameters->cp_tx0,
                       &parameters->cp_ty0) != 2) {
                fprintf(stderr, "-T 'tile offset' argument error !! [-T X0,Y0]");
                return 1;
            }
        }
        break;

        /* ------------------------------------------------------ */

        case 'C': {         /* add a comment */
            parameters->cp_comment = (char*)malloc(strlen(opj_optarg) + 1);
            if (parameters->cp_comment) {
                strcpy(parameters->cp_comment, opj_optarg);
            }
        }
        break;


        /* ------------------------------------------------------ */

        case 'I': {         /* reversible or not */
            parameters->irreversible = 1;
        }
        break;

        /* ------------------------------------------------------ */

        case 'u': {         /* Tile part generation*/
            parameters->tp_flag = opj_optarg[0];
            parameters->tp_on = 1;
        }
        break;

        /* ------------------------------------------------------ */

        case 'z': {         /* Image Directory path */
            img_fol->imgdirpath = (char*)malloc(strlen(opj_optarg) + 1);
            if (img_fol->imgdirpath == NULL) {
                return 1;
            }
            strcpy(img_fol->imgdirpath, opj_optarg);
            img_fol->set_imgdir = 1;
        }
        break;

        /* ------------------------------------------------------ */

        case 'w': {         /* Digital Cinema 2K profile compliance*/
            int fps = 0;
            sscanf(opj_optarg, "%d", &fps);
            if (fps == 24) {
                parameters->rsiz = OPJ_PROFILE_CINEMA_2K;
                parameters->max_comp_size = OPJ_CINEMA_24_COMP;
                parameters->max_cs_size = OPJ_CINEMA_24_CS;
            } else if (fps == 48) {
                parameters->rsiz = OPJ_PROFILE_CINEMA_2K;
                parameters->max_comp_size = OPJ_CINEMA_48_COMP;
                parameters->max_cs_size = OPJ_CINEMA_48_CS;
            } else {
                fprintf(stderr, "Incorrect value!! must be 24 or 48\n");
                return 1;
            }
            fprintf(stdout, "CINEMA 2K profile activated\n"
                    "Other options specified could be overridden\n");

        }
        break;

        /* ------------------------------------------------------ */

        case 'y': {         /* Digital Cinema 4K profile compliance*/
            parameters->rsiz = OPJ_PROFILE_CINEMA_4K;
            fprintf(stdout, "CINEMA 4K profile activated\n"
                    "Other options specified could be overridden\n");
        }
        break;

        /* ------------------------------------------------------ */

        case 'Z': {         /* IMF profile*/
            int mainlevel = 0;
            int sublevel = 0;
            int profile = 0;
            int framerate = 0;
            const char* msg =
                "Wrong value for -IMF. Should be "
                "<PROFILE>[,mainlevel=X][,sublevel=Y][,framerate=FPS] where <PROFILE> is one "
                "of 2K/4K/8K/2K_R/4K_R/8K_R.\n";
            char* comma;

            comma = strstr(opj_optarg, ",mainlevel=");
            if (comma && sscanf(comma + 1, "mainlevel=%d", &mainlevel) != 1) {
                fprintf(stderr, "%s", msg);
                return 1;
            }

            comma = strstr(opj_optarg, ",sublevel=");
            if (comma && sscanf(comma + 1, "sublevel=%d", &sublevel) != 1) {
                fprintf(stderr, "%s", msg);
                return 1;
            }

            comma = strstr(opj_optarg, ",framerate=");
            if (comma && sscanf(comma + 1, "framerate=%d", &framerate) != 1) {
                fprintf(stderr, "%s", msg);
                return 1;
            }

            comma = strchr(opj_optarg, ',');
            if (comma != NULL) {
                *comma = 0;
            }

            if (strcmp(opj_optarg, "2K") == 0) {
                profile = OPJ_PROFILE_IMF_2K;
            } else if (strcmp(opj_optarg, "4K") == 0) {
                profile = OPJ_PROFILE_IMF_4K;
            } else if (strcmp(opj_optarg, "8K") == 0) {
                profile = OPJ_PROFILE_IMF_8K;
            } else if (strcmp(opj_optarg, "2K_R") == 0) {
                profile = OPJ_PROFILE_IMF_2K_R;
            } else if (strcmp(opj_optarg, "4K_R") == 0) {
                profile = OPJ_PROFILE_IMF_4K_R;
            } else if (strcmp(opj_optarg, "8K_R") == 0) {
                profile = OPJ_PROFILE_IMF_8K_R;
            } else {
                fprintf(stderr, "%s", msg);
                return 1;
            }

            if (!(mainlevel >= 0 && mainlevel <= 15)) {
                /* Voluntarily rough validation. More fine grained done in library */
                fprintf(stderr, "Invalid mainlevel value.\n");
                return 1;
            }
            if (!(sublevel >= 0 && sublevel <= 15)) {
                /* Voluntarily rough validation. More fine grained done in library */
                fprintf(stderr, "Invalid sublevel value.\n");
                return 1;
            }
            parameters->rsiz = (OPJ_UINT16)(profile | (sublevel << 4) | mainlevel);

            fprintf(stdout, "IMF profile activated\n"
                    "Other options specified could be overridden\n");

            if (pOutFramerate) {
                *pOutFramerate = framerate;
            }
            if (framerate > 0 && sublevel > 0 && sublevel <= 9) {
                const int limitMBitsSec[] = {
                    0,
                    OPJ_IMF_SUBLEVEL_1_MBITSSEC,
                    OPJ_IMF_SUBLEVEL_2_MBITSSEC,
                    OPJ_IMF_SUBLEVEL_3_MBITSSEC,
                    OPJ_IMF_SUBLEVEL_4_MBITSSEC,
                    OPJ_IMF_SUBLEVEL_5_MBITSSEC,
                    OPJ_IMF_SUBLEVEL_6_MBITSSEC,
                    OPJ_IMF_SUBLEVEL_7_MBITSSEC,
                    OPJ_IMF_SUBLEVEL_8_MBITSSEC,
                    OPJ_IMF_SUBLEVEL_9_MBITSSEC
                };
                parameters->max_cs_size = limitMBitsSec[sublevel] * (1000 * 1000 / 8) /
                                          framerate;
                fprintf(stdout, "Setting max codestream size to %d bytes.\n",
                        parameters->max_cs_size);
            }
        }
        break;

        /* ------------------------------------------------------ */

        case 'Y': {         /* Shall we do an MCT ? 0:no_mct;1:rgb->ycc;2:custom mct (-m option required)*/
            int mct_mode = 0;
            sscanf(opj_optarg, "%d", &mct_mode);
            if (mct_mode < 0 || mct_mode > 2) {
                fprintf(stderr,
                        "MCT incorrect value!! Current accepted values are 0, 1 or 2.\n");
                return 1;
            }
            parameters->tcp_mct = (char) mct_mode;
        }
        break;

        /* ------------------------------------------------------ */


        case 'm': {         /* mct input file */
            char *lFilename = opj_optarg;
            char *lMatrix;
            char *lCurrentPtr ;
            float *lCurrentDoublePtr;
            float *lSpace;
            int *l_int_ptr;
            int lNbComp = 0, lTotalComp, lMctComp, i2;
            size_t lStrLen, lStrFread;

            /* Open file */
            FILE * lFile = fopen(lFilename, "r");
            if (lFile == NULL) {
                return 1;
            }

            /* Set size of file and read its content*/
            fseek(lFile, 0, SEEK_END);
            lStrLen = (size_t)ftell(lFile);
            fseek(lFile, 0, SEEK_SET);
            lMatrix = (char *) malloc(lStrLen + 1);
            if (lMatrix == NULL) {
                fclose(lFile);
                return 1;
            }
            lStrFread = fread(lMatrix, 1, lStrLen, lFile);
            fclose(lFile);
            if (lStrLen != lStrFread) {
                free(lMatrix);
                return 1;
            }

            lMatrix[lStrLen] = 0;
            lCurrentPtr = lMatrix;

            /* replace ',' by 0 */
            while (*lCurrentPtr != 0) {
                if (*lCurrentPtr == ' ') {
                    *lCurrentPtr = 0;
                    ++lNbComp;
                }
                ++lCurrentPtr;
            }
            ++lNbComp;
            lCurrentPtr = lMatrix;

            lNbComp = (int)(sqrt(4 * lNbComp + 1) / 2. - 0.5);
            lMctComp = lNbComp * lNbComp;
            lTotalComp = lMctComp + lNbComp;
            lSpace = (float *) malloc((size_t)lTotalComp * sizeof(float));
            if (lSpace == NULL) {
                free(lMatrix);
                return 1;
            }
            lCurrentDoublePtr = lSpace;
            for (i2 = 0; i2 < lMctComp; ++i2) {
                lStrLen = strlen(lCurrentPtr) + 1;
                *lCurrentDoublePtr++ = (float) atof(lCurrentPtr);
                lCurrentPtr += lStrLen;
            }

            l_int_ptr = (int*) lCurrentDoublePtr;
            for (i2 = 0; i2 < lNbComp; ++i2) {
                lStrLen = strlen(lCurrentPtr) + 1;
                *l_int_ptr++ = atoi(lCurrentPtr);
                lCurrentPtr += lStrLen;
            }

            /* TODO should not be here ! */
            opj_set_MCT(parameters, lSpace, (int *)(lSpace + lMctComp),
                        (OPJ_UINT32)lNbComp);

            /* Free memory*/
            free(lSpace);
            free(lMatrix);
        }
        break;


            /* ------------------------------------------------------ */

            /* UniPG>> */
#ifdef USE_JPWL
        /* ------------------------------------------------------ */

        case 'W': {         /* JPWL capabilities switched on */
            char *token = NULL;
            int hprot, pprot, sens, addr, size, range;

            /* we need to enable indexing */
            if (!indexfilename || !*indexfilename) {
                if (opj_strcpy_s(indexfilename, indexfilename_size,
                                 JPWL_PRIVATEINDEX_NAME) != 0) {
                    return 1;
                }
            }

            /* search for different protection methods */

            /* break the option in comma points and parse the result */
            token = strtok(opj_optarg, ",");
            while (token != NULL) {

                /* search header error protection method */
                if (*token == 'h') {

                    static int tile = 0, tilespec = 0, lasttileno = 0;

                    hprot = 1; /* predefined method */

                    if (sscanf(token, "h=%d", &hprot) == 1) {
                        /* Main header, specified */
                        if (!((hprot == 0) || (hprot == 1) || (hprot == 16) || (hprot == 32) ||
                                ((hprot >= 37) && (hprot <= 128)))) {
                            fprintf(stderr, "ERROR -> invalid main header protection method h = %d\n",
                                    hprot);
                            return 1;
                        }
                        parameters->jpwl_hprot_MH = hprot;

                    } else if (sscanf(token, "h%d=%d", &tile, &hprot) == 2) {
                        /* Tile part header, specified */
                        if (!((hprot == 0) || (hprot == 1) || (hprot == 16) || (hprot == 32) ||
                                ((hprot >= 37) && (hprot <= 128)))) {
                            fprintf(stderr, "ERROR -> invalid tile part header protection method h = %d\n",
                                    hprot);
                            return 1;
                        }
                        if (tile < 0) {
                            fprintf(stderr,
                                    "ERROR -> invalid tile part number on protection method t = %d\n", tile);
                            return 1;
                        }
                        if (tilespec < JPWL_MAX_NO_TILESPECS) {
                            parameters->jpwl_hprot_TPH_tileno[tilespec] = lasttileno = tile;
                            parameters->jpwl_hprot_TPH[tilespec++] = hprot;
                        }

                    } else if (sscanf(token, "h%d", &tile) == 1) {
                        /* Tile part header, unspecified */
                        if (tile < 0) {
                            fprintf(stderr,
                                    "ERROR -> invalid tile part number on protection method t = %d\n", tile);
                            return 1;
                        }
                        if (tilespec < JPWL_MAX_NO_TILESPECS) {
                            parameters->jpwl_hprot_TPH_tileno[tilespec] = lasttileno = tile;
                            parameters->jpwl_hprot_TPH[tilespec++] = hprot;
                        }


                    } else if (!strcmp(token, "h")) {
                        /* Main header, unspecified */
                        parameters->jpwl_hprot_MH = hprot;

                    } else {
                        fprintf(stderr, "ERROR -> invalid protection method selection = %s\n", token);
                        return 1;
                    };

                }

                /* search packet error protection method */
                if (*token == 'p') {

                    static int pack = 0, tile = 0, packspec = 0;

                    pprot = 1; /* predefined method */

                    if (sscanf(token, "p=%d", &pprot) == 1) {
                        /* Method for all tiles and all packets */
                        if (!((pprot == 0) || (pprot == 1) || (pprot == 16) || (pprot == 32) ||
                                ((pprot >= 37) && (pprot <= 128)))) {
                            fprintf(stderr, "ERROR -> invalid default packet protection method p = %d\n",
                                    pprot);
                            return 1;
                        }
                        parameters->jpwl_pprot_tileno[0] = 0;
                        parameters->jpwl_pprot_packno[0] = 0;
                        parameters->jpwl_pprot[0] = pprot;

                    } else if (sscanf(token, "p%d=%d", &tile, &pprot) == 2) {
                        /* method specified from that tile on */
                        if (!((pprot == 0) || (pprot == 1) || (pprot == 16) || (pprot == 32) ||
                                ((pprot >= 37) && (pprot <= 128)))) {
                            fprintf(stderr, "ERROR -> invalid packet protection method p = %d\n", pprot);
                            return 1;
                        }
                        if (tile < 0) {
                            fprintf(stderr,
                                    "ERROR -> invalid tile part number on protection method p = %d\n", tile);
                            return 1;
                        }
                        if (packspec < JPWL_MAX_NO_PACKSPECS) {
                            parameters->jpwl_pprot_tileno[packspec] = tile;
                            parameters->jpwl_pprot_packno[packspec] = 0;
                            parameters->jpwl_pprot[packspec++] = pprot;
                        }

                    } else if (sscanf(token, "p%d:%d=%d", &tile, &pack, &pprot) == 3) {
                        /* method fully specified from that tile and that packet on */
                        if (!((pprot == 0) || (pprot == 1) || (pprot == 16) || (pprot == 32) ||
                                ((pprot >= 37) && (pprot <= 128)))) {
                            fprintf(stderr, "ERROR -> invalid packet protection method p = %d\n", pprot);
                            return 1;
                        }
                        if (tile < 0) {
                            fprintf(stderr,
                                    "ERROR -> invalid tile part number on protection method p = %d\n", tile);
                            return 1;
                        }
                        if (pack < 0) {
                            fprintf(stderr, "ERROR -> invalid packet number on protection method p = %d\n",
                                    pack);
                            return 1;
                        }
                        if (packspec < JPWL_MAX_NO_PACKSPECS) {
                            parameters->jpwl_pprot_tileno[packspec] = tile;
                            parameters->jpwl_pprot_packno[packspec] = pack;
                            parameters->jpwl_pprot[packspec++] = pprot;
                        }

                    } else if (sscanf(token, "p%d:%d", &tile, &pack) == 2) {
                        /* default method from that tile and that packet on */
                        if (!((pprot == 0) || (pprot == 1) || (pprot == 16) || (pprot == 32) ||
                                ((pprot >= 37) && (pprot <= 128)))) {
                            fprintf(stderr, "ERROR -> invalid packet protection method p = %d\n", pprot);
                            return 1;
                        }
                        if (tile < 0) {
                            fprintf(stderr,
                                    "ERROR -> invalid tile part number on protection method p = %d\n", tile);
                            return 1;
                        }
                        if (pack < 0) {
                            fprintf(stderr, "ERROR -> invalid packet number on protection method p = %d\n",
                                    pack);
                            return 1;
                        }
                        if (packspec < JPWL_MAX_NO_PACKSPECS) {
                            parameters->jpwl_pprot_tileno[packspec] = tile;
                            parameters->jpwl_pprot_packno[packspec] = pack;
                            parameters->jpwl_pprot[packspec++] = pprot;
                        }

                    } else if (sscanf(token, "p%d", &tile) == 1) {
                        /* default from a tile on */
                        if (tile < 0) {
                            fprintf(stderr,
                                    "ERROR -> invalid tile part number on protection method p = %d\n", tile);
                            return 1;
                        }
                        if (packspec < JPWL_MAX_NO_PACKSPECS) {
                            parameters->jpwl_pprot_tileno[packspec] = tile;
                            parameters->jpwl_pprot_packno[packspec] = 0;
                            parameters->jpwl_pprot[packspec++] = pprot;
                        }


                    } else if (!strcmp(token, "p")) {
                        /* all default */
                        parameters->jpwl_pprot_tileno[0] = 0;
                        parameters->jpwl_pprot_packno[0] = 0;
                        parameters->jpwl_pprot[0] = pprot;

                    } else {
                        fprintf(stderr, "ERROR -> invalid protection method selection = %s\n", token);
                        return 1;
                    };

                }

                /* search sensitivity method */
                if (*token == 's') {

                    static int tile = 0, tilespec = 0, lasttileno = 0;

                    sens = 0; /* predefined: relative error */

                    if (sscanf(token, "s=%d", &sens) == 1) {
                        /* Main header, specified */
                        if ((sens < -1) || (sens > 7)) {
                            fprintf(stderr, "ERROR -> invalid main header sensitivity method s = %d\n",
                                    sens);
                            return 1;
                        }
                        parameters->jpwl_sens_MH = sens;

                    } else if (sscanf(token, "s%d=%d", &tile, &sens) == 2) {
                        /* Tile part header, specified */
                        if ((sens < -1) || (sens > 7)) {
                            fprintf(stderr, "ERROR -> invalid tile part header sensitivity method s = %d\n",
                                    sens);
                            return 1;
                        }
                        if (tile < 0) {
                            fprintf(stderr,
                                    "ERROR -> invalid tile part number on sensitivity method t = %d\n", tile);
                            return 1;
                        }
                        if (tilespec < JPWL_MAX_NO_TILESPECS) {
                            parameters->jpwl_sens_TPH_tileno[tilespec] = lasttileno = tile;
                            parameters->jpwl_sens_TPH[tilespec++] = sens;
                        }

                    } else if (sscanf(token, "s%d", &tile) == 1) {
                        /* Tile part header, unspecified */
                        if (tile < 0) {
                            fprintf(stderr,
                                    "ERROR -> invalid tile part number on sensitivity method t = %d\n", tile);
                            return 1;
                        }
                        if (tilespec < JPWL_MAX_NO_TILESPECS) {
                            parameters->jpwl_sens_TPH_tileno[tilespec] = lasttileno = tile;
                            parameters->jpwl_sens_TPH[tilespec++] = hprot;
                        }

                    } else if (!strcmp(token, "s")) {
                        /* Main header, unspecified */
                        parameters->jpwl_sens_MH = sens;

                    } else {
                        fprintf(stderr, "ERROR -> invalid sensitivity method selection = %s\n", token);
                        return 1;
                    };

                    parameters->jpwl_sens_size = 2; /* 2 bytes for default size */
                }

                /* search addressing size */
                if (*token == 'a') {


                    addr = 0; /* predefined: auto */

                    if (sscanf(token, "a=%d", &addr) == 1) {
                        /* Specified */
                        if ((addr != 0) && (addr != 2) && (addr != 4)) {
                            fprintf(stderr, "ERROR -> invalid addressing size a = %d\n", addr);
                            return 1;
                        }
                        parameters->jpwl_sens_addr = addr;

                    } else if (!strcmp(token, "a")) {
                        /* default */
                        parameters->jpwl_sens_addr = addr; /* auto for default size */

                    } else {
                        fprintf(stderr, "ERROR -> invalid addressing selection = %s\n", token);
                        return 1;
                    };

                }

                /* search sensitivity size */
                if (*token == 'z') {


                    size = 1; /* predefined: 1 byte */

                    if (sscanf(token, "z=%d", &size) == 1) {
                        /* Specified */
                        if ((size != 0) && (size != 1) && (size != 2)) {
                            fprintf(stderr, "ERROR -> invalid sensitivity size z = %d\n", size);
                            return 1;
                        }
                        parameters->jpwl_sens_size = size;

                    } else if (!strcmp(token, "a")) {
                        /* default */
                        parameters->jpwl_sens_size = size; /* 1 for default size */

                    } else {
                        fprintf(stderr, "ERROR -> invalid size selection = %s\n", token);
                        return 1;
                    };

                }

                /* search range method */
                if (*token == 'g') {


                    range = 0; /* predefined: 0 (packet) */

                    if (sscanf(token, "g=%d", &range) == 1) {
                        /* Specified */
                        if ((range < 0) || (range > 3)) {
                            fprintf(stderr, "ERROR -> invalid sensitivity range method g = %d\n", range);
                            return 1;
                        }
                        parameters->jpwl_sens_range = range;

                    } else if (!strcmp(token, "g")) {
                        /* default */
                        parameters->jpwl_sens_range = range;

                    } else {
                        fprintf(stderr, "ERROR -> invalid range selection = %s\n", token);
                        return 1;
                    };

                }

                /* next token or bust */
                token = strtok(NULL, ",");
            };


            /* some info */
            fprintf(stdout, "Info: JPWL capabilities enabled\n");
            parameters->jpwl_epc_on = OPJ_TRUE;

        }
        break;
#endif /* USE_JPWL */
        /* <<UniPG */
        /* ------------------------------------------------------ */

        case 'J': {         /* jpip on */
            parameters->jpip_on = OPJ_TRUE;
        }
        break;
        /* ------------------------------------------------------ */


        default:
            fprintf(stderr, "[WARNING] An invalid option has been ignored\n");
            break;
        }
    } while (c != -1);

    if (img_fol->set_imgdir == 1) {
        if (!(parameters->infile[0] == 0)) {
            fprintf(stderr, "[ERROR] options -ImgDir and -i cannot be used together !!\n");
            return 1;
        }
        if (img_fol->set_out_format == 0) {
            fprintf(stderr,
                    "[ERROR] When -ImgDir is used, -OutFor <FORMAT> must be used !!\n");
            fprintf(stderr, "Only one format allowed! Valid formats are j2k and jp2!!\n");
            return 1;
        }
        if (!((parameters->outfile[0] == 0))) {
            fprintf(stderr, "[ERROR] options -ImgDir and -o cannot be used together !!\n");
            fprintf(stderr, "Specify OutputFormat using -OutFor<FORMAT> !!\n");
            return 1;
        }
    } else {
        if ((parameters->infile[0] == 0) || (parameters->outfile[0] == 0)) {
            fprintf(stderr, "[ERROR] Required parameters are missing\n"
                    "Example: %s -i image.pgm -o image.j2k\n", argv[0]);
            fprintf(stderr, "   Help: %s -h\n", argv[0]);
            return 1;
        }
    }

    if ((parameters->decod_format == RAW_DFMT && raw_cp->rawWidth == 0)
            || (parameters->decod_format == RAWL_DFMT && raw_cp->rawWidth == 0)) {
        fprintf(stderr, "[ERROR] invalid raw image parameters\n");
        fprintf(stderr, "Please use the Format option -F:\n");
        fprintf(stderr,
                "-F rawWidth,rawHeight,rawComp,rawBitDepth,s/u (Signed/Unsigned)\n");
        fprintf(stderr, "Example: -i lena.raw -o lena.j2k -F 512,512,3,8,u\n");
        fprintf(stderr, "Aborting\n");
        return 1;
    }

    if ((parameters->cp_disto_alloc || parameters->cp_fixed_alloc ||
            parameters->cp_fixed_quality)
            && (!(parameters->cp_disto_alloc ^ parameters->cp_fixed_alloc ^
                  parameters->cp_fixed_quality))) {
        fprintf(stderr, "[ERROR] options -r -q and -f cannot be used together !!\n");
        return 1;
    }               /* mod fixed_quality */


    /* if no rate entered, lossless by default */
    /* Note: post v2.2.0, this is no longer necessary, but for released */
    /* versions at the time of writing, this is needed to avoid crashes */
    if (parameters->tcp_numlayers == 0) {
        parameters->tcp_rates[0] = 0;
        parameters->tcp_numlayers++;
        parameters->cp_disto_alloc = 1;
    }

    if ((parameters->cp_tx0 > parameters->image_offset_x0) ||
            (parameters->cp_ty0 > parameters->image_offset_y0)) {
        fprintf(stderr,
                "[ERROR] Tile offset dimension is unnappropriate --> TX0(%d)<=IMG_X0(%d) TYO(%d)<=IMG_Y0(%d) \n",
                parameters->cp_tx0, parameters->image_offset_x0, parameters->cp_ty0,
                parameters->image_offset_y0);
        return 1;
    }

    for (i = 0; i < parameters->numpocs; i++) {
        if (parameters->POC[i].prg == -1) {
            fprintf(stderr,
                    "Unrecognized progression order in option -P (POC n %d) [LRCP, RLCP, RPCL, PCRL, CPRL] !!\n",
                    i + 1);
        }
    }

    /* If subsampled image is provided, automatically disable MCT */
    if (((parameters->decod_format == RAW_DFMT) ||
            (parameters->decod_format == RAWL_DFMT))
            && (((raw_cp->rawComp > 1) && ((raw_cp->rawComps[1].dx > 1) ||
                                           (raw_cp->rawComps[1].dy > 1)))
                || ((raw_cp->rawComp > 2) && ((raw_cp->rawComps[2].dx > 1) ||
                        (raw_cp->rawComps[2].dy > 1)))
               )) {
        parameters->tcp_mct = 0;
    }

    return 0;

}