static int udf_readdir(struct file *file, struct dir_context *ctx)
{
	struct inode *dir = file_inode(file);
	struct udf_inode_info *iinfo = UDF_I(dir);
	struct udf_fileident_bh fibh = { .sbh = NULL, .ebh = NULL};
	struct fileIdentDesc *fi = NULL;
	struct fileIdentDesc cfi;
	int block, iblock;
	loff_t nf_pos;
	int flen;
	unsigned char *fname = NULL;
	unsigned char *nameptr;
	uint16_t liu;
	uint8_t lfi;
	loff_t size = udf_ext0_offset(dir) + dir->i_size;
	struct buffer_head *tmp, *bha[16];
	struct kernel_lb_addr eloc;
	uint32_t elen;
	sector_t offset;
	int i, num, ret = 0;
	struct extent_position epos = { NULL, 0, {0, 0} };

	if (ctx->pos == 0) {
		if (!dir_emit_dot(file, ctx))
			return 0;
		ctx->pos = 1;
	}
	nf_pos = (ctx->pos - 1) << 2;
	if (nf_pos >= size)
		goto out;

	fname = kmalloc(UDF_NAME_LEN, GFP_NOFS);
	if (!fname) {
		ret = -ENOMEM;
		goto out;
	}

	if (nf_pos == 0)
		nf_pos = udf_ext0_offset(dir);

	fibh.soffset = fibh.eoffset = nf_pos & (dir->i_sb->s_blocksize - 1);
	if (iinfo->i_alloc_type != ICBTAG_FLAG_AD_IN_ICB) {
		if (inode_bmap(dir, nf_pos >> dir->i_sb->s_blocksize_bits,
		    &epos, &eloc, &elen, &offset)
		    != (EXT_RECORDED_ALLOCATED >> 30)) {
			ret = -ENOENT;
			goto out;
		}
		block = udf_get_lb_pblock(dir->i_sb, &eloc, offset);
		if ((++offset << dir->i_sb->s_blocksize_bits) < elen) {
			if (iinfo->i_alloc_type == ICBTAG_FLAG_AD_SHORT)
				epos.offset -= sizeof(struct short_ad);
			else if (iinfo->i_alloc_type ==
					ICBTAG_FLAG_AD_LONG)
				epos.offset -= sizeof(struct long_ad);
		} else {
			offset = 0;
		}

		if (!(fibh.sbh = fibh.ebh = udf_tread(dir->i_sb, block))) {
			ret = -EIO;
			goto out;
		}

		if (!(offset & ((16 >> (dir->i_sb->s_blocksize_bits - 9)) - 1))) {
			i = 16 >> (dir->i_sb->s_blocksize_bits - 9);
			if (i + offset > (elen >> dir->i_sb->s_blocksize_bits))
				i = (elen >> dir->i_sb->s_blocksize_bits) - offset;
			for (num = 0; i > 0; i--) {
				block = udf_get_lb_pblock(dir->i_sb, &eloc, offset + i);
				tmp = udf_tgetblk(dir->i_sb, block);
				if (tmp && !buffer_uptodate(tmp) && !buffer_locked(tmp))
					bha[num++] = tmp;
				else
					brelse(tmp);
			}
			if (num) {
				ll_rw_block(READA, num, bha);
				for (i = 0; i < num; i++)
					brelse(bha[i]);
			}
		}
	}

	while (nf_pos < size) {
		struct kernel_lb_addr tloc;

		ctx->pos = (nf_pos >> 2) + 1;

		fi = udf_fileident_read(dir, &nf_pos, &fibh, &cfi, &epos, &eloc,
					&elen, &offset);
		if (!fi)
			goto out;

		liu = le16_to_cpu(cfi.lengthOfImpUse);
		lfi = cfi.lengthFileIdent;

		if (fibh.sbh == fibh.ebh) {
			nameptr = fi->fileIdent + liu;
		} else {
			int poffset;	/* Unpaded ending offset */

			poffset = fibh.soffset + sizeof(struct fileIdentDesc) + liu + lfi;

			if (poffset >= lfi) {
				nameptr = (char *)(fibh.ebh->b_data + poffset - lfi);
			} else {
				nameptr = fname;
				memcpy(nameptr, fi->fileIdent + liu,
				       lfi - poffset);
				memcpy(nameptr + lfi - poffset,
				       fibh.ebh->b_data, poffset);
			}
		}

		if ((cfi.fileCharacteristics & FID_FILE_CHAR_DELETED) != 0) {
			if (!UDF_QUERY_FLAG(dir->i_sb, UDF_FLAG_UNDELETE))
				continue;
		}

		if ((cfi.fileCharacteristics & FID_FILE_CHAR_HIDDEN) != 0) {
			if (!UDF_QUERY_FLAG(dir->i_sb, UDF_FLAG_UNHIDE))
				continue;
		}

		if (cfi.fileCharacteristics & FID_FILE_CHAR_PARENT) {
			if (!dir_emit_dotdot(file, ctx))
				goto out;
			continue;
		}

		flen = udf_get_filename(dir->i_sb, nameptr, fname, lfi);
		if (!flen)
			continue;

		tloc = lelb_to_cpu(cfi.icb.extLocation);
		iblock = udf_get_lb_pblock(dir->i_sb, &tloc, 0);
		if (!dir_emit(ctx, fname, flen, iblock, DT_UNKNOWN))
			goto out;
	} /* end while */

	ctx->pos = (nf_pos >> 2) + 1;

out:
	if (fibh.sbh != fibh.ebh)
		brelse(fibh.ebh);
	brelse(fibh.sbh);
	brelse(epos.bh);
	kfree(fname);

	return ret;
}