LOCAL int decode_one(MMDB_s *mmdb, uint32_t offset,
                     MMDB_entry_data_s *entry_data)
{
    const uint8_t *mem = mmdb->data_section;

    if (offset > mmdb->data_section_size) {
        return MMDB_INVALID_DATA_ERROR;
    }

    entry_data->offset = offset;
    entry_data->has_data = true;

    DEBUG_NL;
    DEBUG_MSGF("Offset: %i", offset);

    uint8_t ctrl = mem[offset++];
    DEBUG_BINARY("Control byte: %s", ctrl);

    int type = (ctrl >> 5) & 7;
    DEBUG_MSGF("Type: %i (%s)", type, type_num_to_name(type));

    if (type == MMDB_DATA_TYPE_EXTENDED) {
        type = get_ext_type(mem[offset++]);
        DEBUG_MSGF("Extended type: %i (%s)", type, type_num_to_name(type));
    }

    entry_data->type = type;

    if (type == MMDB_DATA_TYPE_POINTER) {
        int psize = (ctrl >> 3) & 3;
        DEBUG_MSGF("Pointer size: %i", psize);

        entry_data->pointer = get_ptr_from(ctrl, &mem[offset], psize);
        DEBUG_MSGF("Pointer to: %i", entry_data->pointer);

        entry_data->data_size = psize + 1;
        entry_data->offset_to_next = offset + psize + 1;
        return MMDB_SUCCESS;
    }

    uint32_t size = ctrl & 31;
    switch (size) {
    case 29:
        size = 29 + mem[offset++];
        break;
    case 30:
        size = 285 + get_uint16(&mem[offset]);
        offset += 2;
        break;
    case 31:
        size = 65821 + get_uint24(&mem[offset]);
        offset += 3;
    default:
        break;
    }

    DEBUG_MSGF("Size: %i", size);

    if (type == MMDB_DATA_TYPE_MAP || type == MMDB_DATA_TYPE_ARRAY) {
        entry_data->data_size = size;
        entry_data->offset_to_next = offset;
        return MMDB_SUCCESS;
    }

    if (type == MMDB_DATA_TYPE_BOOLEAN) {
        entry_data->boolean = size ? true : false;
        entry_data->data_size = 0;
        entry_data->offset_to_next = offset;
        DEBUG_MSGF("boolean value: %s", entry_data->boolean ? "true" : "false");
        return MMDB_SUCCESS;
    }

    if (type == MMDB_DATA_TYPE_UINT16) {
        if (size > 2) {
            return MMDB_INVALID_DATA_ERROR;
        }
        entry_data->uint16 = (uint16_t)get_uintX(&mem[offset], size);
        DEBUG_MSGF("uint16 value: %u", entry_data->uint16);
    } else if (type == MMDB_DATA_TYPE_UINT32) {
        if (size > 4) {
            return MMDB_INVALID_DATA_ERROR;
        }
        entry_data->uint32 = (uint32_t)get_uintX(&mem[offset], size);
        DEBUG_MSGF("uint32 value: %u", entry_data->uint32);
    } else if (type == MMDB_DATA_TYPE_INT32) {
        if (size > 4) {
            return MMDB_INVALID_DATA_ERROR;
        }
        entry_data->int32 = get_sintX(&mem[offset], size);
        DEBUG_MSGF("int32 value: %i", entry_data->int32);
    } else if (type == MMDB_DATA_TYPE_UINT64) {
        if (size > 8) {
            return MMDB_INVALID_DATA_ERROR;
        }
        entry_data->uint64 = get_uintX(&mem[offset], size);
        DEBUG_MSGF("uint64 value: %" PRIu64, entry_data->uint64);
    } else if (type == MMDB_DATA_TYPE_UINT128) {
        if (size > 16) {
            return MMDB_INVALID_DATA_ERROR;
        }
#if MMDB_UINT128_IS_BYTE_ARRAY
        memset(entry_data->uint128, 0, 16);
        if (size > 0) {
            memcpy(entry_data->uint128 + 16 - size, &mem[offset], size);
        }
#else
        entry_data->uint128 = get_uint128(&mem[offset], size);
#endif
    } else if (type == MMDB_DATA_TYPE_FLOAT) {
        if (size != 4) {
            return MMDB_INVALID_DATA_ERROR;
        }
        size = 4;
        entry_data->float_value = get_ieee754_float(&mem[offset]);
        DEBUG_MSGF("float value: %f", entry_data->float_value);
    } else if (type == MMDB_DATA_TYPE_DOUBLE) {
        if (size != 8) {
            return MMDB_INVALID_DATA_ERROR;
        }
        size = 8;
        entry_data->double_value = get_ieee754_double(&mem[offset]);
        DEBUG_MSGF("double value: %f", entry_data->double_value);
    } else if (type == MMDB_DATA_TYPE_UTF8_STRING) {
        entry_data->utf8_string = size == 0 ? "" : (char *)&mem[offset];
        entry_data->data_size = size;
#ifdef MMDB_DEBUG
        char *string = mmdb_strndup(entry_data->utf8_string,
                                    size > 50 ? 50 : size);
        if (NULL == string) {
            abort();
        }
        DEBUG_MSGF("string value: %s", string);
        free(string);
#endif
    } else if (type == MMDB_DATA_TYPE_BYTES) {
        entry_data->bytes = &mem[offset];
        entry_data->data_size = size;
    }

    entry_data->offset_to_next = offset + size;

    return MMDB_SUCCESS;
}