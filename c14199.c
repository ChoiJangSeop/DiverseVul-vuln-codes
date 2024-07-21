bool Permissions::check_create_datareader(
        const PermissionsHandle& local_handle,
        const uint32_t /*domain_id*/,
        const std::string& topic_name,
        const std::vector<std::string>& partitions,
        SecurityException& exception)
{
    bool returned_value = false;
    const AccessPermissionsHandle& lah = AccessPermissionsHandle::narrow(local_handle);

    if (lah.nil())
    {
        exception = _SecurityException_("Bad precondition");
        EMERGENCY_SECURITY_LOGGING("Permissions", exception.what());
        return false;
    }

    const EndpointSecurityAttributes* attributes = nullptr;

    if ((attributes = is_topic_in_sec_attributes(topic_name.c_str(), lah->governance_topic_rules_)) != nullptr)
    {
        if (!attributes->is_read_protected)
        {
            return true;
        }
    }
    else
    {
        exception = _SecurityException_("Not found topic access rule for topic " + topic_name);
        EMERGENCY_SECURITY_LOGGING("Permissions", exception.what());
        return false;
    }

    for (auto rule : lah->grant.rules)
    {
        if (is_topic_in_criterias(topic_name.c_str(), rule.subscribes))
        {
            if (rule.allow)
            {
                returned_value = true;

                if (partitions.empty())
                {
                    if (!is_partition_in_criterias(std::string(), rule.subscribes))
                    {
                        returned_value = false;
                        exception = _SecurityException_(std::string("<empty> partition not found in rule."));
                        EMERGENCY_SECURITY_LOGGING("Permissions", exception.what());
                    }
                }
                else
                {
                    // Search partitions
                    for (auto partition_it = partitions.begin(); returned_value && partition_it != partitions.end();
                            ++partition_it)
                    {
                        if (!is_partition_in_criterias(*partition_it, rule.subscribes))
                        {
                            returned_value = false;
                            exception =
                                    _SecurityException_(*partition_it + std::string(" partition not found in rule."));
                            EMERGENCY_SECURITY_LOGGING("Permissions", exception.what());
                        }
                    }
                }
            }
            else
            {
                exception = _SecurityException_(topic_name + std::string(" topic denied by deny rule."));
                EMERGENCY_SECURITY_LOGGING("Permissions", exception.what());
            }

            break;
        }
    }

    if (!returned_value && strlen(exception.what()) == 0)
    {
        exception = _SecurityException_(topic_name + std::string(" topic not found in allow rule."));
        EMERGENCY_SECURITY_LOGGING("Permissions", exception.what());
    }

    return returned_value;
}