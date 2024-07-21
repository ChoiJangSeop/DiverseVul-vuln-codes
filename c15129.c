QPDFObjectHandle::rotatePage(int angle, bool relative)
{
    if ((angle % 90) != 0)
    {
        throw std::runtime_error(
            "QPDF::rotatePage called with an"
            " angle that is not a multiple of 90");
    }
    int new_angle = angle;
    if (relative)
    {
        int old_angle = 0;
        bool found_rotate = false;
        QPDFObjectHandle cur_obj = *this;
        bool searched_parent = false;
        std::set<QPDFObjGen> visited;
        while (! found_rotate)
        {
            if (visited.count(cur_obj.getObjGen()))
            {
                // Don't get stuck in an infinite loop
                break;
            }
            if (! visited.empty())
            {
                searched_parent = true;
            }
            visited.insert(cur_obj.getObjGen());
            if (cur_obj.getKey("/Rotate").isInteger())
            {
                found_rotate = true;
                old_angle = cur_obj.getKey("/Rotate").getIntValue();
            }
            else if (cur_obj.getKey("/Parent").isDictionary())
            {
                cur_obj = cur_obj.getKey("/Parent");
            }
            else
            {
                break;
            }
        }
        QTC::TC("qpdf", "QPDFObjectHandle found old angle",
                searched_parent ? 0 : 1);
        if ((old_angle % 90) != 0)
        {
            old_angle = 0;
        }
        new_angle += old_angle;
    }
    new_angle = (new_angle + 360) % 360;
    replaceKey("/Rotate", QPDFObjectHandle::newInteger(new_angle));
}