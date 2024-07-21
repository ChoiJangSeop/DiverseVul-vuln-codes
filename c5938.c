static void parse_rotation_parameter(Options& o, std::string const& parameter)
{
    std::string angle_str;
    std::string range;
    size_t colon = parameter.find(':');
    int relative = 0;
    if (colon != std::string::npos)
    {
        if (colon > 0)
        {
            angle_str = parameter.substr(0, colon);
            if (angle_str.length() > 0)
            {
                char first = angle_str.at(0);
                if ((first == '+') || (first == '-'))
                {
                    relative = ((first == '+') ? 1 : -1);
                    angle_str = angle_str.substr(1);
                }
                else if (! QUtil::is_digit(angle_str.at(0)))
                {
                    angle_str = "";
                }
            }
        }
        if (colon + 1 < parameter.length())
        {
            range = parameter.substr(colon + 1);
        }
    }
    bool range_valid = false;
    try
    {
        parse_numrange(range.c_str(), 0, true);
        range_valid = true;
    }
    catch (std::runtime_error)
    {
        // ignore
    }
    if (range_valid &&
        ((angle_str == "90") || (angle_str == "180") || (angle_str == "270")))
    {
        int angle = atoi(angle_str.c_str());
        if (relative == -1)
        {
            angle = -angle;
        }
        o.rotations[range] = RotationSpec(angle, (relative != 0));
    }
    else
    {
        usage("invalid parameter to rotate: " + parameter);
    }
}