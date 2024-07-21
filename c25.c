put_dirent32 (struct dirent *d, struct compat_dirent __user *d32)
{
        if (!access_ok(VERIFY_WRITE, d32, sizeof(struct compat_dirent)))
                return -EFAULT;

        __put_user(d->d_ino, &d32->d_ino);
        __put_user(d->d_off, &d32->d_off);
        __put_user(d->d_reclen, &d32->d_reclen);
        if (__copy_to_user(d32->d_name, d->d_name, d->d_reclen))
		return -EFAULT;

        return 0;
}