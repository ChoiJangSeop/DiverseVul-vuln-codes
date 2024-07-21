TEST_F(ServerSelectorTestFixture, ShouldSelectRandomlyWhenMultipleOptionsAreAvailable) {
    TopologyStateMachine stateMachine(sdamConfiguration);
    auto topologyDescription = std::make_shared<TopologyDescription>(sdamConfiguration);

    const auto s0Latency = Milliseconds(1);
    auto primary = ServerDescriptionBuilder()
                       .withAddress(HostAndPort("s0"))
                       .withType(ServerType::kRSPrimary)
                       .withLastUpdateTime(Date_t::now())
                       .withLastWriteDate(Date_t::now())
                       .withRtt(s0Latency)
                       .withSetName("set")
                       .withHost(HostAndPort("s0"))
                       .withHost(HostAndPort("s1"))
                       .withHost(HostAndPort("s2"))
                       .withHost(HostAndPort("s3"))
                       .withMinWireVersion(WireVersion::SUPPORTS_OP_MSG)
                       .withMaxWireVersion(WireVersion::LATEST_WIRE_VERSION)
                       .instance();
    stateMachine.onServerDescription(*topologyDescription, primary);

    const auto s1Latency = Milliseconds((s0Latency + sdamConfiguration.getLocalThreshold()) / 2);
    auto secondaryInLatencyWindow =
        make_with_latency(s1Latency, HostAndPort("s1"), ServerType::kRSSecondary);
    stateMachine.onServerDescription(*topologyDescription, secondaryInLatencyWindow);

    // s2 is on the boundary of the latency window
    const auto s2Latency = s0Latency + sdamConfiguration.getLocalThreshold();
    auto secondaryOnBoundaryOfLatencyWindow =
        make_with_latency(s2Latency, HostAndPort("s2"), ServerType::kRSSecondary);
    stateMachine.onServerDescription(*topologyDescription, secondaryOnBoundaryOfLatencyWindow);

    // s3 should not be selected
    const auto s3Latency = s2Latency + Milliseconds(10);
    auto secondaryTooFar =
        make_with_latency(s3Latency, HostAndPort("s3"), ServerType::kRSSecondary);
    stateMachine.onServerDescription(*topologyDescription, secondaryTooFar);

    std::map<HostAndPort, int> frequencyInfo{{HostAndPort("s0"), 0},
                                             {HostAndPort("s1"), 0},
                                             {HostAndPort("s2"), 0},
                                             {HostAndPort("s3"), 0}};
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        auto server = selector.selectServer(topologyDescription,
                                            ReadPreferenceSetting(ReadPreference::Nearest));
        if (server) {
            frequencyInfo[(*server)->getAddress()]++;
        }
    }

    ASSERT(frequencyInfo[HostAndPort("s0")]);
    ASSERT(frequencyInfo[HostAndPort("s1")]);
    ASSERT(frequencyInfo[HostAndPort("s2")]);
    ASSERT_FALSE(frequencyInfo[HostAndPort("s3")]);
}