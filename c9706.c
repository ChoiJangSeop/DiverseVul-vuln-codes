TEST_F(ServerSelectorTestFixture, ShouldThrowOnWireError) {
    auto topologyDescription = std::make_shared<TopologyDescription>(sdamConfiguration);
    auto oldServer = ServerDescriptionBuilder()
                         .withAddress(topologyDescription->getServers().back()->getAddress())
                         .withType(ServerType::kRSPrimary)
                         .withMaxWireVersion(WireVersion::RELEASE_2_4_AND_BEFORE)
                         .withMinWireVersion(WireVersion::RELEASE_2_4_AND_BEFORE)
                         .instance();
    topologyDescription->installServerDescription(oldServer);

    ASSERT(!topologyDescription->isWireVersionCompatible());
    ASSERT_THROWS_CODE(selector.selectServers(topologyDescription, ReadPreferenceSetting()),
                       DBException,
                       ErrorCodes::IncompatibleServerVersion);
}