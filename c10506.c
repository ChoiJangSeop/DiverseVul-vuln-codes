SWTPM_NVRAM_LoadData(unsigned char **data,     /* freed by caller */
                     uint32_t *length,
                     uint32_t tpm_number,
                     const char *name)
{
    TPM_RESULT    rc = 0;
    long          lrc;
    size_t        src;
    int           irc;
    FILE          *file = NULL;
    char          filename[FILENAME_MAX]; /* rooted file name from name */
    unsigned char *decrypt_data = NULL;
    uint32_t      decrypt_length;
    uint32_t      dataoffset = 0;
    uint8_t       hdrversion = 0;
    uint16_t      hdrflags;

    TPM_DEBUG(" SWTPM_NVRAM_LoadData: From file %s\n", name);
    *data = NULL;
    *length = 0;
    /* open the file */
    if (rc == 0) {
        /* map name to the rooted filename */
        rc = SWTPM_NVRAM_GetFilenameForName(filename, sizeof(filename),
                                            tpm_number, name, false);
    }

    if (rc == 0) {
        TPM_DEBUG("  SWTPM_NVRAM_LoadData: Opening file %s\n", filename);
        file = fopen(filename, "rb");                           /* closed @1 */
        if (file == NULL) {     /* if failure, determine cause */
            if (errno == ENOENT) {
                TPM_DEBUG("SWTPM_NVRAM_LoadData: No such file %s\n",
                         filename);
                rc = TPM_RETRY;         /* first time start up */
            }
            else {
                logprintf(STDERR_FILENO,
                          "SWTPM_NVRAM_LoadData: Error (fatal) opening "
                          "%s for read, %s\n", filename, strerror(errno));
                rc = TPM_FAIL;
            }
        }
    }

    if (rc == 0) {
        if (fchmod(fileno(file), tpmstate_get_mode()) < 0) {
            logprintf(STDERR_FILENO,
                      "SWTPM_NVRAM_LoadData: Could not fchmod %s : %s\n",
                      filename, strerror(errno));
            rc = TPM_FAIL;
        }
    }

    /* determine the file length */
    if (rc == 0) {
        irc = fseek(file, 0L, SEEK_END);        /* seek to end of file */
        if (irc == -1L) {
            logprintf(STDERR_FILENO,
                      "SWTPM_NVRAM_LoadData: Error (fatal) fseek'ing %s, %s\n",
                      filename, strerror(errno));
            rc = TPM_FAIL;
        }
    }
    if (rc == 0) {
        lrc = ftell(file);                      /* get position in the stream */
        if (lrc == -1L) {
            logprintf(STDERR_FILENO,
                      "SWTPM_NVRAM_LoadData: Error (fatal) ftell'ing %s, %s\n",
                      filename, strerror(errno));
            rc = TPM_FAIL;
        }
        else {
            *length = (uint32_t)lrc;              /* save the length */
        }
    }
    if (rc == 0) {
        irc = fseek(file, 0L, SEEK_SET);        /* seek back to the beginning of the file */
        if (irc == -1L) {
            logprintf(STDERR_FILENO,
                      "SWTPM_NVRAM_LoadData: Error (fatal) fseek'ing %s, %s\n",
                      filename, strerror(errno));
            rc = TPM_FAIL;
        }
    }
    /* allocate a buffer for the actual data */
    if ((rc == 0) && *length != 0) {
        TPM_DEBUG(" SWTPM_NVRAM_LoadData: Reading %u bytes of data\n", *length);
        *data = malloc(*length);
        if (!*data) {
            logprintf(STDERR_FILENO,
                      "SWTPM_NVRAM_LoadData: Error (fatal) allocating %u "
                      "bytes\n", *length);
            rc = TPM_FAIL;
        }
    }
    /* read the contents of the file into the data buffer */
    if ((rc == 0) && *length != 0) {
        src = fread(*data, 1, *length, file);
        if (src != *length) {
            logprintf(STDERR_FILENO,
                      "SWTPM_NVRAM_LoadData: Error (fatal), data read of %u "
                      "only read %lu\n", *length, (unsigned long)src);
            rc = TPM_FAIL;
        }
    }
    /* close the file */
    if (file != NULL) {
        TPM_DEBUG(" SWTPM_NVRAM_LoadData: Closing file %s\n", filename);
        irc = fclose(file);             /* @1 */
        if (irc != 0) {
            logprintf(STDERR_FILENO,
                      "SWTPM_NVRAM_LoadData: Error (fatal) closing file %s\n",
                      filename);
            rc = TPM_FAIL;
        }
        else {
            TPM_DEBUG(" SWTPM_NVRAM_LoadData: Closed file %s\n", filename);
        }
    }

    if (rc == 0) {
        /* this function needs to return the plain data -- no tlv headers */

        /* try to get a header from it -- old files may not have one */
        irc = SWTPM_NVRAM_CheckHeader(*data, *length, &dataoffset,
                                      &hdrflags, &hdrversion, true);
        /* valid header -- this one can only be version 2 or later */
        if (irc) {
            hdrversion = 1; /* no header -- payload was written like vers. 1 */
            hdrflags = 0;
        }

        rc = SWTPM_NVRAM_GetDecryptedData(&filekey,
                                          &decrypt_data, &decrypt_length,
                                          *data + dataoffset,
                                          *length - dataoffset,
                                          TAG_ENCRYPTED_DATA, TAG_DATA,
                                          hdrversion,
                                          TAG_IVEC_ENCRYPTED_DATA,
                                          hdrflags,
                                          BLOB_FLAG_ENCRYPTED_256BIT_KEY);
        TPM_DEBUG(" SWTPM_NVRAM_LoadData: SWTPM_NVRAM_GetDecryptedData rc = %d\n",
                  rc);
        if (rc != 0)
            logprintf(STDERR_FILENO,
                      "SWTPM_NVRAM_LoadData: Error from SWTPM_NVRAM_GetDecryptedData "
                      "rc = %d\n", rc);

        if (rc == 0) {
            TPM_DEBUG(" SWTPM_NVRAM_LoadData: Decrypted %u bytes of "
                      "data to %u bytes.\n",
                      *length, decrypt_length);
            free(*data);
            *data = decrypt_data;
            *length = decrypt_length;
        }
    }

    if (rc != 0) {
        free(*data);
        *data = NULL;
    }

    return rc;
}