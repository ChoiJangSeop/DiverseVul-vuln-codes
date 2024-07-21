TEST_F(Http1ServerConnectionImplTest, ManyTrailersRejected) {
  // Send a request with 101 headers.
  testTrailersExceedLimit(createHeaderFragment(101), true);
}