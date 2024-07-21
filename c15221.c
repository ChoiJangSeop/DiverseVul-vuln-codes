QPDFFormFieldObjectHelper::getFlags()
{
    QPDFObjectHandle f = getInheritableFieldValue("/Ff");
    return f.isInteger() ? f.getIntValue() : 0;
}