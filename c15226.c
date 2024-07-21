QPDF::getExtensionLevel()
{
    int result = 0;
    QPDFObjectHandle obj = getRoot();
    if (obj.hasKey("/Extensions"))
    {
        obj = obj.getKey("/Extensions");
        if (obj.isDictionary() && obj.hasKey("/ADBE"))
        {
            obj = obj.getKey("/ADBE");
            if (obj.isDictionary() && obj.hasKey("/ExtensionLevel"))
            {
                obj = obj.getKey("/ExtensionLevel");
                if (obj.isInteger())
                {
                    result = obj.getIntValue();
                }
            }
        }
    }
    return result;
}