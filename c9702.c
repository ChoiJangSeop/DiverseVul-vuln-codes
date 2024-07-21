TopologyManager::TopologyManager(SdamConfiguration config,
                                 ClockSource* clockSource,
                                 TopologyEventsPublisherPtr eventsPublisher)
    : _config(std::move(config)),
      _clockSource(clockSource),
      _topologyDescription(std::make_shared<TopologyDescription>(_config)),
      _topologyStateMachine(std::make_unique<TopologyStateMachine>(_config)),
      _topologyEventsPublisher(eventsPublisher) {}