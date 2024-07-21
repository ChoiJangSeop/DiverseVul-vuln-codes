static bool is_topic_in_criterias(
        const char* topic_name,
        const std::vector<Criteria>& criterias)
{
    bool returned_value = false;

    for (auto criteria_it = criterias.begin(); !returned_value &&
            criteria_it != criterias.end(); ++criteria_it)
    {
        for (auto topic : (*criteria_it).topics)
        {
            if (StringMatching::matchString(topic.c_str(), topic_name))
            {
                returned_value = true;
                break;
            }
        }
    }

    return returned_value;
}