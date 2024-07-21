boost::optional<ServerDescriptionPtr> TopologyDescription::installServerDescription(
    const ServerDescriptionPtr& newServerDescription) {
    boost::optional<ServerDescriptionPtr> previousDescription;
    if (getType() == TopologyType::kSingle) {
        // For Single, there is always one ServerDescription in TopologyDescription.servers;
        // the ServerDescription in TopologyDescription.servers MUST be replaced with the new
        // ServerDescription if the new topologyVersion is >= the old.
        invariant(_servers.size() == 1);
        previousDescription = _servers[0];
        _servers[0] = std::shared_ptr<ServerDescription>(newServerDescription);
    } else {
        for (auto it = _servers.begin(); it != _servers.end(); ++it) {
            const auto& currentDescription = *it;
            if (currentDescription->getAddress() == newServerDescription->getAddress()) {
                previousDescription = *it;
                *it = std::shared_ptr<ServerDescription>(newServerDescription);
                break;
            }
        }

        if (!previousDescription) {
            _servers.push_back(std::shared_ptr<ServerDescription>(newServerDescription));
        }
    }
    newServerDescription->_topologyDescription = shared_from_this();
    checkWireCompatibilityVersions();
    calculateLogicalSessionTimeout();

    topologyDescriptionInstallServerDescription.shouldFail();
    return previousDescription;
}