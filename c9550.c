static void k_fn(struct vc_data *vc, unsigned char value, char up_flag)
{
	if (up_flag)
		return;

	if ((unsigned)value < ARRAY_SIZE(func_table)) {
		if (func_table[value])
			puts_queue(vc, func_table[value]);
	} else
		pr_err("k_fn called with value=%d\n", value);
}