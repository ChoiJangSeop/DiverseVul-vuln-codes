TEST(Url, ParsingFails) {
  Utility::Url url;
  const bool is_connect = true;
  EXPECT_FALSE(url.initialize("", !is_connect));
  EXPECT_FALSE(url.initialize("foo", !is_connect));
  EXPECT_FALSE(url.initialize("http://", !is_connect));
  EXPECT_FALSE(url.initialize("random_scheme://host.com/path", !is_connect));
  // Only port value in valid range (1-65535) is allowed.
  EXPECT_FALSE(url.initialize("http://host.com:65536/path", !is_connect));
  EXPECT_FALSE(url.initialize("http://host.com:0/path", !is_connect));
  EXPECT_FALSE(url.initialize("http://host.com:-1/path", !is_connect));
  EXPECT_FALSE(url.initialize("http://host.com:port/path", !is_connect));

  // Test parsing fails for CONNECT request URLs.
  EXPECT_FALSE(url.initialize("http://www.foo.com", is_connect));
  EXPECT_FALSE(url.initialize("foo.com", is_connect));
  // Only port value in valid range (1-65535) is allowed.
  EXPECT_FALSE(url.initialize("foo.com:65536", is_connect));
  EXPECT_FALSE(url.initialize("foo.com:0", is_connect));
  EXPECT_FALSE(url.initialize("foo.com:-1", is_connect));
  EXPECT_FALSE(url.initialize("foo.com:port", is_connect));
}