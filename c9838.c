int main(int argc, char **argv)
{

    opj_cparameters_t parameters;   /* compression parameters */

    opj_stream_t *l_stream = 00;
    opj_codec_t* l_codec = 00;
    opj_image_t *image = NULL;
    raw_cparameters_t raw_cp;
    OPJ_SIZE_T num_compressed_files = 0;

    char indexfilename[OPJ_PATH_LEN];   /* index file name */

    unsigned int i, num_images, imageno;
    img_fol_t img_fol;
    dircnt_t *dirptr = NULL;

    int ret = 0;

    OPJ_BOOL bSuccess;
    OPJ_BOOL bUseTiles = OPJ_FALSE; /* OPJ_TRUE */
    OPJ_UINT32 l_nb_tiles = 4;
    int framerate = 0;
    OPJ_FLOAT64 t = opj_clock();

    /* set encoding parameters to default values */
    opj_set_default_encoder_parameters(&parameters);

    /* Initialize indexfilename and img_fol */
    *indexfilename = 0;
    memset(&img_fol, 0, sizeof(img_fol_t));

    /* raw_cp initialization */
    raw_cp.rawBitDepth = 0;
    raw_cp.rawComp = 0;
    raw_cp.rawComps = 0;
    raw_cp.rawHeight = 0;
    raw_cp.rawSigned = 0;
    raw_cp.rawWidth = 0;

    /* parse input and get user encoding parameters */
    parameters.tcp_mct = (char)
                         255; /* This will be set later according to the input image or the provided option */
    if (parse_cmdline_encoder(argc, argv, &parameters, &img_fol, &raw_cp,
                              indexfilename, sizeof(indexfilename), &framerate) == 1) {
        ret = 1;
        goto fin;
    }

    /* Read directory if necessary */
    if (img_fol.set_imgdir == 1) {
        num_images = get_num_images(img_fol.imgdirpath);
        dirptr = (dircnt_t*)malloc(sizeof(dircnt_t));
        if (dirptr) {
            dirptr->filename_buf = (char*)malloc(num_images * OPJ_PATH_LEN * sizeof(
                    char)); /* Stores at max 10 image file names*/
            dirptr->filename = (char**) malloc(num_images * sizeof(char*));
            if (!dirptr->filename_buf) {
                ret = 0;
                goto fin;
            }
            for (i = 0; i < num_images; i++) {
                dirptr->filename[i] = dirptr->filename_buf + i * OPJ_PATH_LEN;
            }
        }
        if (load_images(dirptr, img_fol.imgdirpath) == 1) {
            ret = 0;
            goto fin;
        }
        if (num_images == 0) {
            fprintf(stdout, "Folder is empty\n");
            ret = 0;
            goto fin;
        }
    } else {
        num_images = 1;
    }
    /*Encoding image one by one*/
    for (imageno = 0; imageno < num_images; imageno++) {
        image = NULL;
        fprintf(stderr, "\n");

        if (img_fol.set_imgdir == 1) {
            if (get_next_file((int)imageno, dirptr, &img_fol, &parameters)) {
                fprintf(stderr, "skipping file...\n");
                continue;
            }
        }

        switch (parameters.decod_format) {
        case PGX_DFMT:
            break;
        case PXM_DFMT:
            break;
        case BMP_DFMT:
            break;
        case TIF_DFMT:
            break;
        case RAW_DFMT:
        case RAWL_DFMT:
            break;
        case TGA_DFMT:
            break;
        case PNG_DFMT:
            break;
        default:
            fprintf(stderr, "skipping file...\n");
            continue;
        }

        /* decode the source image */
        /* ----------------------- */

        switch (parameters.decod_format) {
        case PGX_DFMT:
            image = pgxtoimage(parameters.infile, &parameters);
            if (!image) {
                fprintf(stderr, "Unable to load pgx file\n");
                ret = 1;
                goto fin;
            }
            break;

        case PXM_DFMT:
            image = pnmtoimage(parameters.infile, &parameters);
            if (!image) {
                fprintf(stderr, "Unable to load pnm file\n");
                ret = 1;
                goto fin;
            }
            break;

        case BMP_DFMT:
            image = bmptoimage(parameters.infile, &parameters);
            if (!image) {
                fprintf(stderr, "Unable to load bmp file\n");
                ret = 1;
                goto fin;
            }
            break;

#ifdef OPJ_HAVE_LIBTIFF
        case TIF_DFMT:
            image = tiftoimage(parameters.infile, &parameters);
            if (!image) {
                fprintf(stderr, "Unable to load tiff file\n");
                ret = 1;
                goto fin;
            }
            break;
#endif /* OPJ_HAVE_LIBTIFF */

        case RAW_DFMT:
            image = rawtoimage(parameters.infile, &parameters, &raw_cp);
            if (!image) {
                fprintf(stderr, "Unable to load raw file\n");
                ret = 1;
                goto fin;
            }
            break;

        case RAWL_DFMT:
            image = rawltoimage(parameters.infile, &parameters, &raw_cp);
            if (!image) {
                fprintf(stderr, "Unable to load raw file\n");
                ret = 1;
                goto fin;
            }
            break;

        case TGA_DFMT:
            image = tgatoimage(parameters.infile, &parameters);
            if (!image) {
                fprintf(stderr, "Unable to load tga file\n");
                ret = 1;
                goto fin;
            }
            break;

#ifdef OPJ_HAVE_LIBPNG
        case PNG_DFMT:
            image = pngtoimage(parameters.infile, &parameters);
            if (!image) {
                fprintf(stderr, "Unable to load png file\n");
                ret = 1;
                goto fin;
            }
            break;
#endif /* OPJ_HAVE_LIBPNG */
        }

        /* Can happen if input file is TIFF or PNG
        * and OPJ_HAVE_LIBTIF or OPJ_HAVE_LIBPNG is undefined
        */
        if (!image) {
            fprintf(stderr, "Unable to load file: got no image\n");
            ret = 1;
            goto fin;
        }

        /* Decide if MCT should be used */
        if (parameters.tcp_mct == (char)
                255) { /* mct mode has not been set in commandline */
            parameters.tcp_mct = (image->numcomps >= 3) ? 1 : 0;
        } else {            /* mct mode has been set in commandline */
            if ((parameters.tcp_mct == 1) && (image->numcomps < 3)) {
                fprintf(stderr, "RGB->YCC conversion cannot be used:\n");
                fprintf(stderr, "Input image has less than 3 components\n");
                ret = 1;
                goto fin;
            }
            if ((parameters.tcp_mct == 2) && (!parameters.mct_data)) {
                fprintf(stderr, "Custom MCT has been set but no array-based MCT\n");
                fprintf(stderr, "has been provided. Aborting.\n");
                ret = 1;
                goto fin;
            }
        }

        if (OPJ_IS_IMF(parameters.rsiz) && framerate > 0) {
            const int mainlevel = OPJ_GET_IMF_MAINLEVEL(parameters.rsiz);
            if (mainlevel > 0 && mainlevel <= OPJ_IMF_MAINLEVEL_MAX) {
                const int limitMSamplesSec[] = {
                    0,
                    OPJ_IMF_MAINLEVEL_1_MSAMPLESEC,
                    OPJ_IMF_MAINLEVEL_2_MSAMPLESEC,
                    OPJ_IMF_MAINLEVEL_3_MSAMPLESEC,
                    OPJ_IMF_MAINLEVEL_4_MSAMPLESEC,
                    OPJ_IMF_MAINLEVEL_5_MSAMPLESEC,
                    OPJ_IMF_MAINLEVEL_6_MSAMPLESEC,
                    OPJ_IMF_MAINLEVEL_7_MSAMPLESEC,
                    OPJ_IMF_MAINLEVEL_8_MSAMPLESEC,
                    OPJ_IMF_MAINLEVEL_9_MSAMPLESEC,
                    OPJ_IMF_MAINLEVEL_10_MSAMPLESEC,
                    OPJ_IMF_MAINLEVEL_11_MSAMPLESEC
                };
                OPJ_UINT32 avgcomponents = image->numcomps;
                double msamplespersec;
                if (image->numcomps == 3 &&
                        image->comps[1].dx == 2 &&
                        image->comps[1].dy == 2) {
                    avgcomponents = 2;
                }
                msamplespersec = (double)image->x1 * image->y1 * avgcomponents * framerate /
                                 1e6;
                if (msamplespersec > limitMSamplesSec[mainlevel]) {
                    fprintf(stderr,
                            "Warning: MSamples/sec is %f, whereas limit is %d.\n",
                            msamplespersec,
                            limitMSamplesSec[mainlevel]);
                }
            }
        }

        /* encode the destination image */
        /* ---------------------------- */

        switch (parameters.cod_format) {
        case J2K_CFMT: { /* JPEG-2000 codestream */
            /* Get a decoder handle */
            l_codec = opj_create_compress(OPJ_CODEC_J2K);
            break;
        }
        case JP2_CFMT: { /* JPEG 2000 compressed image data */
            /* Get a decoder handle */
            l_codec = opj_create_compress(OPJ_CODEC_JP2);
            break;
        }
        default:
            fprintf(stderr, "skipping file..\n");
            opj_stream_destroy(l_stream);
            continue;
        }

        /* catch events using our callbacks and give a local context */
        opj_set_info_handler(l_codec, info_callback, 00);
        opj_set_warning_handler(l_codec, warning_callback, 00);
        opj_set_error_handler(l_codec, error_callback, 00);

        if (bUseTiles) {
            parameters.cp_tx0 = 0;
            parameters.cp_ty0 = 0;
            parameters.tile_size_on = OPJ_TRUE;
            parameters.cp_tdx = 512;
            parameters.cp_tdy = 512;
        }
        if (! opj_setup_encoder(l_codec, &parameters, image)) {
            fprintf(stderr, "failed to encode image: opj_setup_encoder\n");
            opj_destroy_codec(l_codec);
            opj_image_destroy(image);
            ret = 1;
            goto fin;
        }

        /* open a byte stream for writing and allocate memory for all tiles */
        l_stream = opj_stream_create_default_file_stream(parameters.outfile, OPJ_FALSE);
        if (! l_stream) {
            ret = 1;
            goto fin;
        }

        /* encode the image */
        bSuccess = opj_start_compress(l_codec, image, l_stream);
        if (!bSuccess)  {
            fprintf(stderr, "failed to encode image: opj_start_compress\n");
        }
        if (bSuccess && bUseTiles) {
            OPJ_BYTE *l_data;
            OPJ_UINT32 l_data_size = 512 * 512 * 3;
            l_data = (OPJ_BYTE*) calloc(1, l_data_size);
            if (l_data == NULL) {
                ret = 1;
                goto fin;
            }
            for (i = 0; i < l_nb_tiles; ++i) {
                if (! opj_write_tile(l_codec, i, l_data, l_data_size, l_stream)) {
                    fprintf(stderr, "ERROR -> test_tile_encoder: failed to write the tile %d!\n",
                            i);
                    opj_stream_destroy(l_stream);
                    opj_destroy_codec(l_codec);
                    opj_image_destroy(image);
                    ret = 1;
                    goto fin;
                }
            }
            free(l_data);
        } else {
            bSuccess = bSuccess && opj_encode(l_codec, l_stream);
            if (!bSuccess)  {
                fprintf(stderr, "failed to encode image: opj_encode\n");
            }
        }
        bSuccess = bSuccess && opj_end_compress(l_codec, l_stream);
        if (!bSuccess)  {
            fprintf(stderr, "failed to encode image: opj_end_compress\n");
        }

        if (!bSuccess)  {
            opj_stream_destroy(l_stream);
            opj_destroy_codec(l_codec);
            opj_image_destroy(image);
            fprintf(stderr, "failed to encode image\n");
            remove(parameters.outfile);
            ret = 1;
            goto fin;
        }

        num_compressed_files++;
        fprintf(stdout, "[INFO] Generated outfile %s\n", parameters.outfile);
        /* close and free the byte stream */
        opj_stream_destroy(l_stream);

        /* free remaining compression structures */
        opj_destroy_codec(l_codec);

        /* free image data */
        opj_image_destroy(image);

    }

    t = opj_clock() - t;
    if (num_compressed_files) {
        fprintf(stdout, "encode time: %d ms \n",
                (int)((t * 1000.0) / (OPJ_FLOAT64)num_compressed_files));
    }

    ret = 0;

fin:
    if (parameters.cp_comment) {
        free(parameters.cp_comment);
    }
    if (parameters.cp_matrice) {
        free(parameters.cp_matrice);
    }
    if (raw_cp.rawComps) {
        free(raw_cp.rawComps);
    }
    if (img_fol.imgdirpath) {
        free(img_fol.imgdirpath);
    }
    if (dirptr) {
        if (dirptr->filename_buf) {
            free(dirptr->filename_buf);
        }
        if (dirptr->filename) {
            free(dirptr->filename);
        }
        free(dirptr);
    }
    return ret;
}