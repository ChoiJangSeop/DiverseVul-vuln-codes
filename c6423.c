static bool sanity_check_inode(struct inode *inode)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);

	if (f2fs_sb_has_flexible_inline_xattr(sbi->sb)
			&& !f2fs_has_extra_attr(inode)) {
		set_sbi_flag(sbi, SBI_NEED_FSCK);
		f2fs_msg(sbi->sb, KERN_WARNING,
			"%s: corrupted inode ino=%lx, run fsck to fix.",
			__func__, inode->i_ino);
		return false;
	}

	if (f2fs_has_extra_attr(inode) &&
			!f2fs_sb_has_extra_attr(sbi->sb)) {
		set_sbi_flag(sbi, SBI_NEED_FSCK);
		f2fs_msg(sbi->sb, KERN_WARNING,
			"%s: inode (ino=%lx) is with extra_attr, "
			"but extra_attr feature is off",
			__func__, inode->i_ino);
		return false;
	}
	return true;
}