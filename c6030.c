bool f2fs_init_extent_tree(struct inode *inode, struct f2fs_extent *i_ext)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct extent_tree *et;
	struct extent_node *en;
	struct extent_info ei;

	if (!f2fs_may_extent_tree(inode)) {
		/* drop largest extent */
		if (i_ext && i_ext->len) {
			i_ext->len = 0;
			return true;
		}
		return false;
	}

	et = __grab_extent_tree(inode);

	if (!i_ext || !i_ext->len)
		return false;

	get_extent_info(&ei, i_ext);

	write_lock(&et->lock);
	if (atomic_read(&et->node_cnt))
		goto out;

	en = __init_extent_tree(sbi, et, &ei);
	if (en) {
		spin_lock(&sbi->extent_lock);
		list_add_tail(&en->list, &sbi->extent_list);
		spin_unlock(&sbi->extent_lock);
	}
out:
	write_unlock(&et->lock);
	return false;
}