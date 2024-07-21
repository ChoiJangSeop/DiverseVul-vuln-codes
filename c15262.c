ImageOptimizer::makePipeline(std::string const& description, Pipeline* next)
{
    PointerHolder<Pipeline> result;
    QPDFObjectHandle dict = image.getDict();
    QPDFObjectHandle w_obj = dict.getKey("/Width");
    QPDFObjectHandle h_obj = dict.getKey("/Height");
    QPDFObjectHandle colorspace_obj = dict.getKey("/ColorSpace");
    if (! (w_obj.isNumber() && h_obj.isNumber()))
    {
        if (o.verbose && (! description.empty()))
        {
            std::cout << whoami << ": " << description
                      << ": not optimizing because image dictionary"
                      << " is missing required keys" << std::endl;
        }
        return result;
    }
    QPDFObjectHandle components_obj = dict.getKey("/BitsPerComponent");
    if (! (components_obj.isInteger() && (components_obj.getIntValue() == 8)))
    {
        QTC::TC("qpdf", "qpdf image optimize bits per component");
        if (o.verbose && (! description.empty()))
        {
            std::cout << whoami << ": " << description
                      << ": not optimizing because image has other than"
                      << " 8 bits per component" << std::endl;
        }
        return result;
    }
    // Files have been seen in the wild whose width and height are
    // floating point, which is goofy, but we can deal with it.
    JDIMENSION w = static_cast<JDIMENSION>(
        w_obj.isInteger() ? w_obj.getIntValue() : w_obj.getNumericValue());
    JDIMENSION h = static_cast<JDIMENSION>(
        h_obj.isInteger() ? h_obj.getIntValue() : h_obj.getNumericValue());
    std::string colorspace = (colorspace_obj.isName() ?
                              colorspace_obj.getName() :
                              std::string());
    int components = 0;
    J_COLOR_SPACE cs = JCS_UNKNOWN;
    if (colorspace == "/DeviceRGB")
    {
        components = 3;
        cs = JCS_RGB;
    }
    else if (colorspace == "/DeviceGray")
    {
        components = 1;
        cs = JCS_GRAYSCALE;
    }
    else if (colorspace == "/DeviceCMYK")
    {
        components = 4;
        cs = JCS_CMYK;
    }
    else
    {
        QTC::TC("qpdf", "qpdf image optimize colorspace");
        if (o.verbose && (! description.empty()))
        {
            std::cout << whoami << ": " << description
                      << ": not optimizing because qpdf can't optimize"
                      << " images with this colorspace" << std::endl;
        }
        return result;
    }
    if (((o.oi_min_width > 0) && (w <= o.oi_min_width)) ||
        ((o.oi_min_height > 0) && (h <= o.oi_min_height)) ||
        ((o.oi_min_area > 0) && ((w * h) <= o.oi_min_area)))
    {
        QTC::TC("qpdf", "qpdf image optimize too small");
        if (o.verbose && (! description.empty()))
        {
            std::cout << whoami << ": " << description
                      << ": not optimizing because image"
                      << " is smaller than requested minimum dimensions"
                      << std::endl;
        }
        return result;
    }

    result = new Pl_DCT("jpg", next, w, h, components, cs);
    return result;
}