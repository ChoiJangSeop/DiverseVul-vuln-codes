void validateConnectUrl(absl::string_view raw_url, absl::string_view expected_host_port,
                        uint16_t expected_port) {
  Utility::Url url;
  ASSERT_TRUE(url.initialize(raw_url, /*is_connect=*/true)) << "Failed to initialize " << raw_url;
  EXPECT_TRUE(url.scheme().empty());
  EXPECT_TRUE(url.pathAndQueryParams().empty());
  EXPECT_EQ(url.hostAndPort(), expected_host_port);
  EXPECT_EQ(url.port(), expected_port);
}