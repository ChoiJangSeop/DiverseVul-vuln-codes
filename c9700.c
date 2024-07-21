TEST_F(ServerSelectorTestFixture, ShouldSelectTaggedSecondaryIfPreferredPrimaryNotAvailable) {
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
                        .withHost(HostAndPort("s2"))
                        .withMinWireVersion(WireVersion::SUPPORTS_OP_MSG)
                        .withMaxWireVersion(WireVersion::LATEST_WIRE_VERSION)
                        .withLastWriteDate(d0)
                        .withTag("tag", "primary")
                        .instance();
    stateMachine.onServerDescription(*topologyDescription, s0);

    // old primary unavailable
    const auto s0_failed = ServerDescriptionBuilder()
                               .withAddress(HostAndPort("s0"))
                               .withType(ServerType::kUnknown)
                               .withSetName("set")
                               .instance();
    stateMachine.onServerDescription(*topologyDescription, s0_failed);

    const auto s1 = ServerDescriptionBuilder()
                        .withAddress(HostAndPort("s1"))
                        .withType(ServerType::kRSSecondary)
                        .withRtt(sdamConfiguration.getLocalThreshold())
                        .withSetName("set")
                        .withHost(HostAndPort("s0"))
                        .withHost(HostAndPort("s1"))
                        .withHost(HostAndPort("s2"))
                        .withMinWireVersion(WireVersion::SUPPORTS_OP_MSG)
                        .withMaxWireVersion(WireVersion::LATEST_WIRE_VERSION)
                        .withLastWriteDate(d0)
                        .withTag("tag", "secondary")
                        .instance();
    stateMachine.onServerDescription(*topologyDescription, s1);

    const auto s2 = ServerDescriptionBuilder()
                        .withAddress(HostAndPort("s2"))
                        .withType(ServerType::kRSSecondary)
                        .withRtt(sdamConfiguration.getLocalThreshold())
                        .withSetName("set")
                        .withHost(HostAndPort("s0"))
                        .withHost(HostAndPort("s1"))
                        .withHost(HostAndPort("s2"))
                        .withMinWireVersion(WireVersion::SUPPORTS_OP_MSG)
                        .withMaxWireVersion(WireVersion::LATEST_WIRE_VERSION)
                        .withLastWriteDate(d0)
                        .instance();
    stateMachine.onServerDescription(*topologyDescription, s2);

    const auto primaryPreferredTagSecondary =
        ReadPreferenceSetting(ReadPreference::PrimaryPreferred, TagSets::secondarySet);
    auto result1 = selector.selectServer(topologyDescription, primaryPreferredTagSecondary);
    ASSERT(result1 != boost::none);
    ASSERT_EQ(HostAndPort("s1"), (*result1)->getAddress());
}