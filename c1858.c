int qcow2_snapshot_load_tmp(BlockDriverState *bs,
                            const char *snapshot_id,
                            const char *name,
                            Error **errp)
{
    int i, snapshot_index;
    BDRVQcowState *s = bs->opaque;
    QCowSnapshot *sn;
    uint64_t *new_l1_table;
    int new_l1_bytes;
    int ret;

    assert(bs->read_only);

    /* Search the snapshot */
    snapshot_index = find_snapshot_by_id_and_name(bs, snapshot_id, name);
    if (snapshot_index < 0) {
        error_setg(errp,
                   "Can't find snapshot");
        return -ENOENT;
    }
    sn = &s->snapshots[snapshot_index];

    /* Allocate and read in the snapshot's L1 table */
    new_l1_bytes = s->l1_size * sizeof(uint64_t);
    new_l1_table = g_malloc0(align_offset(new_l1_bytes, 512));

    ret = bdrv_pread(bs->file, sn->l1_table_offset, new_l1_table, new_l1_bytes);
    if (ret < 0) {
        error_setg(errp, "Failed to read l1 table for snapshot");
        g_free(new_l1_table);
        return ret;
    }

    /* Switch the L1 table */
    g_free(s->l1_table);

    s->l1_size = sn->l1_size;
    s->l1_table_offset = sn->l1_table_offset;
    s->l1_table = new_l1_table;

    for(i = 0;i < s->l1_size; i++) {
        be64_to_cpus(&s->l1_table[i]);
    }

    return 0;
}