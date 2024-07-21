ofputil_pull_ofp15_group_mod(struct ofpbuf *msg, enum ofp_version ofp_version,
                             struct ofputil_group_mod *gm)
{
    const struct ofp15_group_mod *ogm;
    uint16_t bucket_list_len;
    enum ofperr error = OFPERR_OFPGMFC_BAD_BUCKET;

    ogm = ofpbuf_pull(msg, sizeof *ogm);
    gm->command = ntohs(ogm->command);
    gm->type = ogm->type;
    gm->group_id = ntohl(ogm->group_id);

    gm->command_bucket_id = ntohl(ogm->command_bucket_id);
    switch (gm->command) {
    case OFPGC15_REMOVE_BUCKET:
        if (gm->command_bucket_id == OFPG15_BUCKET_ALL) {
            error = 0;
        }
        /* Fall through */
    case OFPGC15_INSERT_BUCKET:
        if (gm->command_bucket_id <= OFPG15_BUCKET_MAX ||
            gm->command_bucket_id == OFPG15_BUCKET_FIRST
            || gm->command_bucket_id == OFPG15_BUCKET_LAST) {
            error = 0;
        }
        break;

    case OFPGC11_ADD:
    case OFPGC11_MODIFY:
    case OFPGC11_ADD_OR_MOD:
    case OFPGC11_DELETE:
    default:
        if (gm->command_bucket_id == OFPG15_BUCKET_ALL) {
            error = 0;
        }
        break;
    }
    if (error) {
        VLOG_WARN_RL(&bad_ofmsg_rl,
                     "group command bucket id (%u) is out of range",
                     gm->command_bucket_id);
        return OFPERR_OFPGMFC_BAD_BUCKET;
    }

    bucket_list_len = ntohs(ogm->bucket_array_len);
    if (bucket_list_len > msg->size) {
        return OFPERR_OFPBRC_BAD_LEN;
    }
    error = ofputil_pull_ofp15_buckets(msg, bucket_list_len, ofp_version,
                                       gm->type, &gm->buckets);
    if (error) {
        return error;
    }

    return parse_ofp15_group_properties(msg, gm->type, gm->command, &gm->props,
                                        msg->size);
}