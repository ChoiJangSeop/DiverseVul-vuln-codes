ofputil_decode_group_mod(const struct ofp_header *oh,
                         struct ofputil_group_mod *gm)
{
    ofputil_init_group_properties(&gm->props);

    enum ofp_version ofp_version = oh->version;
    struct ofpbuf msg = ofpbuf_const_initializer(oh, ntohs(oh->length));
    ofpraw_pull_assert(&msg);

    enum ofperr err;
    switch (ofp_version)
    {
    case OFP11_VERSION:
    case OFP12_VERSION:
    case OFP13_VERSION:
    case OFP14_VERSION:
        err = ofputil_pull_ofp11_group_mod(&msg, ofp_version, gm);
        break;

    case OFP15_VERSION:
    case OFP16_VERSION:
        err = ofputil_pull_ofp15_group_mod(&msg, ofp_version, gm);
        break;

    case OFP10_VERSION:
    default:
        OVS_NOT_REACHED();
    }
    if (err) {
        return err;
    }

    switch (gm->type) {
    case OFPGT11_INDIRECT:
        if (gm->command != OFPGC11_DELETE
            && !ovs_list_is_singleton(&gm->buckets) ) {
            return OFPERR_OFPGMFC_INVALID_GROUP;
        }
        break;
    case OFPGT11_ALL:
    case OFPGT11_SELECT:
    case OFPGT11_FF:
        break;
    default:
        return OFPERR_OFPGMFC_BAD_TYPE;
    }

    switch (gm->command) {
    case OFPGC11_ADD:
    case OFPGC11_MODIFY:
    case OFPGC11_ADD_OR_MOD:
    case OFPGC11_DELETE:
    case OFPGC15_INSERT_BUCKET:
        break;
    case OFPGC15_REMOVE_BUCKET:
        if (!ovs_list_is_empty(&gm->buckets)) {
            return OFPERR_OFPGMFC_BAD_BUCKET;
        }
        break;
    default:
        return OFPERR_OFPGMFC_BAD_COMMAND;
    }

    struct ofputil_bucket *bucket;
    LIST_FOR_EACH (bucket, list_node, &gm->buckets) {
        if (bucket->weight && gm->type != OFPGT11_SELECT) {
            return OFPERR_OFPGMFC_INVALID_GROUP;
        }

        switch (gm->type) {
        case OFPGT11_ALL:
        case OFPGT11_INDIRECT:
            if (ofputil_bucket_has_liveness(bucket)) {
                return OFPERR_OFPGMFC_WATCH_UNSUPPORTED;
            }
            break;
        case OFPGT11_SELECT:
            break;
        case OFPGT11_FF:
            if (!ofputil_bucket_has_liveness(bucket)) {
                return OFPERR_OFPGMFC_INVALID_GROUP;
            }
            break;
        default:
            /* Returning BAD TYPE to be consistent
             * though gm->type has been checked already. */
            return OFPERR_OFPGMFC_BAD_TYPE;
        }
    }

    return 0;
}