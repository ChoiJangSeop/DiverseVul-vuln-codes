static ssize_t firmware_loading_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct fw_sysfs *fw_sysfs = to_fw_sysfs(dev);
	int loading = 0;

	mutex_lock(&fw_lock);
	if (fw_sysfs->fw_priv)
		loading = fw_sysfs_loading(fw_sysfs->fw_priv);
	mutex_unlock(&fw_lock);

	return sprintf(buf, "%d\n", loading);
}