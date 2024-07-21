QUtil::parse_numrange(char const* range, int max)
{
    std::vector<int> result;
    char const* p = range;
    try
    {
        std::vector<int> work;
        static int const comma = -1;
        static int const dash = -2;

        enum { st_top,
               st_in_number,
               st_after_number } state = st_top;
        bool last_separator_was_dash = false;
        int cur_number = 0;
        bool from_end = false;
        while (*p)
        {
            char ch = *p;
            if (isdigit(ch))
            {
                if (! ((state == st_top) || (state == st_in_number)))
                {
                    throw std::runtime_error("digit not expected");
                }
                state = st_in_number;
                cur_number *= 10;
                cur_number += (ch - '0');
            }
            else if (ch == 'z')
            {
                // z represents max
                if (! (state == st_top))
                {
                    throw std::runtime_error("z not expected");
                }
                state = st_after_number;
                cur_number = max;
            }
            else if (ch == 'r')
            {
                if (! (state == st_top))
                {
                    throw std::runtime_error("r not expected");
                }
                state = st_in_number;
                from_end = true;
            }
            else if ((ch == ',') || (ch == '-'))
            {
                if (! ((state == st_in_number) || (state == st_after_number)))
                {
                    throw std::runtime_error("unexpected separator");
                }
                cur_number = maybe_from_end(cur_number, from_end, max);
                work.push_back(cur_number);
                cur_number = 0;
                from_end = false;
                if (ch == ',')
                {
                    state = st_top;
                    last_separator_was_dash = false;
                    work.push_back(comma);
                }
                else if (ch == '-')
                {
                    if (last_separator_was_dash)
                    {
                        throw std::runtime_error("unexpected dash");
                    }
                    state = st_top;
                    last_separator_was_dash = true;
                    work.push_back(dash);
                }
            }
            else
            {
                throw std::runtime_error("unexpected character");
            }
            ++p;
        }
        if ((state == st_in_number) || (state == st_after_number))
        {
            cur_number = maybe_from_end(cur_number, from_end, max);
            work.push_back(cur_number);
        }
        else
        {
            throw std::runtime_error("number expected");
        }

        p = 0;
        for (size_t i = 0; i < work.size(); i += 2)
        {
            int num = work.at(i);
            // max == 0 means we don't know the max and are just
            // testing for valid syntax.
            if ((max > 0) && ((num < 1) || (num > max)))
            {
                throw std::runtime_error(
                    "number " + QUtil::int_to_string(num) + " out of range");
            }
            if (i == 0)
            {
                result.push_back(work.at(i));
            }
            else
            {
                int separator = work.at(i-1);
                if (separator == comma)
                {
                    result.push_back(num);
                }
                else if (separator == dash)
                {
                    int lastnum = result.back();
                    if (num > lastnum)
                    {
                        for (int j = lastnum + 1; j <= num; ++j)
                        {
                            result.push_back(j);
                        }
                    }
                    else
                    {
                        for (int j = lastnum - 1; j >= num; --j)
                        {
                            result.push_back(j);
                        }
                    }
                }
                else
                {
                    throw std::logic_error(
                        "INTERNAL ERROR parsing numeric range");
                }
            }
        }
    }
    catch (std::runtime_error const& e)
    {
        std::string message;
        if (p)
        {
            message = "error at * in numeric range " +
                std::string(range, p - range) + "*" + p + ": " + e.what();
        }
        else
        {
            message = "error in numeric range " +
                std::string(range) + ": " + e.what();
        }
        throw std::runtime_error(message);
    }
    return result;
}