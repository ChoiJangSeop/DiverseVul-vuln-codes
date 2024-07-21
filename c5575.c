ofputil_decode_ofp15_group_desc_reply(struct ofputil_group_desc *gd,
                                      struct ofpbuf *msg,
                                      enum ofp_version version)
{
    struct ofp15_group_desc_stats *ogds;
    uint16_t length, bucket_list_len;
    int error;

    if (!msg->header) {
        ofpraw_pull_assert(msg);
    }

    if (!msg->size) {
        return EOF;
    }

    ogds = ofpbuf_try_pull(msg, sizeof *ogds);
    if (!ogds) {
        VLOG_WARN_RL(&bad_ofmsg_rl, "OFPST11_GROUP_DESC reply has %"PRIu32" "
                     "leftover bytes at end", msg->size);
        return OFPERR_OFPBRC_BAD_LEN;
    }
    gd->type = ogds->type;
    gd->group_id = ntohl(ogds->group_id);

    length = ntohs(ogds->length);
    if (length < sizeof *ogds || length - sizeof *ogds > msg->size) {
        VLOG_WARN_RL(&bad_ofmsg_rl, "OFPST11_GROUP_DESC reply claims invalid "
                     "length %u", length);
        return OFPERR_OFPBRC_BAD_LEN;
    }

    bucket_list_len = ntohs(ogds->bucket_list_len);
    if (length < bucket_list_len + sizeof *ogds) {
        VLOG_WARN_RL(&bad_ofmsg_rl, "OFPST11_GROUP_DESC reply claims invalid "
                     "bucket list length %u", bucket_list_len);
        return OFPERR_OFPBRC_BAD_LEN;
    }
    error = ofputil_pull_ofp15_buckets(msg, bucket_list_len, version, gd->type,
                                       &gd->buckets);
    if (error) {
        return error;
    }

    /* By definition group desc messages don't have a group mod command.
     * However, parse_group_prop_ntr_selection_method() checks to make sure
     * that the command is OFPGC15_ADD or OFPGC15_DELETE to guard
     * against group mod messages with other commands supplying
     * a NTR selection method group experimenter property.
     * Such properties are valid for group desc replies so
     * claim that the group mod command is OFPGC15_ADD to
     * satisfy the check in parse_group_prop_ntr_selection_method() */
    return parse_ofp15_group_properties(msg, gd->type, OFPGC15_ADD, &gd->props,
                                        length - sizeof *ogds - bucket_list_len);
}