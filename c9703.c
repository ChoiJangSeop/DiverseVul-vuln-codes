bool TopologyManager::onServerDescription(const IsMasterOutcome& isMasterOutcome) {
    stdx::lock_guard<mongo::Mutex> lock(_mutex);

    boost::optional<IsMasterRTT> lastRTT;
    boost::optional<TopologyVersion> lastTopologyVersion;

    const auto& lastServerDescription =
        _topologyDescription->findServerByAddress(isMasterOutcome.getServer());
    if (lastServerDescription) {
        lastRTT = (*lastServerDescription)->getRtt();
        lastTopologyVersion = (*lastServerDescription)->getTopologyVersion();
    }

    boost::optional<TopologyVersion> newTopologyVersion = isMasterOutcome.getTopologyVersion();
    if (isStaleTopologyVersion(lastTopologyVersion, newTopologyVersion)) {
        LOGV2(
            23930,
            "Ignoring this isMaster response because our topologyVersion: {lastTopologyVersion} is "
            "fresher than the provided topologyVersion: {newTopologyVersion}",
            "Ignoring this isMaster response because our last topologyVersion is fresher than the "
            "new topologyVersion provided",
            "lastTopologyVersion"_attr = lastTopologyVersion->toBSON(),
            "newTopologyVersion"_attr = newTopologyVersion->toBSON());
        return false;
    }

    auto newServerDescription = std::make_shared<ServerDescription>(
        _clockSource, isMasterOutcome, lastRTT, newTopologyVersion);

    auto oldTopologyDescription = _topologyDescription;
    _topologyDescription = std::make_shared<TopologyDescription>(*oldTopologyDescription);

    // if we are equal to the old description, just install the new description without
    // performing any actions on the state machine.
    auto isEqualToOldServerDescription =
        (lastServerDescription && (*lastServerDescription->get()) == *newServerDescription);
    if (isEqualToOldServerDescription) {
        _topologyDescription->installServerDescription(newServerDescription);
    } else {
        _topologyStateMachine->onServerDescription(*_topologyDescription, newServerDescription);
    }

    _publishTopologyDescriptionChanged(oldTopologyDescription, _topologyDescription);
    return true;
}