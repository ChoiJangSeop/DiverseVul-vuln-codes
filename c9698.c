TEST_F(ServerSelectorTestFixture, ShouldFilterByLastWriteTime) {
    TopologyStateMachine stateMachine(sdamConfiguration);
    auto topologyDescription = std::make_shared<TopologyDescription>(sdamConfiguration);

    const int MAX_STALENESS = 90;
    const auto ninetySeconds = Seconds(MAX_STALENESS);
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
                        .withLastUpdateTime(now)
                        .withLastWriteDate(d0)
                        .instance();
    stateMachine.onServerDescription(*topologyDescription, s0);

    const auto d1 = now - Milliseconds(1000 * 5);
    const auto s1 = ServerDescriptionBuilder()
                        .withAddress(HostAndPort("s1"))
                        .withType(ServerType::kRSSecondary)
                        .withRtt(sdamConfiguration.getLocalThreshold())
                        .withSetName("set")
                        .withMinWireVersion(WireVersion::SUPPORTS_OP_MSG)
                        .withMaxWireVersion(WireVersion::LATEST_WIRE_VERSION)
                        .withLastUpdateTime(now)
                        .withLastWriteDate(d1)
                        .instance();
    stateMachine.onServerDescription(*topologyDescription, s1);

    // d2 is stale, so s2 should not be selected.
    const auto d2 = now - ninetySeconds - ninetySeconds;
    const auto s2 = ServerDescriptionBuilder()
                        .withAddress(HostAndPort("s2"))
                        .withType(ServerType::kRSSecondary)
                        .withRtt(sdamConfiguration.getLocalThreshold())
                        .withSetName("set")
                        .withMinWireVersion(WireVersion::SUPPORTS_OP_MSG)
                        .withMaxWireVersion(WireVersion::LATEST_WIRE_VERSION)
                        .withLastUpdateTime(now)
                        .withLastWriteDate(d2)
                        .instance();
    stateMachine.onServerDescription(*topologyDescription, s2);

    const auto readPref =
        ReadPreferenceSetting(ReadPreference::Nearest, TagSets::emptySet, ninetySeconds);

    std::map<HostAndPort, int> frequencyInfo{
        {HostAndPort("s0"), 0}, {HostAndPort("s1"), 0}, {HostAndPort("s2"), 0}};
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        auto server = selector.selectServer(topologyDescription, readPref);

        if (server) {
            frequencyInfo[(*server)->getAddress()]++;
        }
    }

    ASSERT(frequencyInfo[HostAndPort("s0")]);
    ASSERT(frequencyInfo[HostAndPort("s1")]);
    ASSERT_FALSE(frequencyInfo[HostAndPort("s2")]);
}