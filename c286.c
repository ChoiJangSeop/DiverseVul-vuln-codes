				__acquires(kernel_lock)

{
	struct buffer_head *bh;
	struct ext4_super_block *es = NULL;
	struct ext4_sb_info *sbi;
	ext4_fsblk_t block;
	ext4_fsblk_t sb_block = get_sb_block(&data);
	ext4_fsblk_t logical_sb_block;
	unsigned long offset = 0;
	unsigned long journal_devnum = 0;
	unsigned long def_mount_opts;
	struct inode *root;
	char *cp;
	const char *descr;
	int ret = -EINVAL;
	int blocksize;
	int db_count;
	int i;
	int needs_recovery, has_huge_files;
	int features;
	__u64 blocks_count;
	int err;
	unsigned int journal_ioprio = DEFAULT_JOURNAL_IOPRIO;

	sbi = kzalloc(sizeof(*sbi), GFP_KERNEL);
	if (!sbi)
		return -ENOMEM;
	sb->s_fs_info = sbi;
	sbi->s_mount_opt = 0;
	sbi->s_resuid = EXT4_DEF_RESUID;
	sbi->s_resgid = EXT4_DEF_RESGID;
	sbi->s_inode_readahead_blks = EXT4_DEF_INODE_READAHEAD_BLKS;
	sbi->s_sb_block = sb_block;

	unlock_kernel();

	/* Cleanup superblock name */
	for (cp = sb->s_id; (cp = strchr(cp, '/'));)
		*cp = '!';

	blocksize = sb_min_blocksize(sb, EXT4_MIN_BLOCK_SIZE);
	if (!blocksize) {
		printk(KERN_ERR "EXT4-fs: unable to set blocksize\n");
		goto out_fail;
	}

	/*
	 * The ext4 superblock will not be buffer aligned for other than 1kB
	 * block sizes.  We need to calculate the offset from buffer start.
	 */
	if (blocksize != EXT4_MIN_BLOCK_SIZE) {
		logical_sb_block = sb_block * EXT4_MIN_BLOCK_SIZE;
		offset = do_div(logical_sb_block, blocksize);
	} else {
		logical_sb_block = sb_block;
	}

	if (!(bh = sb_bread(sb, logical_sb_block))) {
		printk(KERN_ERR "EXT4-fs: unable to read superblock\n");
		goto out_fail;
	}
	/*
	 * Note: s_es must be initialized as soon as possible because
	 *       some ext4 macro-instructions depend on its value
	 */
	es = (struct ext4_super_block *) (((char *)bh->b_data) + offset);
	sbi->s_es = es;
	sb->s_magic = le16_to_cpu(es->s_magic);
	if (sb->s_magic != EXT4_SUPER_MAGIC)
		goto cantfind_ext4;

	/* Set defaults before we parse the mount options */
	def_mount_opts = le32_to_cpu(es->s_default_mount_opts);
	if (def_mount_opts & EXT4_DEFM_DEBUG)
		set_opt(sbi->s_mount_opt, DEBUG);
	if (def_mount_opts & EXT4_DEFM_BSDGROUPS)
		set_opt(sbi->s_mount_opt, GRPID);
	if (def_mount_opts & EXT4_DEFM_UID16)
		set_opt(sbi->s_mount_opt, NO_UID32);
#ifdef CONFIG_EXT4_FS_XATTR
	if (def_mount_opts & EXT4_DEFM_XATTR_USER)
		set_opt(sbi->s_mount_opt, XATTR_USER);
#endif
#ifdef CONFIG_EXT4_FS_POSIX_ACL
	if (def_mount_opts & EXT4_DEFM_ACL)
		set_opt(sbi->s_mount_opt, POSIX_ACL);
#endif
	if ((def_mount_opts & EXT4_DEFM_JMODE) == EXT4_DEFM_JMODE_DATA)
		sbi->s_mount_opt |= EXT4_MOUNT_JOURNAL_DATA;
	else if ((def_mount_opts & EXT4_DEFM_JMODE) == EXT4_DEFM_JMODE_ORDERED)
		sbi->s_mount_opt |= EXT4_MOUNT_ORDERED_DATA;
	else if ((def_mount_opts & EXT4_DEFM_JMODE) == EXT4_DEFM_JMODE_WBACK)
		sbi->s_mount_opt |= EXT4_MOUNT_WRITEBACK_DATA;

	if (le16_to_cpu(sbi->s_es->s_errors) == EXT4_ERRORS_PANIC)
		set_opt(sbi->s_mount_opt, ERRORS_PANIC);
	else if (le16_to_cpu(sbi->s_es->s_errors) == EXT4_ERRORS_CONTINUE)
		set_opt(sbi->s_mount_opt, ERRORS_CONT);
	else
		set_opt(sbi->s_mount_opt, ERRORS_RO);

	sbi->s_resuid = le16_to_cpu(es->s_def_resuid);
	sbi->s_resgid = le16_to_cpu(es->s_def_resgid);
	sbi->s_commit_interval = JBD2_DEFAULT_MAX_COMMIT_AGE * HZ;
	sbi->s_min_batch_time = EXT4_DEF_MIN_BATCH_TIME;
	sbi->s_max_batch_time = EXT4_DEF_MAX_BATCH_TIME;

	set_opt(sbi->s_mount_opt, RESERVATION);
	set_opt(sbi->s_mount_opt, BARRIER);

	/*
	 * turn on extents feature by default in ext4 filesystem
	 * only if feature flag already set by mkfs or tune2fs.
	 * Use -o noextents to turn it off
	 */
	if (EXT4_HAS_INCOMPAT_FEATURE(sb, EXT4_FEATURE_INCOMPAT_EXTENTS))
		set_opt(sbi->s_mount_opt, EXTENTS);
	else
		ext4_warning(sb, __func__,
			"extents feature not enabled on this filesystem, "
			"use tune2fs.");

	/*
	 * enable delayed allocation by default
	 * Use -o nodelalloc to turn it off
	 */
	set_opt(sbi->s_mount_opt, DELALLOC);


	if (!parse_options((char *) data, sb, &journal_devnum,
			   &journal_ioprio, NULL, 0))
		goto failed_mount;

	sb->s_flags = (sb->s_flags & ~MS_POSIXACL) |
		((sbi->s_mount_opt & EXT4_MOUNT_POSIX_ACL) ? MS_POSIXACL : 0);

	if (le32_to_cpu(es->s_rev_level) == EXT4_GOOD_OLD_REV &&
	    (EXT4_HAS_COMPAT_FEATURE(sb, ~0U) ||
	     EXT4_HAS_RO_COMPAT_FEATURE(sb, ~0U) ||
	     EXT4_HAS_INCOMPAT_FEATURE(sb, ~0U)))
		printk(KERN_WARNING
		       "EXT4-fs warning: feature flags set on rev 0 fs, "
		       "running e2fsck is recommended\n");

	/*
	 * Check feature flags regardless of the revision level, since we
	 * previously didn't change the revision level when setting the flags,
	 * so there is a chance incompat flags are set on a rev 0 filesystem.
	 */
	features = EXT4_HAS_INCOMPAT_FEATURE(sb, ~EXT4_FEATURE_INCOMPAT_SUPP);
	if (features) {
		printk(KERN_ERR "EXT4-fs: %s: couldn't mount because of "
		       "unsupported optional features (%x).\n", sb->s_id,
			(le32_to_cpu(EXT4_SB(sb)->s_es->s_feature_incompat) &
			~EXT4_FEATURE_INCOMPAT_SUPP));
		goto failed_mount;
	}
	features = EXT4_HAS_RO_COMPAT_FEATURE(sb, ~EXT4_FEATURE_RO_COMPAT_SUPP);
	if (!(sb->s_flags & MS_RDONLY) && features) {
		printk(KERN_ERR "EXT4-fs: %s: couldn't mount RDWR because of "
		       "unsupported optional features (%x).\n", sb->s_id,
			(le32_to_cpu(EXT4_SB(sb)->s_es->s_feature_ro_compat) &
			~EXT4_FEATURE_RO_COMPAT_SUPP));
		goto failed_mount;
	}
	has_huge_files = EXT4_HAS_RO_COMPAT_FEATURE(sb,
				    EXT4_FEATURE_RO_COMPAT_HUGE_FILE);
	if (has_huge_files) {
		/*
		 * Large file size enabled file system can only be
		 * mount if kernel is build with CONFIG_LBD
		 */
		if (sizeof(root->i_blocks) < sizeof(u64) &&
				!(sb->s_flags & MS_RDONLY)) {
			printk(KERN_ERR "EXT4-fs: %s: Filesystem with huge "
					"files cannot be mounted read-write "
					"without CONFIG_LBD.\n", sb->s_id);
			goto failed_mount;
		}
	}
	blocksize = BLOCK_SIZE << le32_to_cpu(es->s_log_block_size);

	if (blocksize < EXT4_MIN_BLOCK_SIZE ||
	    blocksize > EXT4_MAX_BLOCK_SIZE) {
		printk(KERN_ERR
		       "EXT4-fs: Unsupported filesystem blocksize %d on %s.\n",
		       blocksize, sb->s_id);
		goto failed_mount;
	}

	if (sb->s_blocksize != blocksize) {

		/* Validate the filesystem blocksize */
		if (!sb_set_blocksize(sb, blocksize)) {
			printk(KERN_ERR "EXT4-fs: bad block size %d.\n",
					blocksize);
			goto failed_mount;
		}

		brelse(bh);
		logical_sb_block = sb_block * EXT4_MIN_BLOCK_SIZE;
		offset = do_div(logical_sb_block, blocksize);
		bh = sb_bread(sb, logical_sb_block);
		if (!bh) {
			printk(KERN_ERR
			       "EXT4-fs: Can't read superblock on 2nd try.\n");
			goto failed_mount;
		}
		es = (struct ext4_super_block *)(((char *)bh->b_data) + offset);
		sbi->s_es = es;
		if (es->s_magic != cpu_to_le16(EXT4_SUPER_MAGIC)) {
			printk(KERN_ERR
			       "EXT4-fs: Magic mismatch, very weird !\n");
			goto failed_mount;
		}
	}

	sbi->s_bitmap_maxbytes = ext4_max_bitmap_size(sb->s_blocksize_bits,
						      has_huge_files);
	sb->s_maxbytes = ext4_max_size(sb->s_blocksize_bits, has_huge_files);

	if (le32_to_cpu(es->s_rev_level) == EXT4_GOOD_OLD_REV) {
		sbi->s_inode_size = EXT4_GOOD_OLD_INODE_SIZE;
		sbi->s_first_ino = EXT4_GOOD_OLD_FIRST_INO;
	} else {
		sbi->s_inode_size = le16_to_cpu(es->s_inode_size);
		sbi->s_first_ino = le32_to_cpu(es->s_first_ino);
		if ((sbi->s_inode_size < EXT4_GOOD_OLD_INODE_SIZE) ||
		    (!is_power_of_2(sbi->s_inode_size)) ||
		    (sbi->s_inode_size > blocksize)) {
			printk(KERN_ERR
			       "EXT4-fs: unsupported inode size: %d\n",
			       sbi->s_inode_size);
			goto failed_mount;
		}
		if (sbi->s_inode_size > EXT4_GOOD_OLD_INODE_SIZE)
			sb->s_time_gran = 1 << (EXT4_EPOCH_BITS - 2);
	}
	sbi->s_desc_size = le16_to_cpu(es->s_desc_size);
	if (EXT4_HAS_INCOMPAT_FEATURE(sb, EXT4_FEATURE_INCOMPAT_64BIT)) {
		if (sbi->s_desc_size < EXT4_MIN_DESC_SIZE_64BIT ||
		    sbi->s_desc_size > EXT4_MAX_DESC_SIZE ||
		    !is_power_of_2(sbi->s_desc_size)) {
			printk(KERN_ERR
			       "EXT4-fs: unsupported descriptor size %lu\n",
			       sbi->s_desc_size);
			goto failed_mount;
		}
	} else
		sbi->s_desc_size = EXT4_MIN_DESC_SIZE;
	sbi->s_blocks_per_group = le32_to_cpu(es->s_blocks_per_group);
	sbi->s_inodes_per_group = le32_to_cpu(es->s_inodes_per_group);
	if (EXT4_INODE_SIZE(sb) == 0 || EXT4_INODES_PER_GROUP(sb) == 0)
		goto cantfind_ext4;
	sbi->s_inodes_per_block = blocksize / EXT4_INODE_SIZE(sb);
	if (sbi->s_inodes_per_block == 0)
		goto cantfind_ext4;
	sbi->s_itb_per_group = sbi->s_inodes_per_group /
					sbi->s_inodes_per_block;
	sbi->s_desc_per_block = blocksize / EXT4_DESC_SIZE(sb);
	sbi->s_sbh = bh;
	sbi->s_mount_state = le16_to_cpu(es->s_state);
	sbi->s_addr_per_block_bits = ilog2(EXT4_ADDR_PER_BLOCK(sb));
	sbi->s_desc_per_block_bits = ilog2(EXT4_DESC_PER_BLOCK(sb));
	for (i = 0; i < 4; i++)
		sbi->s_hash_seed[i] = le32_to_cpu(es->s_hash_seed[i]);
	sbi->s_def_hash_version = es->s_def_hash_version;
	i = le32_to_cpu(es->s_flags);
	if (i & EXT2_FLAGS_UNSIGNED_HASH)
		sbi->s_hash_unsigned = 3;
	else if ((i & EXT2_FLAGS_SIGNED_HASH) == 0) {
#ifdef __CHAR_UNSIGNED__
		es->s_flags |= cpu_to_le32(EXT2_FLAGS_UNSIGNED_HASH);
		sbi->s_hash_unsigned = 3;
#else
		es->s_flags |= cpu_to_le32(EXT2_FLAGS_SIGNED_HASH);
#endif
		sb->s_dirt = 1;
	}

	if (sbi->s_blocks_per_group > blocksize * 8) {
		printk(KERN_ERR
		       "EXT4-fs: #blocks per group too big: %lu\n",
		       sbi->s_blocks_per_group);
		goto failed_mount;
	}
	if (sbi->s_inodes_per_group > blocksize * 8) {
		printk(KERN_ERR
		       "EXT4-fs: #inodes per group too big: %lu\n",
		       sbi->s_inodes_per_group);
		goto failed_mount;
	}

	if (ext4_blocks_count(es) >
		    (sector_t)(~0ULL) >> (sb->s_blocksize_bits - 9)) {
		printk(KERN_ERR "EXT4-fs: filesystem on %s:"
			" too large to mount safely\n", sb->s_id);
		if (sizeof(sector_t) < 8)
			printk(KERN_WARNING "EXT4-fs: CONFIG_LBD not "
					"enabled\n");
		goto failed_mount;
	}

	if (EXT4_BLOCKS_PER_GROUP(sb) == 0)
		goto cantfind_ext4;

	/* ensure blocks_count calculation below doesn't sign-extend */
	if (ext4_blocks_count(es) + EXT4_BLOCKS_PER_GROUP(sb) <
	    le32_to_cpu(es->s_first_data_block) + 1) {
		printk(KERN_WARNING "EXT4-fs: bad geometry: block count %llu, "
		       "first data block %u, blocks per group %lu\n",
			ext4_blocks_count(es),
			le32_to_cpu(es->s_first_data_block),
			EXT4_BLOCKS_PER_GROUP(sb));
		goto failed_mount;
	}
	blocks_count = (ext4_blocks_count(es) -
			le32_to_cpu(es->s_first_data_block) +
			EXT4_BLOCKS_PER_GROUP(sb) - 1);
	do_div(blocks_count, EXT4_BLOCKS_PER_GROUP(sb));
	sbi->s_groups_count = blocks_count;
	db_count = (sbi->s_groups_count + EXT4_DESC_PER_BLOCK(sb) - 1) /
		   EXT4_DESC_PER_BLOCK(sb);
	sbi->s_group_desc = kmalloc(db_count * sizeof(struct buffer_head *),
				    GFP_KERNEL);
	if (sbi->s_group_desc == NULL) {
		printk(KERN_ERR "EXT4-fs: not enough memory\n");
		goto failed_mount;
	}

#ifdef CONFIG_PROC_FS
	if (ext4_proc_root)
		sbi->s_proc = proc_mkdir(sb->s_id, ext4_proc_root);

	if (sbi->s_proc)
		proc_create_data("inode_readahead_blks", 0644, sbi->s_proc,
				 &ext4_ui_proc_fops,
				 &sbi->s_inode_readahead_blks);
#endif

	bgl_lock_init(&sbi->s_blockgroup_lock);

	for (i = 0; i < db_count; i++) {
		block = descriptor_loc(sb, logical_sb_block, i);
		sbi->s_group_desc[i] = sb_bread(sb, block);
		if (!sbi->s_group_desc[i]) {
			printk(KERN_ERR "EXT4-fs: "
			       "can't read group descriptor %d\n", i);
			db_count = i;
			goto failed_mount2;
		}
	}
	if (!ext4_check_descriptors(sb)) {
		printk(KERN_ERR "EXT4-fs: group descriptors corrupted!\n");
		goto failed_mount2;
	}
	if (EXT4_HAS_INCOMPAT_FEATURE(sb, EXT4_FEATURE_INCOMPAT_FLEX_BG))
		if (!ext4_fill_flex_info(sb)) {
			printk(KERN_ERR
			       "EXT4-fs: unable to initialize "
			       "flex_bg meta info!\n");
			goto failed_mount2;
		}

	sbi->s_gdb_count = db_count;
	get_random_bytes(&sbi->s_next_generation, sizeof(u32));
	spin_lock_init(&sbi->s_next_gen_lock);

	err = percpu_counter_init(&sbi->s_freeblocks_counter,
			ext4_count_free_blocks(sb));
	if (!err) {
		err = percpu_counter_init(&sbi->s_freeinodes_counter,
				ext4_count_free_inodes(sb));
	}
	if (!err) {
		err = percpu_counter_init(&sbi->s_dirs_counter,
				ext4_count_dirs(sb));
	}
	if (!err) {
		err = percpu_counter_init(&sbi->s_dirtyblocks_counter, 0);
	}
	if (err) {
		printk(KERN_ERR "EXT4-fs: insufficient memory\n");
		goto failed_mount3;
	}

	sbi->s_stripe = ext4_get_stripe_size(sbi);

	/*
	 * set up enough so that it can read an inode
	 */
	sb->s_op = &ext4_sops;
	sb->s_export_op = &ext4_export_ops;
	sb->s_xattr = ext4_xattr_handlers;
#ifdef CONFIG_QUOTA
	sb->s_qcop = &ext4_qctl_operations;
	sb->dq_op = &ext4_quota_operations;
#endif
	INIT_LIST_HEAD(&sbi->s_orphan); /* unlinked but open files */

	sb->s_root = NULL;

	needs_recovery = (es->s_last_orphan != 0 ||
			  EXT4_HAS_INCOMPAT_FEATURE(sb,
				    EXT4_FEATURE_INCOMPAT_RECOVER));

	/*
	 * The first inode we look at is the journal inode.  Don't try
	 * root first: it may be modified in the journal!
	 */
	if (!test_opt(sb, NOLOAD) &&
	    EXT4_HAS_COMPAT_FEATURE(sb, EXT4_FEATURE_COMPAT_HAS_JOURNAL)) {
		if (ext4_load_journal(sb, es, journal_devnum))
			goto failed_mount3;
		if (!(sb->s_flags & MS_RDONLY) &&
		    EXT4_SB(sb)->s_journal->j_failed_commit) {
			printk(KERN_CRIT "EXT4-fs error (device %s): "
			       "ext4_fill_super: Journal transaction "
			       "%u is corrupt\n", sb->s_id,
			       EXT4_SB(sb)->s_journal->j_failed_commit);
			if (test_opt(sb, ERRORS_RO)) {
				printk(KERN_CRIT
				       "Mounting filesystem read-only\n");
				sb->s_flags |= MS_RDONLY;
				EXT4_SB(sb)->s_mount_state |= EXT4_ERROR_FS;
				es->s_state |= cpu_to_le16(EXT4_ERROR_FS);
			}
			if (test_opt(sb, ERRORS_PANIC)) {
				EXT4_SB(sb)->s_mount_state |= EXT4_ERROR_FS;
				es->s_state |= cpu_to_le16(EXT4_ERROR_FS);
				ext4_commit_super(sb, es, 1);
				goto failed_mount4;
			}
		}
	} else if (test_opt(sb, NOLOAD) && !(sb->s_flags & MS_RDONLY) &&
	      EXT4_HAS_INCOMPAT_FEATURE(sb, EXT4_FEATURE_INCOMPAT_RECOVER)) {
		printk(KERN_ERR "EXT4-fs: required journal recovery "
		       "suppressed and not mounted read-only\n");
		goto failed_mount4;
	} else {
		clear_opt(sbi->s_mount_opt, DATA_FLAGS);
		set_opt(sbi->s_mount_opt, WRITEBACK_DATA);
		sbi->s_journal = NULL;
		needs_recovery = 0;
		goto no_journal;
	}

	if (ext4_blocks_count(es) > 0xffffffffULL &&
	    !jbd2_journal_set_features(EXT4_SB(sb)->s_journal, 0, 0,
				       JBD2_FEATURE_INCOMPAT_64BIT)) {
		printk(KERN_ERR "ext4: Failed to set 64-bit journal feature\n");
		goto failed_mount4;
	}

	if (test_opt(sb, JOURNAL_ASYNC_COMMIT)) {
		jbd2_journal_set_features(sbi->s_journal,
				JBD2_FEATURE_COMPAT_CHECKSUM, 0,
				JBD2_FEATURE_INCOMPAT_ASYNC_COMMIT);
	} else if (test_opt(sb, JOURNAL_CHECKSUM)) {
		jbd2_journal_set_features(sbi->s_journal,
				JBD2_FEATURE_COMPAT_CHECKSUM, 0, 0);
		jbd2_journal_clear_features(sbi->s_journal, 0, 0,
				JBD2_FEATURE_INCOMPAT_ASYNC_COMMIT);
	} else {
		jbd2_journal_clear_features(sbi->s_journal,
				JBD2_FEATURE_COMPAT_CHECKSUM, 0,
				JBD2_FEATURE_INCOMPAT_ASYNC_COMMIT);
	}

	/* We have now updated the journal if required, so we can
	 * validate the data journaling mode. */
	switch (test_opt(sb, DATA_FLAGS)) {
	case 0:
		/* No mode set, assume a default based on the journal
		 * capabilities: ORDERED_DATA if the journal can
		 * cope, else JOURNAL_DATA
		 */
		if (jbd2_journal_check_available_features
		    (sbi->s_journal, 0, 0, JBD2_FEATURE_INCOMPAT_REVOKE))
			set_opt(sbi->s_mount_opt, ORDERED_DATA);
		else
			set_opt(sbi->s_mount_opt, JOURNAL_DATA);
		break;

	case EXT4_MOUNT_ORDERED_DATA:
	case EXT4_MOUNT_WRITEBACK_DATA:
		if (!jbd2_journal_check_available_features
		    (sbi->s_journal, 0, 0, JBD2_FEATURE_INCOMPAT_REVOKE)) {
			printk(KERN_ERR "EXT4-fs: Journal does not support "
			       "requested data journaling mode\n");
			goto failed_mount4;
		}
	default:
		break;
	}
	set_task_ioprio(sbi->s_journal->j_task, journal_ioprio);

no_journal:

	if (test_opt(sb, NOBH)) {
		if (!(test_opt(sb, DATA_FLAGS) == EXT4_MOUNT_WRITEBACK_DATA)) {
			printk(KERN_WARNING "EXT4-fs: Ignoring nobh option - "
				"its supported only with writeback mode\n");
			clear_opt(sbi->s_mount_opt, NOBH);
		}
	}
	/*
	 * The jbd2_journal_load will have done any necessary log recovery,
	 * so we can safely mount the rest of the filesystem now.
	 */

	root = ext4_iget(sb, EXT4_ROOT_INO);
	if (IS_ERR(root)) {
		printk(KERN_ERR "EXT4-fs: get root inode failed\n");
		ret = PTR_ERR(root);
		goto failed_mount4;
	}
	if (!S_ISDIR(root->i_mode) || !root->i_blocks || !root->i_size) {
		iput(root);
		printk(KERN_ERR "EXT4-fs: corrupt root inode, run e2fsck\n");
		goto failed_mount4;
	}
	sb->s_root = d_alloc_root(root);
	if (!sb->s_root) {
		printk(KERN_ERR "EXT4-fs: get root dentry failed\n");
		iput(root);
		ret = -ENOMEM;
		goto failed_mount4;
	}

	ext4_setup_super(sb, es, sb->s_flags & MS_RDONLY);

	/* determine the minimum size of new large inodes, if present */
	if (sbi->s_inode_size > EXT4_GOOD_OLD_INODE_SIZE) {
		sbi->s_want_extra_isize = sizeof(struct ext4_inode) -
						     EXT4_GOOD_OLD_INODE_SIZE;
		if (EXT4_HAS_RO_COMPAT_FEATURE(sb,
				       EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE)) {
			if (sbi->s_want_extra_isize <
			    le16_to_cpu(es->s_want_extra_isize))
				sbi->s_want_extra_isize =
					le16_to_cpu(es->s_want_extra_isize);
			if (sbi->s_want_extra_isize <
			    le16_to_cpu(es->s_min_extra_isize))
				sbi->s_want_extra_isize =
					le16_to_cpu(es->s_min_extra_isize);
		}
	}
	/* Check if enough inode space is available */
	if (EXT4_GOOD_OLD_INODE_SIZE + sbi->s_want_extra_isize >
							sbi->s_inode_size) {
		sbi->s_want_extra_isize = sizeof(struct ext4_inode) -
						       EXT4_GOOD_OLD_INODE_SIZE;
		printk(KERN_INFO "EXT4-fs: required extra inode space not"
			"available.\n");
	}

	if (test_opt(sb, DATA_FLAGS) == EXT4_MOUNT_JOURNAL_DATA) {
		printk(KERN_WARNING "EXT4-fs: Ignoring delalloc option - "
				"requested data journaling mode\n");
		clear_opt(sbi->s_mount_opt, DELALLOC);
	} else if (test_opt(sb, DELALLOC))
		printk(KERN_INFO "EXT4-fs: delayed allocation enabled\n");

	ext4_ext_init(sb);
	err = ext4_mb_init(sb, needs_recovery);
	if (err) {
		printk(KERN_ERR "EXT4-fs: failed to initalize mballoc (%d)\n",
		       err);
		goto failed_mount4;
	}

	/*
	 * akpm: core read_super() calls in here with the superblock locked.
	 * That deadlocks, because orphan cleanup needs to lock the superblock
	 * in numerous places.  Here we just pop the lock - it's relatively
	 * harmless, because we are now ready to accept write_super() requests,
	 * and aviro says that's the only reason for hanging onto the
	 * superblock lock.
	 */
	EXT4_SB(sb)->s_mount_state |= EXT4_ORPHAN_FS;
	ext4_orphan_cleanup(sb, es);
	EXT4_SB(sb)->s_mount_state &= ~EXT4_ORPHAN_FS;
	if (needs_recovery) {
		printk(KERN_INFO "EXT4-fs: recovery complete.\n");
		ext4_mark_recovery_complete(sb, es);
	}
	if (EXT4_SB(sb)->s_journal) {
		if (test_opt(sb, DATA_FLAGS) == EXT4_MOUNT_JOURNAL_DATA)
			descr = " journalled data mode";
		else if (test_opt(sb, DATA_FLAGS) == EXT4_MOUNT_ORDERED_DATA)
			descr = " ordered data mode";
		else
			descr = " writeback data mode";
	} else
		descr = "out journal";

	printk(KERN_INFO "EXT4-fs: mounted filesystem %s with%s\n",
	       sb->s_id, descr);

	lock_kernel();
	return 0;

cantfind_ext4:
	if (!silent)
		printk(KERN_ERR "VFS: Can't find ext4 filesystem on dev %s.\n",
		       sb->s_id);
	goto failed_mount;

failed_mount4:
	printk(KERN_ERR "EXT4-fs (device %s): mount failed\n", sb->s_id);
	if (sbi->s_journal) {
		jbd2_journal_destroy(sbi->s_journal);
		sbi->s_journal = NULL;
	}
failed_mount3:
	percpu_counter_destroy(&sbi->s_freeblocks_counter);
	percpu_counter_destroy(&sbi->s_freeinodes_counter);
	percpu_counter_destroy(&sbi->s_dirs_counter);
	percpu_counter_destroy(&sbi->s_dirtyblocks_counter);
failed_mount2:
	for (i = 0; i < db_count; i++)
		brelse(sbi->s_group_desc[i]);
	kfree(sbi->s_group_desc);
failed_mount:
	if (sbi->s_proc) {
		remove_proc_entry("inode_readahead_blks", sbi->s_proc);
		remove_proc_entry(sb->s_id, ext4_proc_root);
	}
#ifdef CONFIG_QUOTA
	for (i = 0; i < MAXQUOTAS; i++)
		kfree(sbi->s_qf_names[i]);
#endif
	ext4_blkdev_remove(sbi);
	brelse(bh);
out_fail:
	sb->s_fs_info = NULL;
	kfree(sbi);
	lock_kernel();
	return ret;
}