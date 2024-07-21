static ssize_t print_cpu_modalias(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	ssize_t n;
	u32 i;

	n = sprintf(buf, "cpu:type:" CPU_FEATURE_TYPEFMT ":feature:",
		    CPU_FEATURE_TYPEVAL);

	for (i = 0; i < MAX_CPU_FEATURES; i++)
		if (cpu_have_feature(i)) {
			if (PAGE_SIZE < n + sizeof(",XXXX\n")) {
				WARN(1, "CPU features overflow page\n");
				break;
			}
			n += sprintf(&buf[n], ",%04X", i);
		}
	buf[n++] = '\n';
	return n;
}