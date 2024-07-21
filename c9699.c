TEST_F(ServerSelectorTestFixture, ShouldSelectPreferredIfAvailable) {
    TopologyStateMachine stateMachine(sdamConfiguration);
    auto topologyDescription = std::make_shared<TopologyDescription>(sdamConfiguration);

    const auto now = Date_t::now();


    const auto d0 = now - Milliseconds(1000);
    const auto s0 = ServerDescriptionBuilder()
                        .withAddress(HostAndPort("s0"))
                        .withType(ServerType::kRSPrimary)
                        .withRtt(sdamConfiguration.getLocalThreshold())
                        .withSetName("set")
                        .withHost(HostAndPort("s0"))
                        .withHost(HostAndPort("s1"))
                        .withMinWireVersion(WireVersion::SUPPORTS_OP_MSG)
                        .withMaxWireVersion(WireVersion::LATEST_WIRE_VERSION)
                        .withLastWriteDate(d0)
                        .withTag("tag", "primary")
                        .instance();
    stateMachine.onServerDescription(*topologyDescription, s0);

    const auto s1 = ServerDescriptionBuilder()
                        .withAddress(HostAndPort("s1"))
                        .withType(ServerType::kRSSecondary)
                        .withRtt(sdamConfiguration.getLocalThreshold())
                        .withSetName("set")
                        .withHost(HostAndPort("s0"))
                        .withHost(HostAndPort("s1"))
                        .withMinWireVersion(WireVersion::SUPPORTS_OP_MSG)
                        .withMaxWireVersion(WireVersion::LATEST_WIRE_VERSION)
                        .withLastWriteDate(d0)
                        .withTag("tag", "secondary")
                        .instance();
    stateMachine.onServerDescription(*topologyDescription, s1);

    const auto primaryPreferredTagSecondary =
        ReadPreferenceSetting(ReadPreference::PrimaryPreferred, TagSets::secondarySet);
    auto result1 = selector.selectServer(topologyDescription, primaryPreferredTagSecondary);
    ASSERT(result1 != boost::none);
    ASSERT_EQ(HostAndPort("s0"), (*result1)->getAddress());

    const auto secondaryPreferredWithTag =
        ReadPreferenceSetting(ReadPreference::SecondaryPreferred, TagSets::secondarySet);
    auto result2 = selector.selectServer(topologyDescription, secondaryPreferredWithTag);
    ASSERT(result2 != boost::none);
    ASSERT_EQ(HostAndPort("s1"), (*result2)->getAddress());

    const auto secondaryPreferredNoTag = ReadPreferenceSetting(ReadPreference::SecondaryPreferred);
    auto result3 = selector.selectServer(topologyDescription, secondaryPreferredNoTag);
    ASSERT(result3 != boost::none);
    ASSERT_EQ(HostAndPort("s1"), (*result2)->getAddress());
}