TEST_F(ConnectionHandlerTest, FallbackToWildcardListener) {
  TestListener* test_listener1 = addListener(1, true, true, "test_listener1");
  Network::MockListener* listener1 = new Network::MockListener();
  Network::ListenerCallbacks* listener_callbacks1;
  EXPECT_CALL(dispatcher_, createListener_(_, _, _))
      .WillOnce(
          Invoke([&](Network::Socket&, Network::ListenerCallbacks& cb, bool) -> Network::Listener* {
            listener_callbacks1 = &cb;
            return listener1;
          }));
  Network::Address::InstanceConstSharedPtr normal_address(
      new Network::Address::Ipv4Instance("127.0.0.1", 10001));
  EXPECT_CALL(test_listener1->socket_, localAddress()).WillRepeatedly(ReturnRef(normal_address));
  handler_->addListener(*test_listener1);

  TestListener* test_listener2 = addListener(1, false, false, "test_listener2");
  Network::MockListener* listener2 = new Network::MockListener();
  Network::ListenerCallbacks* listener_callbacks2;
  EXPECT_CALL(dispatcher_, createListener_(_, _, _))
      .WillOnce(
          Invoke([&](Network::Socket&, Network::ListenerCallbacks& cb, bool) -> Network::Listener* {
            listener_callbacks2 = &cb;
            return listener2;
          }));
  Network::Address::InstanceConstSharedPtr any_address = Network::Utility::getIpv4AnyAddress();
  EXPECT_CALL(test_listener2->socket_, localAddress()).WillRepeatedly(ReturnRef(any_address));
  handler_->addListener(*test_listener2);

  Network::MockListenerFilter* test_filter = new Network::MockListenerFilter();
  Network::MockConnectionSocket* accepted_socket = new NiceMock<Network::MockConnectionSocket>();
  bool redirected = false;
  EXPECT_CALL(factory_, createListenerFilterChain(_))
      .WillRepeatedly(Invoke([&](Network::ListenerFilterManager& manager) -> bool {
        // Insert the Mock filter.
        if (!redirected) {
          manager.addAcceptFilter(Network::ListenerFilterPtr{test_filter});
          redirected = true;
        }
        return true;
      }));
  // Zero port to match the port of AnyAddress
  Network::Address::InstanceConstSharedPtr alt_address(
      new Network::Address::Ipv4Instance("127.0.0.2", 0));
  EXPECT_CALL(*test_filter, onAccept(_))
      .WillOnce(Invoke([&](Network::ListenerFilterCallbacks& cb) -> Network::FilterStatus {
        cb.socket().restoreLocalAddress(alt_address);
        return Network::FilterStatus::Continue;
      }));
  EXPECT_CALL(*accepted_socket, restoreLocalAddress(alt_address));
  EXPECT_CALL(*accepted_socket, localAddressRestored()).WillOnce(Return(true));
  EXPECT_CALL(*accepted_socket, localAddress()).WillRepeatedly(ReturnRef(alt_address));
  EXPECT_CALL(manager_, findFilterChain(_)).WillOnce(Return(filter_chain_.get()));
  Network::MockConnection* connection = new NiceMock<Network::MockConnection>();
  EXPECT_CALL(dispatcher_, createServerConnection_()).WillOnce(Return(connection));
  EXPECT_CALL(factory_, createNetworkFilterChain(_, _)).WillOnce(Return(true));
  listener_callbacks1->onAccept(Network::ConnectionSocketPtr{accepted_socket});
  EXPECT_EQ(1UL, handler_->numConnections());

  EXPECT_CALL(*listener2, onDestroy());
  EXPECT_CALL(*listener1, onDestroy());
}