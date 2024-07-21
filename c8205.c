TEST_F(ConnectionHandlerTest, TransportProtocolCustom) {
  TestListener* test_listener = addListener(1, true, false, "test_listener");
  Network::MockListener* listener = new Network::MockListener();
  Network::ListenerCallbacks* listener_callbacks;
  EXPECT_CALL(dispatcher_, createListener_(_, _, _))
      .WillOnce(
          Invoke([&](Network::Socket&, Network::ListenerCallbacks& cb, bool) -> Network::Listener* {
            listener_callbacks = &cb;
            return listener;
          }));
  EXPECT_CALL(test_listener->socket_, localAddress());
  handler_->addListener(*test_listener);

  Network::MockListenerFilter* test_filter = new Network::MockListenerFilter();
  EXPECT_CALL(factory_, createListenerFilterChain(_))
      .WillRepeatedly(Invoke([&](Network::ListenerFilterManager& manager) -> bool {
        manager.addAcceptFilter(Network::ListenerFilterPtr{test_filter});
        return true;
      }));
  absl::string_view dummy = "dummy";
  EXPECT_CALL(*test_filter, onAccept(_))
      .WillOnce(Invoke([&](Network::ListenerFilterCallbacks& cb) -> Network::FilterStatus {
        cb.socket().setDetectedTransportProtocol(dummy);
        return Network::FilterStatus::Continue;
      }));
  Network::MockConnectionSocket* accepted_socket = new NiceMock<Network::MockConnectionSocket>();
  EXPECT_CALL(*accepted_socket, setDetectedTransportProtocol(dummy));
  EXPECT_CALL(*accepted_socket, detectedTransportProtocol()).WillOnce(Return(dummy));
  EXPECT_CALL(manager_, findFilterChain(_)).WillOnce(Return(nullptr));
  listener_callbacks->onAccept(Network::ConnectionSocketPtr{accepted_socket});

  EXPECT_CALL(*listener, onDestroy());
}