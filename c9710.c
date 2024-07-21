boost::optional<std::vector<ServerDescriptionPtr>> SdamServerSelector::selectServers(
    const TopologyDescriptionPtr topologyDescription, const ReadPreferenceSetting& criteria) {

    // If the topology wire version is invalid, raise an error
    if (!topologyDescription->isWireVersionCompatible()) {
        uasserted(ErrorCodes::IncompatibleServerVersion,
                  *topologyDescription->getWireVersionCompatibleError());
    }

    if (topologyDescription->getType() == TopologyType::kUnknown) {
        return boost::none;
    }

    if (topologyDescription->getType() == TopologyType::kSingle) {
        auto servers = topologyDescription->getServers();
        return (servers.size() && servers[0]->getType() != ServerType::kUnknown)
            ? boost::optional<std::vector<ServerDescriptionPtr>>{{servers[0]}}
            : boost::none;
    }

    std::vector<ServerDescriptionPtr> results;
    _getCandidateServers(&results, topologyDescription, criteria);

    if (results.size()) {
        if (MONGO_unlikely(sdamServerSelectorIgnoreLatencyWindow.shouldFail())) {
            return results;
        }

        ServerDescriptionPtr minServer =
            *std::min_element(results.begin(), results.end(), LatencyWindow::rttCompareFn);

        invariant(minServer->getRtt());
        auto latencyWindow = LatencyWindow(*minServer->getRtt(), _config.getLocalThreshold());
        latencyWindow.filterServers(&results);

        // latency window should always leave at least one result
        invariant(results.size());
        std::shuffle(std::begin(results), std::end(results), _random.urbg());
        return results;
    }

    return boost::none;
}