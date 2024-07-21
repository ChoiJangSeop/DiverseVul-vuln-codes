TEST_F(ServerSelectorTestFixture, ShouldFilterCorrectlyByLatencyWindow) {
    const auto delta = Milliseconds(10);
    const auto windowWidth = Milliseconds(100);
    const auto lowerBound = Milliseconds(100);

    auto window = LatencyWindow(lowerBound, windowWidth);

    std::vector<ServerDescriptionPtr> servers = {
        make_with_latency(window.lower - delta, HostAndPort("less")),
        make_with_latency(window.lower, HostAndPort("boundary-lower")),
        make_with_latency(window.lower + delta, HostAndPort("within")),
        make_with_latency(window.upper, HostAndPort("boundary-upper")),
        make_with_latency(window.upper + delta, HostAndPort("greater"))};

    window.filterServers(&servers);

    ASSERT_EQ(3, servers.size());
    ASSERT_EQ(HostAndPort("boundary-lower"), servers[0]->getAddress());
    ASSERT_EQ(HostAndPort("within"), servers[1]->getAddress());
    ASSERT_EQ(HostAndPort("boundary-upper"), servers[2]->getAddress());
}