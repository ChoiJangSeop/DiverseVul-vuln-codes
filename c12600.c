void dns_server_reset_features(DnsServer *s) {
        assert(s);

        s->verified_feature_level = _DNS_SERVER_FEATURE_LEVEL_INVALID;
        s->possible_feature_level = DNS_SERVER_FEATURE_LEVEL_BEST;

        s->received_udp_packet_max = DNS_PACKET_UNICAST_SIZE_MAX;

        s->packet_bad_opt = false;
        s->packet_rrsig_missing = false;

        s->features_grace_period_usec = DNS_SERVER_FEATURE_GRACE_PERIOD_MIN_USEC;

        s->warned_downgrade = false;

        dns_server_reset_counters(s);
}