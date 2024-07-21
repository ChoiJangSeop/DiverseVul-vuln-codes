QPDFAnnotationObjectHelper::getFlags()
{
    QPDFObjectHandle flags_obj = this->oh.getKey("/F");
    return flags_obj.isInteger() ? flags_obj.getIntValue() : 0;
}