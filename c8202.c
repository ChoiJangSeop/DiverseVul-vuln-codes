TEST_F(ConnectionHandlerTest, ListenerFilterTimeout) {
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
  EXPECT_CALL(*test_filter, onAccept(_))
      .WillOnce(Invoke([&](Network::ListenerFilterCallbacks&) -> Network::FilterStatus {
        return Network::FilterStatus::StopIteration;
      }));
  Network::MockConnectionSocket* accepted_socket = new NiceMock<Network::MockConnectionSocket>();
  Network::IoSocketHandleImpl io_handle{42};
  EXPECT_CALL(*accepted_socket, ioHandle()).WillRepeatedly(ReturnRef(io_handle));
  Event::MockTimer* timeout = new Event::MockTimer(&dispatcher_);
  EXPECT_CALL(*timeout, enableTimer(std::chrono::milliseconds(15000), _));
  listener_callbacks->onAccept(Network::ConnectionSocketPtr{accepted_socket});
  Stats::Gauge& downstream_pre_cx_active =
      stats_store_.gauge("downstream_pre_cx_active", Stats::Gauge::ImportMode::Accumulate);
  EXPECT_EQ(1UL, downstream_pre_cx_active.value());

  EXPECT_CALL(*timeout, disableTimer());
  timeout->invokeCallback();
  dispatcher_.clearDeferredDeleteList();
  EXPECT_EQ(0UL, downstream_pre_cx_active.value());
  EXPECT_EQ(1UL, stats_store_.counter("downstream_pre_cx_timeout").value());

  // Make sure we didn't continue to try create connection.
  EXPECT_EQ(0UL, stats_store_.counter("no_filter_chain_match").value());

  EXPECT_CALL(*listener, onDestroy());
}