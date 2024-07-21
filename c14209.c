ReturnCode_t DataReaderImpl::set_qos(
        const DataReaderQos& qos)
{
    bool enabled = reader_ != nullptr;
    const DataReaderQos& qos_to_set = (&qos == &DATAREADER_QOS_DEFAULT) ?
            subscriber_->get_default_datareader_qos() : qos;

    // Default qos is always considered consistent
    if (&qos != &DATAREADER_QOS_DEFAULT)
    {
        if (subscriber_->get_participant()->get_qos().allocation().data_limits.max_user_data != 0 &&
                subscriber_->get_participant()->get_qos().allocation().data_limits.max_user_data <
                qos_to_set.user_data().getValue().size())
        {
            return ReturnCode_t::RETCODE_INCONSISTENT_POLICY;
        }

        ReturnCode_t check_result = check_qos(qos_to_set);
        if (!check_result)
        {
            return check_result;
        }
    }

    if (enabled && !can_qos_be_updated(qos_, qos_to_set))
    {
        return ReturnCode_t::RETCODE_IMMUTABLE_POLICY;
    }

    set_qos(qos_, qos_to_set, !enabled);

    if (enabled)
    {
        //NOTIFY THE BUILTIN PROTOCOLS THAT THE READER HAS CHANGED
        ReaderQos rqos = qos.get_readerqos(get_subscriber()->get_qos());
        subscriber_->rtps_participant()->updateReader(reader_, topic_attributes(), rqos);

        // Deadline
        if (qos_.deadline().period != c_TimeInfinite)
        {
            deadline_duration_us_ = duration<double, std::ratio<1, 1000000> >(qos_.deadline().period.to_ns() * 1e-3);
            deadline_timer_->update_interval_millisec(qos_.deadline().period.to_ns() * 1e-6);
        }
        else
        {
            deadline_timer_->cancel_timer();
        }

        // Lifespan
        if (qos_.lifespan().duration != c_TimeInfinite)
        {
            lifespan_duration_us_ =
                    std::chrono::duration<double, std::ratio<1, 1000000> >(qos_.lifespan().duration.to_ns() * 1e-3);
            lifespan_timer_->update_interval_millisec(qos_.lifespan().duration.to_ns() * 1e-6);
        }
        else
        {
            lifespan_timer_->cancel_timer();
        }
    }

    return ReturnCode_t::RETCODE_OK;
}