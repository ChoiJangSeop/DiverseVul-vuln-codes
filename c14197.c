Publisher* ParticipantImpl::createPublisher(
        const PublisherAttributes& att,
        PublisherListener* listen)
{
    logInfo(PARTICIPANT, "CREATING PUBLISHER IN TOPIC: " << att.topic.getTopicName());
    //Look for the correct type registration

    TopicDataType* p_type = nullptr;

    /// Preconditions
    // Check the type was registered.
    if (!getRegisteredType(att.topic.getTopicDataType().c_str(), &p_type))
    {
        logError(PARTICIPANT, "Type : " << att.topic.getTopicDataType() << " Not Registered");
        return nullptr;
    }
    // Check the type supports keys.
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
        logError(PARTICIPANT, "Unicast Locator List for Publisher contains invalid Locator");
        return nullptr;
    }
    if (!att.multicastLocatorList.isValid())
    {
        logError(PARTICIPANT, " Multicast Locator List for Publisher contains invalid Locator");
        return nullptr;
    }
    if (!att.remoteLocatorList.isValid())
    {
        logError(PARTICIPANT, "Remote Locator List for Publisher contains invalid Locator");
        return nullptr;
    }
    if (!att.qos.checkQos() || !att.topic.checkQos())
    {
        return nullptr;
    }

    //TODO CONSTRUIR LA IMPLEMENTACION DENTRO DEL OBJETO DEL USUARIO.
    PublisherImpl* pubimpl = new PublisherImpl(this, p_type, att, listen);
    Publisher* pub = new Publisher(pubimpl);
    pubimpl->mp_userPublisher = pub;
    pubimpl->mp_rtpsParticipant = this->mp_rtpsParticipant;

    WriterAttributes watt;
    watt.throughputController = att.throughputController;
    watt.endpoint.durabilityKind = att.qos.m_durability.durabilityKind();
    watt.endpoint.endpointKind = WRITER;
    watt.endpoint.multicastLocatorList = att.multicastLocatorList;
    watt.endpoint.reliabilityKind = att.qos.m_reliability.kind == RELIABLE_RELIABILITY_QOS ? RELIABLE : BEST_EFFORT;
    watt.endpoint.topicKind = att.topic.topicKind;
    watt.endpoint.unicastLocatorList = att.unicastLocatorList;
    watt.endpoint.remoteLocatorList = att.remoteLocatorList;
    watt.mode = att.qos.m_publishMode.kind ==
            eprosima::fastrtps::SYNCHRONOUS_PUBLISH_MODE ? SYNCHRONOUS_WRITER : ASYNCHRONOUS_WRITER;
    watt.endpoint.properties = att.properties;
    if (att.getEntityID() > 0)
    {
        watt.endpoint.setEntityID((uint8_t)att.getEntityID());
    }
    if (att.getUserDefinedID() > 0)
    {
        watt.endpoint.setUserDefinedID((uint8_t)att.getUserDefinedID());
    }
    watt.times = att.times;
    watt.liveliness_kind = att.qos.m_liveliness.kind;
    watt.liveliness_lease_duration = att.qos.m_liveliness.lease_duration;
    watt.liveliness_announcement_period = att.qos.m_liveliness.announcement_period;
    watt.matched_readers_allocation = att.matched_subscriber_allocation;

    // TODO(Ricardo) Remove in future
    // Insert topic_name and partitions
    Property property;
    property.name("topic_name");
    property.value(att.topic.getTopicName().c_str());
    watt.endpoint.properties.properties().push_back(std::move(property));
    if (att.qos.m_partition.names().size() > 0)
    {
        property.name("partitions");
        std::string partitions;
        for (auto partition : att.qos.m_partition.names())
        {
            partitions += partition + ";";
        }
        property.value(std::move(partitions));
        watt.endpoint.properties.properties().push_back(std::move(property));
    }
    if (att.qos.m_disablePositiveACKs.enabled &&
            att.qos.m_disablePositiveACKs.duration != c_TimeInfinite)
    {
        watt.disable_positive_acks = true;
        watt.keep_duration = att.qos.m_disablePositiveACKs.duration;
    }

    RTPSWriter* writer = RTPSDomain::createRTPSWriter(
        this->mp_rtpsParticipant,
        watt,
        (WriterHistory*)&pubimpl->m_history,
        (WriterListener*)&pubimpl->m_writerListener);
    if (writer == nullptr)
    {
        logError(PARTICIPANT, "Problem creating associated Writer");
        delete(pubimpl);
        return nullptr;
    }
    pubimpl->mp_writer = writer;

    // In case it has been loaded from the persistence DB, rebuild instances on history
    pubimpl->m_history.rebuild_instances();

    //SAVE THE PUBLISHER PAIR
    t_p_PublisherPair pubpair;
    pubpair.first = pub;
    pubpair.second = pubimpl;
    m_publishers.push_back(pubpair);

    //REGISTER THE WRITER
    this->mp_rtpsParticipant->registerWriter(writer, att.topic, att.qos);

    return pub;
}