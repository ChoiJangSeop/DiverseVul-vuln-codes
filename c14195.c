Subscriber* ParticipantImpl::createSubscriber(
        const SubscriberAttributes& att,
        SubscriberListener* listen)
{
    logInfo(PARTICIPANT, "CREATING SUBSCRIBER IN TOPIC: " << att.topic.getTopicName())
    //Look for the correct type registration

    TopicDataType* p_type = nullptr;

    if (!getRegisteredType(att.topic.getTopicDataType().c_str(), &p_type))
    {
        logError(PARTICIPANT, "Type : " << att.topic.getTopicDataType() << " Not Registered");
        return nullptr;
    }
    if (att.topic.topicKind == WITH_KEY && !p_type->m_isGetKeyDefined)
    {
        logError(PARTICIPANT, "Keyed Topic needs getKey function");
        return nullptr;
    }
    if (m_att.rtps.builtin.discovery_config.use_STATIC_EndpointDiscoveryProtocol)
    {
        if (att.getUserDefinedID() <= 0)
        {
            logError(PARTICIPANT, "Static EDP requires user defined Id");
            return nullptr;
        }
    }
    if (!att.unicastLocatorList.isValid())
    {
        logError(PARTICIPANT, "Unicast Locator List for Subscriber contains invalid Locator");
        return nullptr;
    }
    if (!att.multicastLocatorList.isValid())
    {
        logError(PARTICIPANT, " Multicast Locator List for Subscriber contains invalid Locator");
        return nullptr;
    }
    if (!att.remoteLocatorList.isValid())
    {
        logError(PARTICIPANT, "Output Locator List for Subscriber contains invalid Locator");
        return nullptr;
    }
    if (!att.qos.checkQos() || !att.topic.checkQos())
    {
        return nullptr;
    }

    SubscriberImpl* subimpl = new SubscriberImpl(this, p_type, att, listen);
    Subscriber* sub = new Subscriber(subimpl);
    subimpl->mp_userSubscriber = sub;
    subimpl->mp_rtpsParticipant = this->mp_rtpsParticipant;

    ReaderAttributes ratt;
    ratt.endpoint.durabilityKind = att.qos.m_durability.durabilityKind();
    ratt.endpoint.endpointKind = READER;
    ratt.endpoint.multicastLocatorList = att.multicastLocatorList;
    ratt.endpoint.reliabilityKind = att.qos.m_reliability.kind == RELIABLE_RELIABILITY_QOS ? RELIABLE : BEST_EFFORT;
    ratt.endpoint.topicKind = att.topic.topicKind;
    ratt.endpoint.unicastLocatorList = att.unicastLocatorList;
    ratt.endpoint.remoteLocatorList = att.remoteLocatorList;
    ratt.expectsInlineQos = att.expectsInlineQos;
    ratt.endpoint.properties = att.properties;
    if (att.getEntityID() > 0)
    {
        ratt.endpoint.setEntityID((uint8_t)att.getEntityID());
    }
    if (att.getUserDefinedID() > 0)
    {
        ratt.endpoint.setUserDefinedID((uint8_t)att.getUserDefinedID());
    }
    ratt.times = att.times;
    ratt.matched_writers_allocation = att.matched_publisher_allocation;
    ratt.liveliness_kind_ = att.qos.m_liveliness.kind;
    ratt.liveliness_lease_duration = att.qos.m_liveliness.lease_duration;

    // TODO(Ricardo) Remove in future
    // Insert topic_name and partitions
    Property property;
    property.name("topic_name");
    property.value(att.topic.getTopicName().c_str());
    ratt.endpoint.properties.properties().push_back(std::move(property));
    if (att.qos.m_partition.names().size() > 0)
    {
        property.name("partitions");
        std::string partitions;
        for (auto partition : att.qos.m_partition.names())
        {
            partitions += partition + ";";
        }
        property.value(std::move(partitions));
        ratt.endpoint.properties.properties().push_back(std::move(property));
    }
    if (att.qos.m_disablePositiveACKs.enabled)
    {
        ratt.disable_positive_acks = true;
    }

    RTPSReader* reader = RTPSDomain::createRTPSReader(this->mp_rtpsParticipant,
                    ratt,
                    (ReaderHistory*)&subimpl->m_history,
                    (ReaderListener*)&subimpl->m_readerListener);
    if (reader == nullptr)
    {
        logError(PARTICIPANT, "Problem creating associated Reader");
        delete(subimpl);
        return nullptr;
    }
    subimpl->mp_reader = reader;
    //SAVE THE PUBLICHER PAIR
    t_p_SubscriberPair subpair;
    subpair.first = sub;
    subpair.second = subimpl;
    m_subscribers.push_back(subpair);

    //REGISTER THE READER
    this->mp_rtpsParticipant->registerReader(reader, att.topic, att.qos);

    return sub;
}