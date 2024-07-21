SWTPM_NVRAM_StoreData_Intern(const unsigned char *data,
                             uint32_t length,
                             uint32_t tpm_number,
                             const char *name,
                             TPM_BOOL encrypt         /* encrypt if key is set */)
{
    TPM_RESULT    rc = 0;
    uint32_t      lrc;
    int           irc;
    FILE          *file = NULL;
    char          tmpfile[FILENAME_MAX];  /* rooted temporary file */
    char          filename[FILENAME_MAX]; /* rooted file name from name */
    unsigned char *filedata = NULL;
    uint32_t      filedata_length = 0;
    tlv_data      td[3];
    size_t        td_len = 0;
    uint16_t      flags = 0;

    TPM_DEBUG(" SWTPM_NVRAM_StoreData: To name %s\n", name);
    if (rc == 0) {
        /* map name to the rooted filename */
        rc = SWTPM_NVRAM_GetFilenameForName(filename, sizeof(filename),
                                            tpm_number, name, false);
    }

    if (rc == 0) {
        /* map name to the rooted temporary file */
        rc = SWTPM_NVRAM_GetFilenameForName(tmpfile, sizeof(tmpfile),
                                            tpm_number, name, true);
    }


    if (rc == 0) {
        /* open the file */
        TPM_DEBUG(" SWTPM_NVRAM_StoreData: Opening file %s\n", tmpfile);
        file = fopen(tmpfile, "wb");                           /* closed @1 */
        if (file == NULL) {
            logprintf(STDERR_FILENO,
                      "SWTPM_NVRAM_StoreData: Error (fatal) opening %s for "
                      "write failed, %s\n", tmpfile, strerror(errno));
            rc = TPM_FAIL;
        }
    }

    if (rc == 0) {
        if (fchmod(fileno(file), tpmstate_get_mode()) < 0) {
            logprintf(STDERR_FILENO,
                      "SWTPM_NVRAM_StoreData: Could not fchmod %s : %s\n",
                      tmpfile, strerror(errno));
            rc = TPM_FAIL;
        }
    }

    if (rc == 0) {
        if (encrypt && SWTPM_NVRAM_Has_FileKey()) {
            td_len = 3;
            rc = SWTPM_NVRAM_EncryptData(&filekey, &td[0], &td_len,
                                         TAG_ENCRYPTED_DATA, data, length,
                                         TAG_IVEC_ENCRYPTED_DATA);
            if (rc) {
                logprintf(STDERR_FILENO,
                          "SWTPM_NVRAM_EncryptData failed: 0x%02x\n", rc);
            } else {
                TPM_DEBUG("  SWTPM_NVRAM_StoreData: Encrypted %u bytes before "
                          "write, will write %u bytes\n", length,
                          td[0].tlv.length);
            }
            flags |= BLOB_FLAG_ENCRYPTED;
            if (SWTPM_NVRAM_FileKey_Size() == SWTPM_AES256_BLOCK_SIZE)
                flags |= BLOB_FLAG_ENCRYPTED_256BIT_KEY;
        } else {
            td_len = 1;
            td[0] = TLV_DATA_CONST(TAG_DATA, length, data);
        }
    }

    if (rc == 0)
        rc = tlv_data_append(&filedata, &filedata_length, td, td_len);

    if (rc == 0)
        rc = SWTPM_NVRAM_PrependHeader(&filedata, &filedata_length, flags);

    /* write the data to the file */
    if (rc == 0) {
        TPM_DEBUG("  SWTPM_NVRAM_StoreData: Writing %u bytes of data\n", length);
        lrc = fwrite(filedata, 1, filedata_length, file);
        if (lrc != filedata_length) {
            logprintf(STDERR_FILENO,
                      "TPM_NVRAM_StoreData: Error (fatal), data write "
                      "of %u only wrote %u\n", filedata_length, lrc);
            rc = TPM_FAIL;
        }
    }
    if (file != NULL) {
        TPM_DEBUG("  SWTPM_NVRAM_StoreData: Closing file %s\n", tmpfile);
        irc = fclose(file);             /* @1 */
        if (irc != 0) {
            logprintf(STDERR_FILENO,
                      "SWTPM_NVRAM_StoreData: Error (fatal) closing file\n");
            rc = TPM_FAIL;
        }
        else {
            TPM_DEBUG("  SWTPM_NVRAM_StoreData: Closed file %s\n", tmpfile);
        }
    }

    if (rc == 0 && file != NULL) {
        irc = rename(tmpfile, filename);
        if (irc != 0) {
            logprintf(STDERR_FILENO,
                      "SWTPM_NVRAM_StoreData: Error (fatal) renaming file: %s\n",
                      strerror(errno));
            rc = TPM_FAIL;
        } else {
            TPM_DEBUG("  SWTPM_NVRAM_StoreData: Renamed file to %s\n", filename);
        }
    }

    if (rc != 0 && file != NULL) {
        unlink(tmpfile);
    }

    tlv_data_free(td, td_len);
    free(filedata);

    TPM_DEBUG(" SWTPM_NVRAM_StoreData: rc=%d\n", rc);

    return rc;
}