static bool is_partition_in_criterias(
        const std::string& partition,
        const std::vector<Criteria>& criterias)
{
    bool returned_value = false;

    for (auto criteria_it = criterias.begin(); !returned_value &&
            criteria_it != criterias.end(); ++criteria_it)
    {
        for (auto part : (*criteria_it).partitions)
        {
            if (StringMatching::matchString(partition.c_str(), part.c_str()))
            {
                returned_value = true;
                break;
            }
        }
    }

    return returned_value;
}