LOCAL MMDB_entry_data_list_s *dump_entry_data_list(
    FILE *stream, MMDB_entry_data_list_s *entry_data_list, int indent,
    int *status)
{
    switch (entry_data_list->entry_data.type) {
    case MMDB_DATA_TYPE_MAP:
        {
            uint32_t size = entry_data_list->entry_data.data_size;

            print_indentation(stream, indent);
            fprintf(stream, "{\n");
            indent += 2;

            for (entry_data_list = entry_data_list->next;
                 size && entry_data_list; size--) {

                char *key =
                    mmdb_strndup(
                        (char *)entry_data_list->entry_data.utf8_string,
                        entry_data_list->entry_data.data_size);
                if (NULL == key) {
                    *status = MMDB_OUT_OF_MEMORY_ERROR;
                    return NULL;
                }

                print_indentation(stream, indent);
                fprintf(stream, "\"%s\": \n", key);
                free(key);

                entry_data_list = entry_data_list->next;
                entry_data_list =
                    dump_entry_data_list(stream, entry_data_list, indent + 2,
                                         status);

                if (MMDB_SUCCESS != *status) {
                    return NULL;
                }
            }

            indent -= 2;
            print_indentation(stream, indent);
            fprintf(stream, "}\n");
        }
        break;
    case MMDB_DATA_TYPE_ARRAY:
        {
            uint32_t size = entry_data_list->entry_data.data_size;

            print_indentation(stream, indent);
            fprintf(stream, "[\n");
            indent += 2;

            for (entry_data_list = entry_data_list->next;
                 size && entry_data_list; size--) {
                entry_data_list =
                    dump_entry_data_list(stream, entry_data_list, indent,
                                         status);
                if (MMDB_SUCCESS != *status) {
                    return NULL;
                }
            }

            indent -= 2;
            print_indentation(stream, indent);
            fprintf(stream, "]\n");
        }
        break;
    case MMDB_DATA_TYPE_UTF8_STRING:
        {
            char *string =
                mmdb_strndup((char *)entry_data_list->entry_data.utf8_string,
                             entry_data_list->entry_data.data_size);
            if (NULL == string) {
                *status = MMDB_OUT_OF_MEMORY_ERROR;
                return NULL;
            }
            print_indentation(stream, indent);
            fprintf(stream, "\"%s\" <utf8_string>\n", string);
            free(string);
            entry_data_list = entry_data_list->next;
        }
        break;
    case MMDB_DATA_TYPE_BYTES:
        {
            char *hex_string =
                bytes_to_hex((uint8_t *)entry_data_list->entry_data.bytes,
                             entry_data_list->entry_data.data_size);
            if (NULL == hex_string) {
                *status = MMDB_OUT_OF_MEMORY_ERROR;
                return NULL;
            }

            print_indentation(stream, indent);
            fprintf(stream, "%s <bytes>\n", hex_string);
            free(hex_string);

            entry_data_list = entry_data_list->next;
        }
        break;
    case MMDB_DATA_TYPE_DOUBLE:
        print_indentation(stream, indent);
        fprintf(stream, "%f <double>\n",
                entry_data_list->entry_data.double_value);
        entry_data_list = entry_data_list->next;
        break;
    case MMDB_DATA_TYPE_FLOAT:
        print_indentation(stream, indent);
        fprintf(stream, "%f <float>\n",
                entry_data_list->entry_data.float_value);
        entry_data_list = entry_data_list->next;
        break;
    case MMDB_DATA_TYPE_UINT16:
        print_indentation(stream, indent);
        fprintf(stream, "%u <uint16>\n", entry_data_list->entry_data.uint16);
        entry_data_list = entry_data_list->next;
        break;
    case MMDB_DATA_TYPE_UINT32:
        print_indentation(stream, indent);
        fprintf(stream, "%u <uint32>\n", entry_data_list->entry_data.uint32);
        entry_data_list = entry_data_list->next;
        break;
    case MMDB_DATA_TYPE_BOOLEAN:
        print_indentation(stream, indent);
        fprintf(stream, "%s <boolean>\n",
                entry_data_list->entry_data.boolean ? "true" : "false");
        entry_data_list = entry_data_list->next;
        break;
    case MMDB_DATA_TYPE_UINT64:
        print_indentation(stream, indent);
        fprintf(stream, "%" PRIu64 " <uint64>\n",
                entry_data_list->entry_data.uint64);
        entry_data_list = entry_data_list->next;
        break;
    case MMDB_DATA_TYPE_UINT128:
        print_indentation(stream, indent);
#if MMDB_UINT128_IS_BYTE_ARRAY
        char *hex_string =
            bytes_to_hex((uint8_t *)entry_data_list->entry_data.uint128, 16);
        fprintf(stream, "0x%s <uint128>\n", hex_string);
        free(hex_string);
#else
        uint64_t high = entry_data_list->entry_data.uint128 >> 64;
        uint64_t low = (uint64_t)entry_data_list->entry_data.uint128;
        fprintf(stream, "0x%016" PRIX64 "%016" PRIX64 " <uint128>\n", high,
                low);
#endif
        entry_data_list = entry_data_list->next;
        break;
    case MMDB_DATA_TYPE_INT32:
        print_indentation(stream, indent);
        fprintf(stream, "%d <int32>\n", entry_data_list->entry_data.int32);
        entry_data_list = entry_data_list->next;
        break;
    default:
        *status = MMDB_INVALID_DATA_ERROR;
        return NULL;
    }

    *status = MMDB_SUCCESS;
    return entry_data_list;
}