void SdamServerSelector::_getCandidateServers(std::vector<ServerDescriptionPtr>* result,
                                              const TopologyDescriptionPtr topologyDescription,
                                              const ReadPreferenceSetting& criteria) {
    // when querying the primary we don't need to consider tags
    bool shouldTagFilter = true;

    // TODO SERVER-46499: check to see if we want to enforce minOpTime at all since
    // it was effectively optional in the original implementation.
    if (!criteria.minOpTime.isNull()) {
        auto eligibleServers = topologyDescription->findServers([](const ServerDescriptionPtr& s) {
            return (s->getType() == ServerType::kRSPrimary ||
                    s->getType() == ServerType::kRSSecondary);
        });

        auto beginIt = eligibleServers.begin();
        auto endIt = eligibleServers.end();
        auto maxIt = std::max_element(beginIt,
                                      endIt,
                                      [topologyDescription](const ServerDescriptionPtr& left,
                                                            const ServerDescriptionPtr& right) {
                                          return left->getOpTime() < right->getOpTime();
                                      });
        if (maxIt != endIt) {
            auto maxOpTime = (*maxIt)->getOpTime();
            if (maxOpTime && maxOpTime < criteria.minOpTime) {
                // ignore minOpTime
                const_cast<ReadPreferenceSetting&>(criteria) = ReadPreferenceSetting(criteria.pref);
            }
        }
    }

    switch (criteria.pref) {
        case ReadPreference::Nearest:
            *result = topologyDescription->findServers(nearestFilter(criteria));
            break;

        case ReadPreference::SecondaryOnly:
            *result = topologyDescription->findServers(secondaryFilter(criteria));
            break;

        case ReadPreference::PrimaryOnly: {
            const auto primaryCriteria = ReadPreferenceSetting(criteria.pref);
            *result = topologyDescription->findServers(primaryFilter(primaryCriteria));
            shouldTagFilter = false;
            break;
        }

        case ReadPreference::PrimaryPreferred: {
            // ignore tags and max staleness for primary query
            auto primaryCriteria = ReadPreferenceSetting(ReadPreference::PrimaryOnly);
            _getCandidateServers(result, topologyDescription, primaryCriteria);
            if (result->size()) {
                shouldTagFilter = false;
                break;
            }

            // keep tags and maxStaleness for secondary query
            auto secondaryCriteria = criteria;
            secondaryCriteria.pref = ReadPreference::SecondaryOnly;
            _getCandidateServers(result, topologyDescription, secondaryCriteria);
            break;
        }

        case ReadPreference::SecondaryPreferred: {
            // keep tags and maxStaleness for secondary query
            auto secondaryCriteria = criteria;
            secondaryCriteria.pref = ReadPreference::SecondaryOnly;
            _getCandidateServers(result, topologyDescription, secondaryCriteria);
            if (result->size()) {
                break;
            }

            // ignore tags and maxStaleness for primary query
            shouldTagFilter = false;
            auto primaryCriteria = ReadPreferenceSetting(ReadPreference::PrimaryOnly);
            _getCandidateServers(result, topologyDescription, primaryCriteria);
            break;
        }

        default:
            MONGO_UNREACHABLE
    }

    if (shouldTagFilter) {
        filterTags(result, criteria.tags);
    }
}