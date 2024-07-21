TEST_F(HttpConnectionManagerImplTest, OverlyLongHeadersRejected) {
  setup(false, "");

  std::string response_code;
  std::string response_body;
  EXPECT_CALL(*codec_, dispatch(_)).WillOnce(Invoke([&](Buffer::Instance&) -> void {
    StreamDecoder* decoder = &conn_manager_->newStream(response_encoder_);
    HeaderMapPtr headers{
        new TestHeaderMapImpl{{":authority", "host"}, {":path", "/"}, {":method", "GET"}}};
    headers->addCopy(LowerCaseString("Foo"), std::string(60 * 1024, 'a'));

    EXPECT_CALL(response_encoder_, encodeHeaders(_, true))
        .WillOnce(Invoke([&response_code](const HeaderMap& headers, bool) -> void {
          response_code = std::string(headers.Status()->value().getStringView());
        }));
    decoder->decodeHeaders(std::move(headers), true);
    conn_manager_->newStream(response_encoder_);
  }));

  Buffer::OwnedImpl fake_input("1234");
  conn_manager_->onData(fake_input, false); // kick off request

  EXPECT_EQ("431", response_code);
  EXPECT_EQ("", response_body);
}