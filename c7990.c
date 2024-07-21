TEST_F(HttpConnectionManagerImplTest, OverlyLongHeadersAcceptedIfConfigured) {
  max_request_headers_kb_ = 62;
  setup(false, "");

  EXPECT_CALL(*codec_, dispatch(_)).WillOnce(Invoke([&](Buffer::Instance&) -> void {
    StreamDecoder* decoder = &conn_manager_->newStream(response_encoder_);
    HeaderMapPtr headers{
        new TestHeaderMapImpl{{":authority", "host"}, {":path", "/"}, {":method", "GET"}}};
    headers->addCopy(LowerCaseString("Foo"), std::string(60 * 1024, 'a'));

    EXPECT_CALL(response_encoder_, encodeHeaders(_, _)).Times(0);
    decoder->decodeHeaders(std::move(headers), true);
    conn_manager_->newStream(response_encoder_);
  }));

  Buffer::OwnedImpl fake_input("1234");
  conn_manager_->onData(fake_input, false); // kick off request
}