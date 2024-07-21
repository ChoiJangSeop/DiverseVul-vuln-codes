ofputil_pull_ofp11_group_mod(struct ofpbuf *msg, enum ofp_version ofp_version,
                             struct ofputil_group_mod *gm)
{
    const struct ofp11_group_mod *ogm;
    enum ofperr error;

    ogm = ofpbuf_pull(msg, sizeof *ogm);
    gm->command = ntohs(ogm->command);
    gm->type = ogm->type;
    gm->group_id = ntohl(ogm->group_id);
    gm->command_bucket_id = OFPG15_BUCKET_ALL;

    error = ofputil_pull_ofp11_buckets(msg, msg->size, ofp_version,
                                       &gm->buckets);

    /* OF1.3.5+ prescribes an error when an OFPGC_DELETE includes buckets. */
    if (!error
        && ofp_version >= OFP13_VERSION
        && gm->command == OFPGC11_DELETE
        && !ovs_list_is_empty(&gm->buckets)) {
        error = OFPERR_OFPGMFC_INVALID_GROUP;
    }

    return error;
}