int mnt_fs_is_deleted(struct libmnt_fs *fs)
{
	return mnt_fs_get_flags(fs) & MNT_FS_DELETED;
}