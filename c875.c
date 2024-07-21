_vte_table_extract_numbers(GValueArray **array,
			   struct _vte_table_arginfo *arginfo, long increment)
{
	GValue value = {0,};
	gssize i;

	g_value_init(&value, G_TYPE_LONG);
	i = 0;
	do {
		long total = 0;
		for (; i < arginfo->length && arginfo->start[i] != ';'; i++) {
			gint v = g_unichar_digit_value (arginfo->start[i]);
			total *= 10;
			total += v == -1 ?  0 : v;
		}
		if (G_UNLIKELY (*array == NULL)) {
			*array = g_value_array_new(1);
		}
		g_value_set_long(&value, total);
		g_value_array_append(*array, &value);
	} while (i++ < arginfo->length);
	g_value_unset(&value);
}