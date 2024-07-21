TEST_F(ServerSelectorTestFixture, ShouldReturnNoneIfTopologyUnknown) {
    auto topologyDescription = std::make_shared<TopologyDescription>(sdamConfiguration);
    ASSERT_EQ(TopologyType::kUnknown, topologyDescription->getType());
    ASSERT_EQ(boost::none, selector.selectServers(topologyDescription, ReadPreferenceSetting()));
}