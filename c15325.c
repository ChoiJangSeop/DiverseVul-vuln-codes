static ssize_t ib_uverbs_write(struct file *filp, const char __user *buf,
			     size_t count, loff_t *pos)
{
	struct ib_uverbs_file *file = filp->private_data;
	struct ib_device *ib_dev;
	struct ib_uverbs_cmd_hdr hdr;
	__u32 command;
	__u32 flags;
	int srcu_key;
	ssize_t ret;

	if (count < sizeof hdr)
		return -EINVAL;

	if (copy_from_user(&hdr, buf, sizeof hdr))
		return -EFAULT;

	srcu_key = srcu_read_lock(&file->device->disassociate_srcu);
	ib_dev = srcu_dereference(file->device->ib_dev,
				  &file->device->disassociate_srcu);
	if (!ib_dev) {
		ret = -EIO;
		goto out;
	}

	if (hdr.command & ~(__u32)(IB_USER_VERBS_CMD_FLAGS_MASK |
				   IB_USER_VERBS_CMD_COMMAND_MASK)) {
		ret = -EINVAL;
		goto out;
	}

	command = hdr.command & IB_USER_VERBS_CMD_COMMAND_MASK;
	if (verify_command_mask(ib_dev, command)) {
		ret = -EOPNOTSUPP;
		goto out;
	}

	if (!file->ucontext &&
	    command != IB_USER_VERBS_CMD_GET_CONTEXT) {
		ret = -EINVAL;
		goto out;
	}

	flags = (hdr.command &
		 IB_USER_VERBS_CMD_FLAGS_MASK) >> IB_USER_VERBS_CMD_FLAGS_SHIFT;

	if (!flags) {
		if (command >= ARRAY_SIZE(uverbs_cmd_table) ||
		    !uverbs_cmd_table[command]) {
			ret = -EINVAL;
			goto out;
		}

		if (hdr.in_words * 4 != count) {
			ret = -EINVAL;
			goto out;
		}

		ret = uverbs_cmd_table[command](file, ib_dev,
						 buf + sizeof(hdr),
						 hdr.in_words * 4,
						 hdr.out_words * 4);

	} else if (flags == IB_USER_VERBS_CMD_FLAG_EXTENDED) {
		struct ib_uverbs_ex_cmd_hdr ex_hdr;
		struct ib_udata ucore;
		struct ib_udata uhw;
		size_t written_count = count;

		if (command >= ARRAY_SIZE(uverbs_ex_cmd_table) ||
		    !uverbs_ex_cmd_table[command]) {
			ret = -ENOSYS;
			goto out;
		}

		if (!file->ucontext) {
			ret = -EINVAL;
			goto out;
		}

		if (count < (sizeof(hdr) + sizeof(ex_hdr))) {
			ret = -EINVAL;
			goto out;
		}

		if (copy_from_user(&ex_hdr, buf + sizeof(hdr), sizeof(ex_hdr))) {
			ret = -EFAULT;
			goto out;
		}

		count -= sizeof(hdr) + sizeof(ex_hdr);
		buf += sizeof(hdr) + sizeof(ex_hdr);

		if ((hdr.in_words + ex_hdr.provider_in_words) * 8 != count) {
			ret = -EINVAL;
			goto out;
		}

		if (ex_hdr.cmd_hdr_reserved) {
			ret = -EINVAL;
			goto out;
		}

		if (ex_hdr.response) {
			if (!hdr.out_words && !ex_hdr.provider_out_words) {
				ret = -EINVAL;
				goto out;
			}

			if (!access_ok(VERIFY_WRITE,
				       (void __user *) (unsigned long) ex_hdr.response,
				       (hdr.out_words + ex_hdr.provider_out_words) * 8)) {
				ret = -EFAULT;
				goto out;
			}
		} else {
			if (hdr.out_words || ex_hdr.provider_out_words) {
				ret = -EINVAL;
				goto out;
			}
		}

		INIT_UDATA_BUF_OR_NULL(&ucore, buf, (unsigned long) ex_hdr.response,
				       hdr.in_words * 8, hdr.out_words * 8);

		INIT_UDATA_BUF_OR_NULL(&uhw,
				       buf + ucore.inlen,
				       (unsigned long) ex_hdr.response + ucore.outlen,
				       ex_hdr.provider_in_words * 8,
				       ex_hdr.provider_out_words * 8);

		ret = uverbs_ex_cmd_table[command](file,
						   ib_dev,
						   &ucore,
						   &uhw);
		if (!ret)
			ret = written_count;
	} else {
		ret = -ENOSYS;
	}

out:
	srcu_read_unlock(&file->device->disassociate_srcu, srcu_key);
	return ret;
}