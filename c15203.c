QPDFFormFieldObjectHelper::getChoices()
{
    std::vector<std::string> result;
    if (! isChoice())
    {
        return result;
    }
    QPDFObjectHandle opt = getInheritableFieldValue("/Opt");
    if (opt.isArray())
    {
        size_t n = opt.getArrayNItems();
        for (size_t i = 0; i < n; ++i)
        {
            QPDFObjectHandle item = opt.getArrayItem(i);
            if (item.isString())
            {
                result.push_back(item.getUTF8Value());
            }
        }
    }
    return result;
}