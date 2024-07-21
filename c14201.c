ReturnCode_t DataReaderImpl::enable()
{
    assert(reader_ == nullptr);

    fastrtps::rtps::ReaderAttributes att;

    att.endpoint.durabilityKind = qos_.durability().durabilityKind();
    att.endpoint.endpointKind = READER;
    att.endpoint.multicastLocatorList = qos_.endpoint().multicast_locator_list;
    att.endpoint.reliabilityKind = qos_.reliability().kind == RELIABLE_RELIABILITY_QOS ? RELIABLE : BEST_EFFORT;
    att.endpoint.topicKind = type_->m_isGetKeyDefined ? WITH_KEY : NO_KEY;
    att.endpoint.unicastLocatorList = qos_.endpoint().unicast_locator_list;
    att.endpoint.remoteLocatorList = qos_.endpoint().remote_locator_list;
    att.endpoint.properties = qos_.properties();

    if (qos_.endpoint().entity_id > 0)
    {
        att.endpoint.setEntityID(static_cast<uint8_t>(qos_.endpoint().entity_id));
    }

    if (qos_.endpoint().user_defined_id > 0)
    {
        att.endpoint.setUserDefinedID(static_cast<uint8_t>(qos_.endpoint().user_defined_id));
    }

    att.times = qos_.reliable_reader_qos().times;
    att.liveliness_lease_duration = qos_.liveliness().lease_duration;
    att.liveliness_kind_ = qos_.liveliness().kind;
    att.matched_writers_allocation = qos_.reader_resource_limits().matched_publisher_allocation;
    att.expectsInlineQos = qos_.expects_inline_qos();
    att.disable_positive_acks = qos_.reliable_reader_qos().disable_positive_ACKs.enabled;


    // TODO(Ricardo) Remove in future
    // Insert topic_name and partitions
    Property property;
    property.name("topic_name");
    property.value(topic_->get_name().c_str());
    att.endpoint.properties.properties().push_back(std::move(property));
    if (subscriber_->get_qos().partition().names().size() > 0)
    {
        property.name("partitions");
        std::string partitions;
        for (auto partition : subscriber_->get_qos().partition().names())
        {
            partitions += partition + ";";
        }
        property.value(std::move(partitions));
        att.endpoint.properties.properties().push_back(std::move(property));
    }

    RTPSReader* reader = RTPSDomain::createRTPSReader(
        subscriber_->rtps_participant(),
        att,
        static_cast<ReaderHistory*>(&history_),
        static_cast<ReaderListener*>(&reader_listener_));

    if (reader == nullptr)
    {
        logError(DATA_READER, "Problem creating associated Reader");
        return ReturnCode_t::RETCODE_ERROR;
    }

    reader_ = reader;

    deadline_timer_ = new TimedEvent(subscriber_->get_participant()->get_resource_event(),
                    [&]() -> bool
                    {
                        return deadline_missed();
                    },
                    qos_.deadline().period.to_ns() * 1e-6);

    lifespan_timer_ = new TimedEvent(subscriber_->get_participant()->get_resource_event(),
                    [&]() -> bool
                    {
                        return lifespan_expired();
                    },
                    qos_.lifespan().duration.to_ns() * 1e-6);

    // Register the reader
    ReaderQos rqos = qos_.get_readerqos(subscriber_->get_qos());
    subscriber_->rtps_participant()->registerReader(reader_, topic_attributes(), rqos);

    return ReturnCode_t::RETCODE_OK;
}