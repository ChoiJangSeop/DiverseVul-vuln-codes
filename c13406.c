int main(int argc, char *argv[])
{
    int rc=0;
    int passlen=0;
    FILE *infp = NULL;
    FILE *outfp = NULL;
    encryptmode_t mode=UNINIT;
    char *infile = NULL;
    unsigned char pass[MAX_PASSWD_BUF];
    int file_count = 0;
    char outfile[1024];
    int password_acquired = 0;

    /* Initialize the output filename */
    outfile[0] = '\0';
    
    while ((rc = getopt(argc, argv, "vhdek:p:o:")) != -1)
    {
        switch (rc)
        {
            case 'h':
                usage(argv[0]);
                return 0;
            case 'v':
                version(argv[0]);
                return 0;
            case 'd':
                if (mode != UNINIT)
                {
                    fprintf(stderr, "Error: only specify one of -d or -e\n");
                    cleanup(outfile);
                    return -1;
                }
                mode = DEC;
                break;
            case 'e':
                if (mode != UNINIT)
                {
                    fprintf(stderr, "Error: only specify one of -d or -e\n");
                    cleanup(outfile);
                    return -1;
                }
                mode = ENC;
                break;
            case 'k':
                if (password_acquired)
                {
                    fprintf(stderr, "Error: password supplied twice\n");
                    cleanup(outfile);
                    return -1;
                }
                if (optarg != 0)
                {
                    if (!strcmp("-",optarg))
                    {
                        fprintf(stderr,
                                "Error: keyfile cannot be read from stdin\n");
                        cleanup(outfile);
                        return -1;
                    }

                    passlen = ReadKeyFile(optarg, pass);
                    if (passlen < 0)
                    {
                        cleanup(outfile);
                        return -1;
                    }
                    password_acquired = 1;
                }
                break;
            case 'p':
                if (password_acquired)
                {
                    fprintf(stderr, "Error: password supplied twice\n");
                    cleanup(outfile);
                    return -1;
                }
                if (optarg != 0)
                {
                    passlen = passwd_to_utf16(  (unsigned char*) optarg,
                                                strlen((char *)optarg),
                                                MAX_PASSWD_LEN,
                                                pass);
                    if (passlen < 0)
                    {
                        cleanup(outfile);
                        return -1;
                    }
                    password_acquired = 1;
                }
                break;
            case 'o':
                /* outfile argument */
                if (!strncmp("-", optarg, 2))
                {
                    /* if '-' is outfile name then out to stdout */
                    outfp = stdout;
                }
                else if ((outfp = fopen(optarg, "w")) == NULL)
                {
                    fprintf(stderr, "Error opening output file %s:", optarg);
                    perror("");
                    cleanup(outfile);
                    return -1;
                }
                strncpy(outfile, optarg, 1024);
                outfile[1023] = '\0';
                break;
            default:
                fprintf(stderr, "Error: Unknown option '%c'\n", rc);
        }
    }
    
    if (optind >= argc)
    {
        fprintf(stderr, "Error: No file argument specified\n");
        usage(argv[0]);
        cleanup(outfile);
        return -1;
    }

    if (mode == UNINIT)
    {
        fprintf(stderr, "Error: -e or -d not specified\n");
        usage(argv[0]);
        cleanup(outfile);
        return -1;
    }

    /* Prompt for password if not provided on the command line */
    if (passlen == 0)
    {
        passlen = read_password(pass, mode);

        switch (passlen)
        {
            case 0: /* no password in input */
                fprintf(stderr, "Error: No password supplied.\n");
                cleanup(outfile);
                return -1;
            case AESCRYPT_READPWD_FOPEN:
            case AESCRYPT_READPWD_FILENO:
            case AESCRYPT_READPWD_TCGETATTR:
            case AESCRYPT_READPWD_TCSETATTR:
            case AESCRYPT_READPWD_FGETC:
            case AESCRYPT_READPWD_TOOLONG:
            case AESCRYPT_READPWD_ICONV:
                fprintf(stderr, "Error in read_password: %s.\n",
                        read_password_error(passlen));
                cleanup(outfile);
                return -1;
            case AESCRYPT_READPWD_NOMATCH:
                fprintf(stderr, "Error: Passwords don't match.\n");
                cleanup(outfile);
                return -1;
        }

        passlen = passwd_to_utf16(  pass,
                                    strlen((char*)pass),
                                    MAX_PASSWD_LEN,
                                    pass);

        if (passlen < 0)
        {
            cleanup(outfile);
            /* For security reasons, erase the password */
            memset_secure(pass, 0, MAX_PASSWD_BUF);
            return -1;
        }
    }

    file_count = argc - optind;
    if ((file_count > 1) && (outfp != NULL))
    {
        if (outfp != stdout)
        {
            fclose(outfp);
        }
        fprintf(stderr, "Error: A single output file may not be specified with multiple input files.\n");
        usage(argv[0]);
        cleanup(outfile);
        /* For security reasons, erase the password */
        memset_secure(pass, 0, MAX_PASSWD_BUF);
        return -1;
    }

    while (optind < argc)
    {
        infile = argv[optind++];

        if(!strncmp("-", infile, 2))
        {
            if (file_count > 1)
            {
                if ((outfp != stdout) && (outfp != NULL))
                {
                    fclose(outfp);
                }
                fprintf(stderr, "Error: STDIN may not be specified with multiple input files.\n");
                usage(argv[0]);
                cleanup(outfile);
                /* For security reasons, erase the password */
                memset_secure(pass, 0, MAX_PASSWD_BUF);
                return -1;
            }
            infp = stdin;
            if (outfp == NULL)
            {
                outfp = stdout;
            }
        }
        else if ((infp = fopen(infile, "r")) == NULL)
        {
            if ((outfp != stdout) && (outfp != NULL))
            {
                fclose(outfp);
            }
            fprintf(stderr, "Error opening input file %s : ", infile);
            perror("");
            cleanup(outfile);
            /* For security reasons, erase the password */
            memset_secure(pass, 0, MAX_PASSWD_BUF);
            return -1;
        }
        
        if (mode == ENC)
        {
            if (outfp == NULL)
            {
                snprintf(outfile, 1024, "%s.aes", infile);
                if ((outfp = fopen(outfile, "w")) == NULL)
                {
                    if ((infp != stdin) && (infp != NULL))
                    {
                        fclose(infp);
                    }
                    fprintf(stderr, "Error opening output file %s : ", outfile);
                    perror("");
                    cleanup(outfile);
                    /* For security reasons, erase the password */
                    memset_secure(pass, 0, MAX_PASSWD_BUF);
                    return -1;
                }
            }
            
            rc = encrypt_stream(infp, outfp, pass, passlen);
        }
        else if (mode == DEC)
        {
            if (outfp == NULL)
            {
                /* assume .aes extension */
                strncpy(outfile, infile, strlen(infile)-4);
                outfile[strlen(infile)-4] = '\0';
                if ((outfp = fopen(outfile, "w")) == NULL)
                {
                    if ((infp != stdin) && (infp != NULL))
                    {
                        fclose(infp);
                    }
                    fprintf(stderr, "Error opening output file %s : ", outfile);
                    perror("");
                    cleanup(outfile);
                    /* For security reasons, erase the password */
                    memset_secure(pass, 0, MAX_PASSWD_BUF);
                    return -1;
                }
            }
            
            /*
             * should probably test against ascii, utf-16le, and utf-16be
             * encodings
             */
            rc = decrypt_stream(infp, outfp, pass, passlen);
        }
        
        if ((infp != stdin) && (infp != NULL))
        {
            fclose(infp);
        }
        if ((outfp != stdout) && (outfp != NULL))
        {
            if (fclose(outfp))
            {
                if (!rc)
                {
                    fprintf(stderr,
                            "Error: Could not properly close output file \n");
                    rc = -1;
                }
            }
        }

        /* If there was an error, remove the output file */
        if (rc)
        {
            cleanup(outfile);
            /* For security reasons, erase the password */
            memset_secure(pass, 0, MAX_PASSWD_BUF);
            return -1;
        }

        /* Reset input/output file names and desriptors */
        outfile[0] = '\0';
        infp = NULL;
        outfp = NULL;
    }

    /* For security reasons, erase the password */
    memset_secure(pass, 0, MAX_PASSWD_BUF);
    
    return rc;
}