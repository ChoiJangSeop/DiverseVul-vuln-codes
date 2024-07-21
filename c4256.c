TORRENT_TEST(gzip)
{
	std::vector<char> zipped;
	error_code ec;
	load_file(combine_path("..", "zeroes.gz"), zipped, ec, 1000000);
	if (ec) fprintf(stderr, "failed to open file: (%d) %s\n", ec.value()
		, ec.message().c_str());
	TEST_CHECK(!ec);

	std::vector<char> inflated;
	inflate_gzip(&zipped[0], zipped.size(), inflated, 1000000, ec);

	if (ec) {
		fprintf(stderr, "failed to unzip: %s\n", ec.message().c_str());
	}
	TEST_CHECK(!ec);
	TEST_CHECK(inflated.size() > 0);
	for (int i = 0; i < int(inflated.size()); ++i)
		TEST_EQUAL(inflated[i], 0);
}