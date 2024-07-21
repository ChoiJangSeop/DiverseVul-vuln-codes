int MMDB_open(const char *const filename, uint32_t flags, MMDB_s *const mmdb)
{
    int status = MMDB_SUCCESS;

    mmdb->file_content = NULL;
    mmdb->data_section = NULL;
    mmdb->metadata.database_type = NULL;
    mmdb->metadata.languages.count = 0;
    mmdb->metadata.description.count = 0;

    mmdb->filename = mmdb_strdup(filename);
    if (NULL == mmdb->filename) {
        status = MMDB_OUT_OF_MEMORY_ERROR;
        goto cleanup;
    }

    if ((flags & MMDB_MODE_MASK) == 0) {
        flags |= MMDB_MODE_MMAP;
    }
    mmdb->flags = flags;

    if (MMDB_SUCCESS != (status = map_file(mmdb)) ) {
        goto cleanup;
    }

#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    uint32_t metadata_size = 0;
    const uint8_t *metadata = find_metadata(mmdb->file_content, mmdb->file_size,
                                            &metadata_size);
    if (NULL == metadata) {
        status = MMDB_INVALID_METADATA_ERROR;
        goto cleanup;
    }

    mmdb->metadata_section = metadata;
    mmdb->metadata_section_size = metadata_size;

    status = read_metadata(mmdb);
    if (MMDB_SUCCESS != status) {
        goto cleanup;
    }

    if (mmdb->metadata.binary_format_major_version != 2) {
        status = MMDB_UNKNOWN_DATABASE_FORMAT_ERROR;
        goto cleanup;
    }

    uint32_t search_tree_size = mmdb->metadata.node_count *
                                mmdb->full_record_byte_size;

    mmdb->data_section = mmdb->file_content + search_tree_size;
    mmdb->data_section_size = mmdb->file_size - search_tree_size;
    mmdb->metadata_section = metadata;
    mmdb->ipv4_start_node.node_value = 0;
    mmdb->ipv4_start_node.netmask = 0;

 cleanup:
    if (MMDB_SUCCESS != status) {
        int saved_errno = errno;
        free_mmdb_struct(mmdb);
        errno = saved_errno;
    }
    return status;
}