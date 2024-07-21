static int target_xcopy_locate_se_dev_e4(const unsigned char *dev_wwn,
					struct se_device **found_dev)
{
	struct xcopy_dev_search_info info;
	int ret;

	memset(&info, 0, sizeof(info));
	info.dev_wwn = dev_wwn;

	ret = target_for_each_device(target_xcopy_locate_se_dev_e4_iter, &info);
	if (ret == 1) {
		*found_dev = info.found_dev;
		return 0;
	} else {
		pr_debug_ratelimited("Unable to locate 0xe4 descriptor for EXTENDED_COPY\n");
		return -EINVAL;
	}
}