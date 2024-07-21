ofputil_pull_ofp11_buckets(struct ofpbuf *msg, size_t buckets_length,
                           enum ofp_version version, struct ovs_list *buckets)
{
    struct ofp11_bucket *ob;
    uint32_t bucket_id = 0;

    ovs_list_init(buckets);
    while (buckets_length > 0) {
        struct ofputil_bucket *bucket;
        struct ofpbuf ofpacts;
        enum ofperr error;
        size_t ob_len;

        ob = (buckets_length >= sizeof *ob
              ? ofpbuf_try_pull(msg, sizeof *ob)
              : NULL);
        if (!ob) {
            VLOG_WARN_RL(&bad_ofmsg_rl, "buckets end with %"PRIuSIZE" leftover bytes",
                         buckets_length);
            return OFPERR_OFPGMFC_BAD_BUCKET;
        }

        ob_len = ntohs(ob->len);
        if (ob_len < sizeof *ob) {
            VLOG_WARN_RL(&bad_ofmsg_rl, "OpenFlow message bucket length "
                         "%"PRIuSIZE" is not valid", ob_len);
            return OFPERR_OFPGMFC_BAD_BUCKET;
        } else if (ob_len > buckets_length) {
            VLOG_WARN_RL(&bad_ofmsg_rl, "OpenFlow message bucket length "
                         "%"PRIuSIZE" exceeds remaining buckets data size %"PRIuSIZE,
                         ob_len, buckets_length);
            return OFPERR_OFPGMFC_BAD_BUCKET;
        }
        buckets_length -= ob_len;

        ofpbuf_init(&ofpacts, 0);
        error = ofpacts_pull_openflow_actions(msg, ob_len - sizeof *ob,
                                              version, NULL, NULL, &ofpacts);
        if (error) {
            ofpbuf_uninit(&ofpacts);
            ofputil_bucket_list_destroy(buckets);
            return error;
        }

        bucket = xzalloc(sizeof *bucket);
        bucket->weight = ntohs(ob->weight);
        error = ofputil_port_from_ofp11(ob->watch_port, &bucket->watch_port);
        if (error) {
            ofpbuf_uninit(&ofpacts);
            ofputil_bucket_list_destroy(buckets);
            free(bucket);
            return OFPERR_OFPGMFC_BAD_WATCH;
        }
        bucket->watch_group = ntohl(ob->watch_group);
        bucket->bucket_id = bucket_id++;

        bucket->ofpacts = ofpbuf_steal_data(&ofpacts);
        bucket->ofpacts_len = ofpacts.size;
        ovs_list_push_back(buckets, &bucket->list_node);
    }

    return 0;
}