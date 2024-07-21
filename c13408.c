int main(int argc, char *argv[])
{
    int option;
    int passlen=0;
    FILE *outfp = NULL;
    char outfile[1024];
    unsigned char pass[MAX_PASSWD_BUF];
    int file_count = 0;
    unsigned char bom[2];
    int password_acquired = 0;

    while ((option = getopt(argc, argv, "vhg:p:o:")) != -1)
    {
        switch (option)
        {
            case 'h':
                usage(argv[0]);
                return 0;
            case 'v':
                version(argv[0]);
                return 0;
            case 'g':
                if (password_acquired)
                {
                    fprintf(stderr, "Error: password supplied twice\n");
                    return -1;
                }
                if (optarg != 0)
                {
                    passlen = generate_password(atoi((char*) optarg),
                                                pass);
                    if (passlen < 0)
                    {
                        return -1;
                    }
                }
                password_acquired = 1;
                break;
            case 'p':
                if (password_acquired)
                {
                    fprintf(stderr, "Error: password supplied twice\n");
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
                        return -1;
                    }
                }
                password_acquired = 1;
                break;
            default:
                fprintf(stderr, "Error: Unknown option '%c'\n", option);
                return -1;
        }
    }
    
    file_count = argc - optind;
    if (file_count != 1)
    {
        fprintf(stderr, "Error: A single output file must be specified.\n");
        usage(argv[0]);
        /* For security reasons, erase the password */
        memset_secure(pass, 0, MAX_PASSWD_BUF);
        return -1;
    }
    else
    {
        /* What is the filename for the key file? */
        strncpy(outfile, argv[optind++], 1024);
        outfile[1023] = '\0';
    }

    /* Prompt for password if not provided on the command line */
    if (passlen == 0)
    {
        passlen = read_password(pass, ENC);

        switch (passlen)
        {
            case 0: /* no password in input */
                fprintf(stderr, "Error: No password supplied.\n");
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
                return -1;
            case AESCRYPT_READPWD_NOMATCH:
                fprintf(stderr, "Error: Passwords don't match.\n");
                return -1;
        }
    }

    if(!strcmp("-", outfile))
    {
        outfp = stdout;
    }
    else if ((outfp = fopen(outfile, "w")) == NULL)
    {
        fprintf(stderr, "Error opening output file %s : ", outfile);
        perror("");
        /* For security reasons, erase the password */
        memset_secure(pass, 0, MAX_PASSWD_BUF);
        return  -1;
    }

    /* Write the BOM.  AES Crypt uses UTF-16LE */
    bom[0] = 0xFF;
    bom[1] = 0xFE;
    if (fwrite(bom, 1, 2, outfp) != 2)
    {
        fprintf(stderr, "Error: Could not write BOM to password file.\n");
        if (strcmp("-",outfile))
        {
            fclose(outfp);
        }
        cleanup(outfile);
        return  -1;
    }
    
    if (fwrite(pass, 1, passlen, outfp) != (size_t) passlen)
    {
        fprintf(stderr, "Error: Could not write password file.\n");
        if (strcmp("-",outfile))
        {
            fclose(outfp);
        }
        cleanup(outfile);
        return  -1;
    }

    /* Close the output file, so long as it is not stdout */
    if (strcmp("-",outfile))
    {
        fclose(outfp);
    }

    /* For security reasons, erase the password */
    memset_secure(pass, 0, MAX_PASSWD_BUF);

    return 0;
}