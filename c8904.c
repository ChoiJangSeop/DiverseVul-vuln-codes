compat_mptfwxfer_ioctl(struct file *filp, unsigned int cmd,
			unsigned long arg)
{
	struct mpt_fw_xfer32 kfw32;
	struct mpt_fw_xfer kfw;
	MPT_ADAPTER *iocp = NULL;
	int iocnum, iocnumX;
	int nonblock = (filp->f_flags & O_NONBLOCK);
	int ret;


	if (copy_from_user(&kfw32, (char __user *)arg, sizeof(kfw32)))
		return -EFAULT;

	/* Verify intended MPT adapter */
	iocnumX = kfw32.iocnum & 0xFF;
	if (((iocnum = mpt_verify_adapter(iocnumX, &iocp)) < 0) ||
	    (iocp == NULL)) {
		printk(KERN_DEBUG MYNAM "::compat_mptfwxfer_ioctl @%d - ioc%d not found!\n",
			__LINE__, iocnumX);
		return -ENODEV;
	}

	if ((ret = mptctl_syscall_down(iocp, nonblock)) != 0)
		return ret;

	dctlprintk(iocp, printk(MYIOC_s_DEBUG_FMT "compat_mptfwxfer_ioctl() called\n",
	    iocp->name));
	kfw.iocnum = iocnum;
	kfw.fwlen = kfw32.fwlen;
	kfw.bufp = compat_ptr(kfw32.bufp);

	ret = mptctl_do_fw_download(kfw.iocnum, kfw.bufp, kfw.fwlen);

	mutex_unlock(&iocp->ioctl_cmds.mutex);

	return ret;
}