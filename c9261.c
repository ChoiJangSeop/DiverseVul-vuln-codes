cl_error_t cli_codepage_to_utf8(char* in, size_t in_size, uint16_t codepage, char** out, size_t* out_size)
{
    cl_error_t status = CL_BREAK;

    char* out_utf8       = NULL;
    size_t out_utf8_size = 0;

#if defined(HAVE_ICONV)
    iconv_t conv = NULL;
#elif defined(WIN32)
    LPWSTR lpWideCharStr = NULL;
    int cchWideChar      = 0;
#endif

    if (NULL == in || in_size == 0 || NULL == out || NULL == out_size) {
        cli_dbgmsg("egg_filename_to_utf8: Invalid args.\n");
        status = CL_EARG;
        goto done;
    }

    *out      = NULL;
    *out_size = 0;

    switch (codepage) {
        case 20127:   /* US-ASCII (7-bit) */
        case 65001: { /* Unicode (UTF-8) */
            char* track;
            int byte_count, sigbit_count;

            out_utf8_size = in_size;
            out_utf8      = cli_calloc(1, out_utf8_size + 1);
            if (NULL == out_utf8) {
                cli_errmsg("egg_filename_to_utf8: Failure allocating buffer for utf8 filename.\n");
                status = CL_EMEM;
                goto done;
            }
            memcpy(out_utf8, in, in_size);

            track = out_utf8 + in_size - 1;
            if ((codepage == 65001) && (*track & 0x80)) {
                /*
                 * UTF-8 with a most significant bit.
                 */

                /* locate the start of the last character */
                for (byte_count = 1; (track != out_utf8); track--, byte_count++) {
                    if (((uint8_t)*track & 0xC0) != 0x80)
                        break;
                }

                /* count number of set (1) significant bits */
                for (sigbit_count = 0; sigbit_count < (int)(sizeof(uint8_t) * 8); sigbit_count++) {
                    if (((uint8_t)*track & (0x80 >> sigbit_count)) == 0)
                        break;
                }

                if (byte_count != sigbit_count) {
                    cli_dbgmsg("egg_filename_to_utf8: cleaning out %d bytes from incomplete "
                               "utf-8 character length %d\n",
                               byte_count, sigbit_count);
                    for (; byte_count > 0; byte_count--, track++) {
                        *track = '\0';
                    }
                }
            }
            break;
        }
        default: {

#if defined(WIN32) && !defined(HAVE_ICONV)

            /*
             * Do conversion using native Win32 APIs.
             */

            if (1200 != codepage) { /* not already UTF16-LE (Windows Unicode) */
                /*
                 * First, Convert from codepage -> UCS-2 LE with MultiByteToWideChar(codepage)
                 */
                cchWideChar = MultiByteToWideChar(
                    codepage,
                    0,
                    in,
                    in_size,
                    NULL,
                    0);
                if (0 == cchWideChar) {
                    cli_dbgmsg("egg_filename_to_utf8: failed to determine string size needed for ansi to widechar conversion.\n");
                    status = CL_EPARSE;
                    goto done;
                }

                lpWideCharStr = malloc((cchWideChar + 1) * sizeof(WCHAR));
                if (NULL == lpWideCharStr) {
                    cli_dbgmsg("egg_filename_to_utf8: failed to allocate memory for wide char string.\n");
                    status = CL_EMEM;
                    goto done;
                }

                cchWideChar = MultiByteToWideChar(
                    codepage,
                    0,
                    in,
                    in_size,
                    lpWideCharStr,
                    cchWideChar + 1);
                if (0 == cchWideChar) {
                    cli_dbgmsg("egg_filename_to_utf8: failed to convert multibyte string to widechars.\n");
                    status = CL_EPARSE;
                    goto done;
                }

                in      = (char*)lpWideCharStr;
                in_size = cchWideChar;
            }

            /*
             * Convert from UCS-2 LE -> UTF8 with WideCharToMultiByte(CP_UTF8)
             */
            out_utf8_size = WideCharToMultiByte(
                CP_UTF8,
                0,
                (LPCWCH)in,
                in_size / sizeof(WCHAR),
                NULL,
                0,
                NULL,
                NULL);
            if (0 == out_utf8_size) {
                cli_dbgmsg("egg_filename_to_utf8: failed to determine string size needed for widechar conversion.\n");
                status = CL_EPARSE;
                goto done;
            }

            out_utf8 = malloc(out_utf8_size + 1);
            if (NULL == lpWideCharStr) {
                cli_dbgmsg("egg_filename_to_utf8: failed to allocate memory for wide char to utf-8 string.\n");
                status = CL_EMEM;
                goto done;
            }

            out_utf8_size = WideCharToMultiByte(
                CP_UTF8,
                0,
                (LPCWCH)in,
                in_size / sizeof(WCHAR),
                out_utf8,
                out_utf8_size,
                NULL,
                NULL);
            if (0 == out_utf8_size) {
                cli_dbgmsg("egg_filename_to_utf8: failed to convert widechar string to utf-8.\n");
                status = CL_EPARSE;
                goto done;
            }

#elif defined(HAVE_ICONV)

            uint32_t attempt, i;
            size_t inbytesleft, outbytesleft;
            const char* encoding = NULL;

            for (i = 0; i < NUMCODEPAGES; ++i) {
                if (codepage == codepage_entries[i].codepage) {
                    encoding = codepage_entries[i].encoding;
                } else if (codepage < codepage_entries[i].codepage) {
                    break; /* fail-out early, requires sorted array */
                }
            }

            for (attempt = 1; attempt <= 3; attempt++) {
                char* out_utf8_tmp;

                /* Charset to UTF-8 should never exceed in_size * 6;
                 * We can shrink final buffer after the conversion, if needed. */
                out_utf8_size = (in_size * 2) * attempt;

                inbytesleft  = in_size;
                outbytesleft = out_utf8_size;

                out_utf8 = cli_calloc(1, out_utf8_size + 1);
                if (NULL == out_utf8) {
                    cli_errmsg("egg_filename_to_utf8: Failure allocating buffer for utf8 data.\n");
                    status = CL_EMEM;
                }

                conv = iconv_open("UTF-8//TRANSLIT", encoding);
                if (conv == (iconv_t)-1) {
                    cli_warnmsg("egg_filename_to_utf8: Failed to open iconv.\n");
                    goto done;
                }

                if ((size_t)-1 == iconv(conv, &in, &inbytesleft, &out_utf8, &outbytesleft)) {
                    switch (errno) {
                        case E2BIG:
                            cli_warnmsg("egg_filename_to_utf8: iconv error: There is not sufficient room at *outbuf.\n");
                            free(out_utf8);
                            out_utf8 = NULL;
                            continue; /* Try again, with a larger buffer. */
                        case EILSEQ:
                            cli_warnmsg("egg_filename_to_utf8: iconv error: An invalid multibyte sequence has been encountered in the input.\n");
                            break;
                        case EINVAL:
                            cli_warnmsg("egg_filename_to_utf8: iconv error: An incomplete multibyte sequence has been encountered in the input.\n");
                            break;
                        default:
                            cli_warnmsg("egg_filename_to_utf8: iconv error: Unexpected error code %d.\n", errno);
                    }
                    status = CL_EPARSE;
                    goto done;
                }

                /* iconv succeeded, but probably didn't use the whole buffer. Free up the extra memory. */
                out_utf8_tmp = cli_realloc(out_utf8, out_utf8_size - outbytesleft + 1);
                if (NULL == out_utf8_tmp) {
                    cli_errmsg("egg_filename_to_utf8: failure cli_realloc'ing converted filename.\n");
                    status = CL_EMEM;
                    goto done;
                }
                out_utf8      = out_utf8_tmp;
                out_utf8_size = out_utf8_size - outbytesleft;
            }

#else

            /*
             * No way to do the conversion.
             */
            goto done;

#endif
        }
    }

    *out      = out_utf8;
    *out_size = out_utf8_size;

    status = CL_SUCCESS;

done:

#if defined(WIN32) && !defined(HAVE_ICONV)
    if (NULL != lpWideCharStr) {
        free(lpWideCharStr);
    }
#endif

    if (CL_SUCCESS != status) {
        if (NULL != out_utf8) {
            free(out_utf8);
        }
    }

    return status;
}