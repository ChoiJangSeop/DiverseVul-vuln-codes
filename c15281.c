QUtil::toUTF16(unsigned long uval)
{
    std::string result;
    if ((uval >= 0xd800) && (uval <= 0xdfff))
    {
        result = "\xff\xfd";
    }
    else if (uval <= 0xffff)
    {
        char out[2];
        out[0] = (uval & 0xff00) >> 8;
        out[1] = (uval & 0xff);
        result = std::string(out, 2);
    }
    else if (uval <= 0x10ffff)
    {
        char out[4];
        uval -= 0x10000;
        unsigned short high = ((uval & 0xffc00) >> 10) + 0xd800;
        unsigned short low = (uval & 0x3ff) + 0xdc00;
        out[0] = (high & 0xff00) >> 8;
        out[1] = (high & 0xff);
        out[2] = (low & 0xff00) >> 8;
        out[3] = (low & 0xff);
        result = std::string(out, 4);
    }
    else
    {
        result = "\xff\xfd";
    }

    return result;
}