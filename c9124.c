TEST_F(Http1ServerConnectionImplTest, LargeTrailersRejectedIgnored) {
  // Default limit of 60 KiB
  std::string long_string = "big: " + std::string(60 * 1024, 'q') + "\r\n";
  testTrailersExceedLimit(long_string, false);
}