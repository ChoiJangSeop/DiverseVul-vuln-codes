QPDFObjectHandle::isMatrix()
{
    if (! isArray())
    {
        return false;
    }
    if (getArrayNItems() != 6)
    {
        return false;
    }
    for (size_t i = 0; i < 6; ++i)
    {
        if (! getArrayItem(i).isNumber())
        {
            return false;
        }
    }
    return true;
}