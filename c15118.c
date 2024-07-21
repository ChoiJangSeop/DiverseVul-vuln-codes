QPDFOutlineObjectHelper::getCount()
{
    int count = 0;
    if (this->oh.hasKey("/Count"))
    {
        count = this->oh.getKey("/Count").getIntValue();
    }
    return count;
}