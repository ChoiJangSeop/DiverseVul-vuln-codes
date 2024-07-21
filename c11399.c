MOBI_RET mobi_reconstruct_infl(char *outstring, const MOBIIndx *infl, const MOBIIndexEntry *orth_entry) {
    const char *label = orth_entry->label;
    uint32_t *infl_groups = NULL;
    size_t infl_count = mobi_get_indxentry_tagarray(&infl_groups, orth_entry, INDX_TAGARR_ORTH_INFL);
    
    if (infl_count == 0 || !infl_groups) {
        return MOBI_SUCCESS;
    }    
    const char *start_tag = "<idx:infl>";
    const char *end_tag = "</idx:infl>";
    const char *iform_tag = "<idx:iform%s value=\"%s\"/>";
    char name_attr[INDX_INFLBUF_SIZEMAX + 1];
    char infl_tag[INDX_INFLBUF_SIZEMAX + 1];
    strcpy(outstring, start_tag);
    size_t initlen = strlen(start_tag) + strlen(end_tag);
    size_t outlen = initlen;
    size_t label_length = strlen(label);
    if (label_length > INDX_INFLBUF_SIZEMAX) {
        debug_print("Entry label too long (%s)\n", label);
        return MOBI_DATA_CORRUPT;
    }
    if (infl->cncx_record == NULL) {
        debug_print("%s\n", "Missing cncx record");
        return MOBI_DATA_CORRUPT;
    }
    for (size_t i = 0; i < infl_count; i++) {
        size_t offset = infl_groups[i];
        if (offset >= infl->entries_count) {
            debug_print("%s\n", "Invalid entry offset");
            return MOBI_DATA_CORRUPT;
        }
        uint32_t *groups;
        size_t group_cnt = mobi_get_indxentry_tagarray(&groups, &infl->entries[offset], INDX_TAGARR_INFL_GROUPS);
        uint32_t *parts;
        size_t part_cnt = mobi_get_indxentry_tagarray(&parts, &infl->entries[offset], INDX_TAGARR_INFL_PARTS_V2);
        if (group_cnt != part_cnt) {
            return MOBI_DATA_CORRUPT;
        }
        for (size_t j = 0; j < part_cnt; j++) {
            name_attr[0] = '\0';
            char *group_name = mobi_get_cncx_string(infl->cncx_record, groups[j]);
            if (group_name == NULL) {
                debug_print("%s\n", "Memory allocation failed");
                return MOBI_MALLOC_FAILED;
            }
            if (strlen(group_name)) {
                snprintf(name_attr, INDX_INFLBUF_SIZEMAX, " name=\"%s\"", group_name);
            }
            free(group_name);
            
            unsigned char decoded[INDX_INFLBUF_SIZEMAX + 1];
            memset(decoded, 0, INDX_INFLBUF_SIZEMAX + 1);
            unsigned char *rule = (unsigned char *) infl->entries[parts[j]].label;
            memcpy(decoded, label, label_length);
            int decoded_length = (int) label_length;
            MOBI_RET ret = mobi_decode_infl(decoded, &decoded_length, rule);
            if (ret != MOBI_SUCCESS) {
                return ret;
            }
            if (decoded_length == 0) {
                continue;
            }
            int n = snprintf(infl_tag, INDX_INFLBUF_SIZEMAX, iform_tag, name_attr, decoded);
            if (n > INDX_INFLBUF_SIZEMAX) {
                debug_print("Skipping truncated tag: %s\n", infl_tag);
                continue;
            }
            outlen += strlen(infl_tag);
            if (outlen > INDX_INFLTAG_SIZEMAX) {
                debug_print("Inflections text in %s too long (%zu)\n", label, outlen);
                return MOBI_ERROR;
            }
            strcat(outstring, infl_tag);
        }
    }
    if (outlen == initlen) {
        outstring[0] = '\0';
    } else {
        strcat(outstring, end_tag);
    }
    return MOBI_SUCCESS;
}