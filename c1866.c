static int qcow2_open(BlockDriverState *bs, QDict *options, int flags,
                      Error **errp)
{
    BDRVQcowState *s = bs->opaque;
    unsigned int len, i;
    int ret = 0;
    QCowHeader header;
    QemuOpts *opts;
    Error *local_err = NULL;
    uint64_t ext_end;
    uint64_t l1_vm_state_index;
    const char *opt_overlap_check;
    int overlap_check_template = 0;

    ret = bdrv_pread(bs->file, 0, &header, sizeof(header));
    if (ret < 0) {
        error_setg_errno(errp, -ret, "Could not read qcow2 header");
        goto fail;
    }
    be32_to_cpus(&header.magic);
    be32_to_cpus(&header.version);
    be64_to_cpus(&header.backing_file_offset);
    be32_to_cpus(&header.backing_file_size);
    be64_to_cpus(&header.size);
    be32_to_cpus(&header.cluster_bits);
    be32_to_cpus(&header.crypt_method);
    be64_to_cpus(&header.l1_table_offset);
    be32_to_cpus(&header.l1_size);
    be64_to_cpus(&header.refcount_table_offset);
    be32_to_cpus(&header.refcount_table_clusters);
    be64_to_cpus(&header.snapshots_offset);
    be32_to_cpus(&header.nb_snapshots);

    if (header.magic != QCOW_MAGIC) {
        error_setg(errp, "Image is not in qcow2 format");
        ret = -EINVAL;
        goto fail;
    }
    if (header.version < 2 || header.version > 3) {
        report_unsupported(bs, errp, "QCOW version %d", header.version);
        ret = -ENOTSUP;
        goto fail;
    }

    s->qcow_version = header.version;

    /* Initialise cluster size */
    if (header.cluster_bits < MIN_CLUSTER_BITS ||
        header.cluster_bits > MAX_CLUSTER_BITS) {
        error_setg(errp, "Unsupported cluster size: 2^%i", header.cluster_bits);
        ret = -EINVAL;
        goto fail;
    }

    s->cluster_bits = header.cluster_bits;
    s->cluster_size = 1 << s->cluster_bits;
    s->cluster_sectors = 1 << (s->cluster_bits - 9);

    /* Initialise version 3 header fields */
    if (header.version == 2) {
        header.incompatible_features    = 0;
        header.compatible_features      = 0;
        header.autoclear_features       = 0;
        header.refcount_order           = 4;
        header.header_length            = 72;
    } else {
        be64_to_cpus(&header.incompatible_features);
        be64_to_cpus(&header.compatible_features);
        be64_to_cpus(&header.autoclear_features);
        be32_to_cpus(&header.refcount_order);
        be32_to_cpus(&header.header_length);

        if (header.header_length < 104) {
            error_setg(errp, "qcow2 header too short");
            ret = -EINVAL;
            goto fail;
        }
    }

    if (header.header_length > s->cluster_size) {
        error_setg(errp, "qcow2 header exceeds cluster size");
        ret = -EINVAL;
        goto fail;
    }

    if (header.header_length > sizeof(header)) {
        s->unknown_header_fields_size = header.header_length - sizeof(header);
        s->unknown_header_fields = g_malloc(s->unknown_header_fields_size);
        ret = bdrv_pread(bs->file, sizeof(header), s->unknown_header_fields,
                         s->unknown_header_fields_size);
        if (ret < 0) {
            error_setg_errno(errp, -ret, "Could not read unknown qcow2 header "
                             "fields");
            goto fail;
        }
    }

    if (header.backing_file_offset > s->cluster_size) {
        error_setg(errp, "Invalid backing file offset");
        ret = -EINVAL;
        goto fail;
    }

    if (header.backing_file_offset) {
        ext_end = header.backing_file_offset;
    } else {
        ext_end = 1 << header.cluster_bits;
    }

    /* Handle feature bits */
    s->incompatible_features    = header.incompatible_features;
    s->compatible_features      = header.compatible_features;
    s->autoclear_features       = header.autoclear_features;

    if (s->incompatible_features & ~QCOW2_INCOMPAT_MASK) {
        void *feature_table = NULL;
        qcow2_read_extensions(bs, header.header_length, ext_end,
                              &feature_table, NULL);
        report_unsupported_feature(bs, errp, feature_table,
                                   s->incompatible_features &
                                   ~QCOW2_INCOMPAT_MASK);
        ret = -ENOTSUP;
        g_free(feature_table);
        goto fail;
    }

    if (s->incompatible_features & QCOW2_INCOMPAT_CORRUPT) {
        /* Corrupt images may not be written to unless they are being repaired
         */
        if ((flags & BDRV_O_RDWR) && !(flags & BDRV_O_CHECK)) {
            error_setg(errp, "qcow2: Image is corrupt; cannot be opened "
                       "read/write");
            ret = -EACCES;
            goto fail;
        }
    }

    /* Check support for various header values */
    if (header.refcount_order != 4) {
        report_unsupported(bs, errp, "%d bit reference counts",
                           1 << header.refcount_order);
        ret = -ENOTSUP;
        goto fail;
    }
    s->refcount_order = header.refcount_order;

    if (header.crypt_method > QCOW_CRYPT_AES) {
        error_setg(errp, "Unsupported encryption method: %i",
                   header.crypt_method);
        ret = -EINVAL;
        goto fail;
    }
    s->crypt_method_header = header.crypt_method;
    if (s->crypt_method_header) {
        bs->encrypted = 1;
    }

    s->l2_bits = s->cluster_bits - 3; /* L2 is always one cluster */
    s->l2_size = 1 << s->l2_bits;
    bs->total_sectors = header.size / 512;
    s->csize_shift = (62 - (s->cluster_bits - 8));
    s->csize_mask = (1 << (s->cluster_bits - 8)) - 1;
    s->cluster_offset_mask = (1LL << s->csize_shift) - 1;

    s->refcount_table_offset = header.refcount_table_offset;
    s->refcount_table_size =
        header.refcount_table_clusters << (s->cluster_bits - 3);

    if (header.refcount_table_clusters > qcow2_max_refcount_clusters(s)) {
        error_setg(errp, "Reference count table too large");
        ret = -EINVAL;
        goto fail;
    }

    ret = validate_table_offset(bs, s->refcount_table_offset,
                                s->refcount_table_size, sizeof(uint64_t));
    if (ret < 0) {
        error_setg(errp, "Invalid reference count table offset");
        goto fail;
    }

    /* Snapshot table offset/length */
    if (header.nb_snapshots > QCOW_MAX_SNAPSHOTS) {
        error_setg(errp, "Too many snapshots");
        ret = -EINVAL;
        goto fail;
    }

    ret = validate_table_offset(bs, header.snapshots_offset,
                                header.nb_snapshots,
                                sizeof(QCowSnapshotHeader));
    if (ret < 0) {
        error_setg(errp, "Invalid snapshot table offset");
        goto fail;
    }

    /* read the level 1 table */
    if (header.l1_size > 0x2000000) {
        /* 32 MB L1 table is enough for 2 PB images at 64k cluster size
         * (128 GB for 512 byte clusters, 2 EB for 2 MB clusters) */
        error_setg(errp, "Active L1 table too large");
        ret = -EFBIG;
        goto fail;
    }
    s->l1_size = header.l1_size;

    l1_vm_state_index = size_to_l1(s, header.size);
    if (l1_vm_state_index > INT_MAX) {
        error_setg(errp, "Image is too big");
        ret = -EFBIG;
        goto fail;
    }
    s->l1_vm_state_index = l1_vm_state_index;

    /* the L1 table must contain at least enough entries to put
       header.size bytes */
    if (s->l1_size < s->l1_vm_state_index) {
        error_setg(errp, "L1 table is too small");
        ret = -EINVAL;
        goto fail;
    }

    ret = validate_table_offset(bs, header.l1_table_offset,
                                header.l1_size, sizeof(uint64_t));
    if (ret < 0) {
        error_setg(errp, "Invalid L1 table offset");
        goto fail;
    }
    s->l1_table_offset = header.l1_table_offset;


    if (s->l1_size > 0) {
        s->l1_table = g_malloc0(
            align_offset(s->l1_size * sizeof(uint64_t), 512));
        ret = bdrv_pread(bs->file, s->l1_table_offset, s->l1_table,
                         s->l1_size * sizeof(uint64_t));
        if (ret < 0) {
            error_setg_errno(errp, -ret, "Could not read L1 table");
            goto fail;
        }
        for(i = 0;i < s->l1_size; i++) {
            be64_to_cpus(&s->l1_table[i]);
        }
    }

    /* alloc L2 table/refcount block cache */
    s->l2_table_cache = qcow2_cache_create(bs, L2_CACHE_SIZE);
    s->refcount_block_cache = qcow2_cache_create(bs, REFCOUNT_CACHE_SIZE);

    s->cluster_cache = g_malloc(s->cluster_size);
    /* one more sector for decompressed data alignment */
    s->cluster_data = qemu_blockalign(bs, QCOW_MAX_CRYPT_CLUSTERS * s->cluster_size
                                  + 512);
    s->cluster_cache_offset = -1;
    s->flags = flags;

    ret = qcow2_refcount_init(bs);
    if (ret != 0) {
        error_setg_errno(errp, -ret, "Could not initialize refcount handling");
        goto fail;
    }

    QLIST_INIT(&s->cluster_allocs);
    QTAILQ_INIT(&s->discards);

    /* read qcow2 extensions */
    if (qcow2_read_extensions(bs, header.header_length, ext_end, NULL,
        &local_err)) {
        error_propagate(errp, local_err);
        ret = -EINVAL;
        goto fail;
    }

    /* read the backing file name */
    if (header.backing_file_offset != 0) {
        len = header.backing_file_size;
        if (len > MIN(1023, s->cluster_size - header.backing_file_offset)) {
            error_setg(errp, "Backing file name too long");
            ret = -EINVAL;
            goto fail;
        }
        ret = bdrv_pread(bs->file, header.backing_file_offset,
                         bs->backing_file, len);
        if (ret < 0) {
            error_setg_errno(errp, -ret, "Could not read backing file name");
            goto fail;
        }
        bs->backing_file[len] = '\0';
    }

    /* Internal snapshots */
    s->snapshots_offset = header.snapshots_offset;
    s->nb_snapshots = header.nb_snapshots;

    ret = qcow2_read_snapshots(bs);
    if (ret < 0) {
        error_setg_errno(errp, -ret, "Could not read snapshots");
        goto fail;
    }

    /* Clear unknown autoclear feature bits */
    if (!bs->read_only && !(flags & BDRV_O_INCOMING) && s->autoclear_features) {
        s->autoclear_features = 0;
        ret = qcow2_update_header(bs);
        if (ret < 0) {
            error_setg_errno(errp, -ret, "Could not update qcow2 header");
            goto fail;
        }
    }

    /* Initialise locks */
    qemu_co_mutex_init(&s->lock);

    /* Repair image if dirty */
    if (!(flags & (BDRV_O_CHECK | BDRV_O_INCOMING)) && !bs->read_only &&
        (s->incompatible_features & QCOW2_INCOMPAT_DIRTY)) {
        BdrvCheckResult result = {0};

        ret = qcow2_check(bs, &result, BDRV_FIX_ERRORS);
        if (ret < 0) {
            error_setg_errno(errp, -ret, "Could not repair dirty image");
            goto fail;
        }
    }

    /* Enable lazy_refcounts according to image and command line options */
    opts = qemu_opts_create(&qcow2_runtime_opts, NULL, 0, &error_abort);
    qemu_opts_absorb_qdict(opts, options, &local_err);
    if (local_err) {
        error_propagate(errp, local_err);
        ret = -EINVAL;
        goto fail;
    }

    s->use_lazy_refcounts = qemu_opt_get_bool(opts, QCOW2_OPT_LAZY_REFCOUNTS,
        (s->compatible_features & QCOW2_COMPAT_LAZY_REFCOUNTS));

    s->discard_passthrough[QCOW2_DISCARD_NEVER] = false;
    s->discard_passthrough[QCOW2_DISCARD_ALWAYS] = true;
    s->discard_passthrough[QCOW2_DISCARD_REQUEST] =
        qemu_opt_get_bool(opts, QCOW2_OPT_DISCARD_REQUEST,
                          flags & BDRV_O_UNMAP);
    s->discard_passthrough[QCOW2_DISCARD_SNAPSHOT] =
        qemu_opt_get_bool(opts, QCOW2_OPT_DISCARD_SNAPSHOT, true);
    s->discard_passthrough[QCOW2_DISCARD_OTHER] =
        qemu_opt_get_bool(opts, QCOW2_OPT_DISCARD_OTHER, false);

    opt_overlap_check = qemu_opt_get(opts, "overlap-check") ?: "cached";
    if (!strcmp(opt_overlap_check, "none")) {
        overlap_check_template = 0;
    } else if (!strcmp(opt_overlap_check, "constant")) {
        overlap_check_template = QCOW2_OL_CONSTANT;
    } else if (!strcmp(opt_overlap_check, "cached")) {
        overlap_check_template = QCOW2_OL_CACHED;
    } else if (!strcmp(opt_overlap_check, "all")) {
        overlap_check_template = QCOW2_OL_ALL;
    } else {
        error_setg(errp, "Unsupported value '%s' for qcow2 option "
                   "'overlap-check'. Allowed are either of the following: "
                   "none, constant, cached, all", opt_overlap_check);
        qemu_opts_del(opts);
        ret = -EINVAL;
        goto fail;
    }

    s->overlap_check = 0;
    for (i = 0; i < QCOW2_OL_MAX_BITNR; i++) {
        /* overlap-check defines a template bitmask, but every flag may be
         * overwritten through the associated boolean option */
        s->overlap_check |=
            qemu_opt_get_bool(opts, overlap_bool_option_names[i],
                              overlap_check_template & (1 << i)) << i;
    }

    qemu_opts_del(opts);

    if (s->use_lazy_refcounts && s->qcow_version < 3) {
        error_setg(errp, "Lazy refcounts require a qcow2 image with at least "
                   "qemu 1.1 compatibility level");
        ret = -EINVAL;
        goto fail;
    }

#ifdef DEBUG_ALLOC
    {
        BdrvCheckResult result = {0};
        qcow2_check_refcounts(bs, &result, 0);
    }
#endif
    return ret;

 fail:
    g_free(s->unknown_header_fields);
    cleanup_unknown_header_ext(bs);
    qcow2_free_snapshots(bs);
    qcow2_refcount_close(bs);
    g_free(s->l1_table);
    /* else pre-write overlap checks in cache_destroy may crash */
    s->l1_table = NULL;
    if (s->l2_table_cache) {
        qcow2_cache_destroy(bs, s->l2_table_cache);
    }
    if (s->refcount_block_cache) {
        qcow2_cache_destroy(bs, s->refcount_block_cache);
    }
    g_free(s->cluster_cache);
    qemu_vfree(s->cluster_data);
    return ret;
}