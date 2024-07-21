LOCAL MMDB_ipv4_start_node_s find_ipv4_start_node(MMDB_s *mmdb)
{
    /* In a pathological case of a database with a single node search tree,
     * this check will be true even after we've found the IPv4 start node, but
     * that doesn't seem worth trying to fix. */
    if (mmdb->ipv4_start_node.node_value != 0) {
        return mmdb->ipv4_start_node;
    }

    record_info_s record_info = record_info_for_database(mmdb);

    const uint8_t *search_tree = mmdb->file_content;
    uint32_t node_value = 0;
    const uint8_t *record_pointer;
    uint32_t netmask;
    for (netmask = 0; netmask < 96; netmask++) {
        record_pointer = &search_tree[node_value * record_info.record_length];
        node_value = record_info.left_record_getter(record_pointer);
        /* This can happen if there's no IPv4 data _or_ if there is a subnet
         * with data that contains the entire IPv4 range (like ::/64) */
        if (node_value >= mmdb->metadata.node_count) {
            break;
        }
    }

    mmdb->ipv4_start_node.node_value = node_value;
    mmdb->ipv4_start_node.netmask = netmask;

    return mmdb->ipv4_start_node;
}