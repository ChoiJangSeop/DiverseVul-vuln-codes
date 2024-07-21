TEST_F(ConnectionHandlerTest, ListenerFilterTimeoutResetOnSuccess) {
  InSequence s;

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
  Network::ListenerFilterCallbacks* listener_filter_cb{};
  EXPECT_CALL(*test_filter, onAccept(_))
      .WillOnce(Invoke([&](Network::ListenerFilterCallbacks& cb) -> Network::FilterStatus {
        listener_filter_cb = &cb;
        return Network::FilterStatus::StopIteration;
      }));
  Network::MockConnectionSocket* accepted_socket = new NiceMock<Network::MockConnectionSocket>();
  Network::IoSocketHandleImpl io_handle{42};
  EXPECT_CALL(*accepted_socket, ioHandle()).WillRepeatedly(ReturnRef(io_handle));

  Event::MockTimer* timeout = new Event::MockTimer(&dispatcher_);
  EXPECT_CALL(*timeout, enableTimer(std::chrono::milliseconds(15000), _));
  listener_callbacks->onAccept(Network::ConnectionSocketPtr{accepted_socket});

  EXPECT_CALL(manager_, findFilterChain(_)).WillOnce(Return(nullptr));
  EXPECT_CALL(*timeout, disableTimer());
  listener_filter_cb->continueFilterChain(true);

  EXPECT_CALL(*listener, onDestroy());
}