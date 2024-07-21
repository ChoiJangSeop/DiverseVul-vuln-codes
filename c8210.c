TEST_F(ConnectionHandlerTest, WildcardListenerWithNoOriginalDst) {
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
      new Network::Address::Ipv4Instance("127.0.0.1", 80));
  Network::Address::InstanceConstSharedPtr any_address = Network::Utility::getAddressWithPort(
      *Network::Utility::getIpv4AnyAddress(), normal_address->ip()->port());
  EXPECT_CALL(test_listener1->socket_, localAddress()).WillRepeatedly(ReturnRef(any_address));
  handler_->addListener(*test_listener1);

  Network::MockListenerFilter* test_filter = new Network::MockListenerFilter();
  Network::MockConnectionSocket* accepted_socket = new NiceMock<Network::MockConnectionSocket>();
  EXPECT_CALL(factory_, createListenerFilterChain(_))
      .WillRepeatedly(Invoke([&](Network::ListenerFilterManager& manager) -> bool {
        // Insert the Mock filter.
        manager.addAcceptFilter(Network::ListenerFilterPtr{test_filter});
        return true;
      }));
  EXPECT_CALL(*test_filter, onAccept(_)).WillOnce(Return(Network::FilterStatus::Continue));
  EXPECT_CALL(*accepted_socket, localAddressRestored()).WillOnce(Return(false));
  EXPECT_CALL(*accepted_socket, localAddress()).WillRepeatedly(ReturnRef(normal_address));
  EXPECT_CALL(manager_, findFilterChain(_)).WillOnce(Return(filter_chain_.get()));
  Network::MockConnection* connection = new NiceMock<Network::MockConnection>();
  EXPECT_CALL(dispatcher_, createServerConnection_()).WillOnce(Return(connection));
  EXPECT_CALL(factory_, createNetworkFilterChain(_, _)).WillOnce(Return(true));
  listener_callbacks1->onAccept(Network::ConnectionSocketPtr{accepted_socket});
  EXPECT_EQ(1UL, handler_->numConnections());

  EXPECT_CALL(*listener1, onDestroy());
}