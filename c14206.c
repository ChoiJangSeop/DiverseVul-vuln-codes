ReturnCode_t DataWriterImpl::enable()
{
    assert(writer_ == nullptr);

    WriterAttributes w_att;
    w_att.throughputController = qos_.throughput_controller();
    w_att.endpoint.durabilityKind = qos_.durability().durabilityKind();
    w_att.endpoint.endpointKind = WRITER;
    w_att.endpoint.multicastLocatorList = qos_.endpoint().multicast_locator_list;
    w_att.endpoint.reliabilityKind = qos_.reliability().kind == RELIABLE_RELIABILITY_QOS ? RELIABLE : BEST_EFFORT;
    w_att.endpoint.topicKind = type_->m_isGetKeyDefined ? WITH_KEY : NO_KEY;
    w_att.endpoint.unicastLocatorList = qos_.endpoint().unicast_locator_list;
    w_att.endpoint.remoteLocatorList = qos_.endpoint().remote_locator_list;
    w_att.mode = qos_.publish_mode().kind == SYNCHRONOUS_PUBLISH_MODE ? SYNCHRONOUS_WRITER : ASYNCHRONOUS_WRITER;
    w_att.endpoint.properties = qos_.properties();

    if (qos_.endpoint().entity_id > 0)
    {
        w_att.endpoint.setEntityID(static_cast<uint8_t>(qos_.endpoint().entity_id));
    }

    if (qos_.endpoint().user_defined_id > 0)
    {
        w_att.endpoint.setUserDefinedID(static_cast<uint8_t>(qos_.endpoint().user_defined_id));
    }

    w_att.times = qos_.reliable_writer_qos().times;
    w_att.liveliness_kind = qos_.liveliness().kind;
    w_att.liveliness_lease_duration = qos_.liveliness().lease_duration;
    w_att.liveliness_announcement_period = qos_.liveliness().announcement_period;
    w_att.matched_readers_allocation = qos_.writer_resource_limits().matched_subscriber_allocation;

    // TODO(Ricardo) Remove in future
    // Insert topic_name and partitions
    Property property;
    property.name("topic_name");
    property.value(topic_->get_name().c_str());
    w_att.endpoint.properties.properties().push_back(std::move(property));

    if (publisher_->get_qos().partition().names().size() > 0)
    {
        property.name("partitions");
        std::string partitions;
        for (auto partition : publisher_->get_qos().partition().names())
        {
            partitions += partition + ";";
        }
        property.value(std::move(partitions));
        w_att.endpoint.properties.properties().push_back(std::move(property));
    }

    if (qos_.reliable_writer_qos().disable_positive_acks.enabled &&
            qos_.reliable_writer_qos().disable_positive_acks.duration != c_TimeInfinite)
    {
        w_att.disable_positive_acks = true;
        w_att.keep_duration = qos_.reliable_writer_qos().disable_positive_acks.duration;
    }

    RTPSWriter* writer = RTPSDomain::createRTPSWriter(
        publisher_->rtps_participant(),
        w_att,
        static_cast<WriterHistory*>(&history_),
        static_cast<WriterListener*>(&writer_listener_));

    if (writer == nullptr)
    {
        logError(DATA_WRITER, "Problem creating associated Writer");
        return ReturnCode_t::RETCODE_ERROR;
    }

    writer_ = writer;

    // In case it has been loaded from the persistence DB, rebuild instances on history
    history_.rebuild_instances();

    //TODO(Ricardo) This logic in a class. Then a user of rtps layer can use it.
    if (high_mark_for_frag_ == 0)
    {
        RTPSParticipant* part = publisher_->rtps_participant();
        uint32_t max_data_size = writer_->getMaxDataSize();
        uint32_t writer_throughput_controller_bytes =
                writer_->calculateMaxDataSize(qos_.throughput_controller().bytesPerPeriod);
        uint32_t participant_throughput_controller_bytes =
                writer_->calculateMaxDataSize(
            part->getRTPSParticipantAttributes().throughputController.bytesPerPeriod);

        high_mark_for_frag_ =
                max_data_size > writer_throughput_controller_bytes ?
                writer_throughput_controller_bytes :
                (max_data_size > participant_throughput_controller_bytes ?
                participant_throughput_controller_bytes :
                max_data_size);
        high_mark_for_frag_ &= ~3;
    }

    for (PublisherHistory::iterator it = history_.changesBegin(); it != history_.changesEnd(); it++)
    {
        WriteParams wparams;
        set_fragment_size_on_change(wparams, *it, high_mark_for_frag_);
    }

    deadline_timer_ = new TimedEvent(publisher_->get_participant()->get_resource_event(),
                    [&]() -> bool
                    {
                        return deadline_missed();
                    },
                    qos_.deadline().period.to_ns() * 1e-6);

    lifespan_timer_ = new TimedEvent(publisher_->get_participant()->get_resource_event(),
                    [&]() -> bool
                    {
                        return lifespan_expired();
                    },
                    qos_.lifespan().duration.to_ns() * 1e-6);

    // REGISTER THE WRITER
    WriterQos wqos = qos_.get_writerqos(get_publisher()->get_qos(), topic_->get_qos());
    publisher_->rtps_participant()->registerWriter(writer_, get_topic_attributes(qos_, *topic_, type_), wqos);

    return ReturnCode_t::RETCODE_OK;
}