static void test_json_append_escaped(void)
{
	string_t *str = t_str_new(32);

	test_begin("json_append_escaped()");
	json_append_escaped(str, "\b\f\r\n\t\"\\\001\002-\xC3\xA4\xf0\x90\x90\xb7");
	test_assert(strcmp(str_c(str), "\\b\\f\\r\\n\\t\\\"\\\\\\u0001\\u0002-\\u00e4\\ud801\\udc37") == 0);
	test_end();
}