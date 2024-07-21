bool Permissions::check_remote_datawriter(
        const PermissionsHandle& remote_handle,
        const uint32_t domain_id,
        const WriterProxyData& publication_data,
        SecurityException& exception)
{
    bool returned_value = false;
    const AccessPermissionsHandle& rah = AccessPermissionsHandle::narrow(remote_handle);

    if (rah.nil())
    {
        exception = _SecurityException_("Bad precondition");
        EMERGENCY_SECURITY_LOGGING("Permissions", exception.what());
        return false;
    }

    const EndpointSecurityAttributes* attributes = nullptr;

    if ((attributes = is_topic_in_sec_attributes(publication_data.topicName().c_str(), rah->governance_topic_rules_))
            != nullptr)
    {
        if (!attributes->is_write_protected)
        {
            return true;
        }
    }
    else
    {
        exception = _SecurityException_(
            "Not found topic access rule for topic " + publication_data.topicName().to_string());
        EMERGENCY_SECURITY_LOGGING("Permissions", exception.what());
        return false;
    }

    for (auto rule : rah->grant.rules)
    {
        if (is_domain_in_set(domain_id, rule.domains))
        {
            if (is_topic_in_criterias(publication_data.topicName().c_str(), rule.publishes))
            {
                if (rule.allow)
                {
                    returned_value = true;
                }
                else
                {
                    exception = _SecurityException_(publication_data.topicName().to_string() +
                                    std::string(" topic denied by deny rule."));
                    EMERGENCY_SECURITY_LOGGING("Permissions", exception.what());
                }

                break;
            }
        }
    }

    if (!returned_value && strlen(exception.what()) == 0)
    {
        exception = _SecurityException_(publication_data.topicName().to_string() +
                        std::string(" topic not found in allow rule."));
        EMERGENCY_SECURITY_LOGGING("Permissions", exception.what());
    }

    return returned_value;
}