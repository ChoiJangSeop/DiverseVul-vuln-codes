TEST_F(Http1ServerConnectionImplTest, ManyTrailersIgnored) {
  // Send a request with 101 headers.
  testTrailersExceedLimit(createHeaderFragment(101), false);
}