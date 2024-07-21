int main(void)
{
	static void (*const test_functions[])(void) = {
		test_message_parser_small_blocks,
		test_message_parser_stop_early,
		test_message_parser_truncated_mime_headers,
		test_message_parser_truncated_mime_headers2,
		test_message_parser_truncated_mime_headers3,
		test_message_parser_empty_multipart,
		test_message_parser_duplicate_mime_boundary,
		test_message_parser_garbage_suffix_mime_boundary,
		test_message_parser_trailing_dashes,
		test_message_parser_continuing_mime_boundary,
		test_message_parser_continuing_truncated_mime_boundary,
		test_message_parser_continuing_mime_boundary_reverse,
		test_message_parser_long_mime_boundary,
		test_message_parser_no_eoh,
		test_message_parser_mime_part_nested_limit,
		test_message_parser_mime_part_nested_limit_rfc822,
		test_message_parser_mime_part_limit,
		test_message_parser_mime_version,
		test_message_parser_mime_version_missing,
		NULL
	};
	return test_run(test_functions);
}