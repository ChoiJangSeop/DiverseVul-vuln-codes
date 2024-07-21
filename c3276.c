LOCAL int find_address_in_search_tree(MMDB_s *mmdb, uint8_t *address,
                                      sa_family_t address_family,
                                      MMDB_lookup_result_s *result)
{
    record_info_s record_info = record_info_for_database(mmdb);
    if (0 == record_info.right_record_offset) {
        return MMDB_UNKNOWN_DATABASE_FORMAT_ERROR;
    }

    DEBUG_NL;
    DEBUG_MSG("Looking for address in search tree");

    uint32_t node_count = mmdb->metadata.node_count;
    uint32_t value = 0;
    uint16_t max_depth0 = mmdb->depth - 1;
    uint16_t start_bit = max_depth0;

    if (mmdb->metadata.ip_version == 6 && address_family == AF_INET) {
        MMDB_ipv4_start_node_s ipv4_start_node = find_ipv4_start_node(mmdb);
        DEBUG_MSGF("IPv4 start node is %u (netmask %u)",
                   ipv4_start_node.node_value, ipv4_start_node.netmask);
        /* We have an IPv6 database with no IPv4 data */
        if (ipv4_start_node.node_value >= node_count) {
            return populate_result(mmdb, node_count, ipv4_start_node.node_value,
                                   ipv4_start_node.netmask, result);
        }

        value = ipv4_start_node.node_value;
        start_bit -= ipv4_start_node.netmask;
    }

    const uint8_t *search_tree = mmdb->file_content;
    const uint8_t *record_pointer;
    for (int current_bit = start_bit; current_bit >= 0; current_bit--) {
        uint8_t bit_is_true =
            address[(max_depth0 - current_bit) >> 3]
            & (1U << (~(max_depth0 - current_bit) & 7)) ? 1 : 0;

        DEBUG_MSGF("Looking at bit %i - bit's value is %i", current_bit,
                   bit_is_true);
        DEBUG_MSGF("  current node = %u", value);

        record_pointer = &search_tree[value * record_info.record_length];
        if (bit_is_true) {
            record_pointer += record_info.right_record_offset;
            value = record_info.right_record_getter(record_pointer);
        } else {
            value = record_info.left_record_getter(record_pointer);
        }

        /* Ideally we'd check to make sure that a record never points to a
         * previously seen value, but that's more complicated. For now, we can
         * at least check that we don't end up at the top of the tree again. */
        if (0 == value) {
            DEBUG_MSGF("  %s record has a value of 0",
                       bit_is_true ? "right" : "left");
            return MMDB_CORRUPT_SEARCH_TREE_ERROR;
        }

        if (value >= node_count) {
            return populate_result(mmdb, node_count, value, current_bit, result);
        } else {
            DEBUG_MSGF("  proceeding to search tree node %i", value);
        }
    }

    DEBUG_MSG(
        "Reached the end of the address bits without leaving the search tree");

    // We should not be able to reach this return. If we do, something very bad happened.
    return MMDB_CORRUPT_SEARCH_TREE_ERROR;
}