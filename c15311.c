QPDF_Stream::understandDecodeParams(
    std::string const& filter, QPDFObjectHandle decode_obj,
    int& predictor, int& columns,
    int& colors, int& bits_per_component,
    bool& early_code_change)
{
    bool filterable = true;
    std::set<std::string> keys = decode_obj.getKeys();
    for (std::set<std::string>::iterator iter = keys.begin();
         iter != keys.end(); ++iter)
    {
        std::string const& key = *iter;
        if (((filter == "/FlateDecode") || (filter == "/LZWDecode")) &&
            (key == "/Predictor"))
        {
            QPDFObjectHandle predictor_obj = decode_obj.getKey(key);
            if (predictor_obj.isInteger())
            {
                predictor = predictor_obj.getIntValue();
                if (! ((predictor == 1) || (predictor == 2) ||
                       ((predictor >= 10) && (predictor <= 15))))
                {
                    filterable = false;
                }
            }
            else
            {
                filterable = false;
            }
        }
        else if ((filter == "/LZWDecode") && (key == "/EarlyChange"))
        {
            QPDFObjectHandle earlychange_obj = decode_obj.getKey(key);
            if (earlychange_obj.isInteger())
            {
                int earlychange = earlychange_obj.getIntValue();
                early_code_change = (earlychange == 1);
                if (! ((earlychange == 0) || (earlychange == 1)))
                {
                    filterable = false;
                }
            }
            else
            {
                filterable = false;
            }
        }
        else if ((key == "/Columns") ||
                 (key == "/Colors") ||
                 (key == "/BitsPerComponent"))
        {
            QPDFObjectHandle param_obj = decode_obj.getKey(key);
            if (param_obj.isInteger())
            {
                int val = param_obj.getIntValue();
                if (key == "/Columns")
                {
                    columns = val;
                }
                else if (key == "/Colors")
                {
                    colors = val;
                }
                else if (key == "/BitsPerComponent")
                {
                    bits_per_component = val;
                }
            }
            else
            {
                filterable = false;
            }
        }
        else if ((filter == "/Crypt") &&
                 (((key == "/Type") || (key == "/Name")) &&
                  (decode_obj.getKey("/Type").isNull() ||
                   (decode_obj.getKey("/Type").isName() &&
                    (decode_obj.getKey("/Type").getName() ==
                     "/CryptFilterDecodeParms")))))
        {
            // we handle this in decryptStream
        }
        else
        {
            filterable = false;
        }
    }

    return filterable;
}