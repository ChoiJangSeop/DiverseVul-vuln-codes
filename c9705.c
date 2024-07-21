void TopologyManager::onServerRTTUpdated(HostAndPort hostAndPort, IsMasterRTT rtt) {
    {
        stdx::lock_guard<mongo::Mutex> lock(_mutex);

        auto oldServerDescription = _topologyDescription->findServerByAddress(hostAndPort);
        if (oldServerDescription) {
            auto newServerDescription = (*oldServerDescription)->cloneWithRTT(rtt);

            auto oldTopologyDescription = _topologyDescription;
            _topologyDescription = std::make_shared<TopologyDescription>(*_topologyDescription);
            _topologyDescription->installServerDescription(newServerDescription);

            _publishTopologyDescriptionChanged(oldTopologyDescription, _topologyDescription);

            return;
        }
    }
    // otherwise, the server was removed from the topology. Nothing to do.
    LOGV2(4333201,
          "Not updating RTT. Server {server} does not exist in {replicaSet}",
          "Not updating RTT. The server does not exist in the replica set",
          "server"_attr = hostAndPort,
          "replicaSet"_attr = getTopologyDescription()->getSetName());
}