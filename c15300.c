QPDFObjectHandle::isRectangle()
{
    if (! isArray())
    {
        return false;
    }
    if (getArrayNItems() != 4)
    {
        return false;
    }
    for (size_t i = 0; i < 4; ++i)
    {
        if (! getArrayItem(i).isNumber())
        {
            return false;
        }
    }
    return true;
}