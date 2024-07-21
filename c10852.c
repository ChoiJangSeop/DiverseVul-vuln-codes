static uint16_t nvme_changed_nslist(NvmeCtrl *n, uint8_t rae, uint32_t buf_len,
                                    uint64_t off, NvmeRequest *req)
{
    uint32_t nslist[1024];
    uint32_t trans_len;
    int i = 0;
    uint32_t nsid;

    memset(nslist, 0x0, sizeof(nslist));
    trans_len = MIN(sizeof(nslist) - off, buf_len);

    while ((nsid = find_first_bit(n->changed_nsids, NVME_CHANGED_NSID_SIZE)) !=
            NVME_CHANGED_NSID_SIZE) {
        /*
         * If more than 1024 namespaces, the first entry in the log page should
         * be set to FFFFFFFFh and the others to 0 as spec.
         */
        if (i == ARRAY_SIZE(nslist)) {
            memset(nslist, 0x0, sizeof(nslist));
            nslist[0] = 0xffffffff;
            break;
        }

        nslist[i++] = nsid;
        clear_bit(nsid, n->changed_nsids);
    }

    /*
     * Remove all the remaining list entries in case returns directly due to
     * more than 1024 namespaces.
     */
    if (nslist[0] == 0xffffffff) {
        bitmap_zero(n->changed_nsids, NVME_CHANGED_NSID_SIZE);
    }

    if (!rae) {
        nvme_clear_events(n, NVME_AER_TYPE_NOTICE);
    }

    return nvme_c2h(n, ((uint8_t *)nslist) + off, trans_len, req);
}