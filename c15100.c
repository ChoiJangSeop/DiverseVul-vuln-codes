static void check(char const* filename,
                  std::vector<std::string> const& color_spaces,
                  std::vector<std::string> const& filters)
{
    // Each stream is compressed the way it is supposed to be. We will
    // add additional tests in qpdf.test to exercise QPDFWriter more
    // fully. In this case, we want to make sure that we actually have
    // RunLengthDecode and DCTDecode where we are supposed to and
    // FlateDecode where we provided no filters.

    // Each image is correct. For non-lossy image compression, the
    // uncompressed image data should exactly match what ImageProvider
    // provided. For the DCTDecode data, allow for some fuzz to handle
    // jpeg compression as well as its variance on different systems.

    // These tests should use QPDFObjectHandle's stream data retrieval
    // methods, but don't try to fully exercise them here. That is
    // done elsewhere.

    size_t n_color_spaces = color_spaces.size();
    size_t n_filters = filters.size();

    QPDF pdf;
    pdf.processFile(filename);
    QPDFPageDocumentHelper dh(pdf);
    std::vector<QPDFPageObjectHelper> pages = dh.getAllPages();
    if (n_color_spaces * n_filters != pages.size())
    {
        throw std::logic_error("incorrect number of pages");
    }
    size_t pageno = 1;
    bool errors = false;
    for (std::vector<QPDFPageObjectHelper>::iterator page_iter =
             pages.begin();
         page_iter != pages.end(); ++page_iter)
    {
        QPDFPageObjectHelper& page(*page_iter);
        std::map<std::string, QPDFObjectHandle> images = page.getPageImages();
        if (images.size() != 1)
        {
            throw std::logic_error("incorrect number of images on page");
        }

        // Check filter and color space.
        std::string desired_color_space =
            color_spaces[(pageno - 1) / n_color_spaces];
        std::string desired_filter =
            filters[(pageno - 1) % n_filters];
        // In the default mode, QPDFWriter will compress with
        // /FlateDecode if no filters are provided.
        if (desired_filter == "null")
        {
            desired_filter = "/FlateDecode";
        }
        QPDFObjectHandle image = images.begin()->second;
        QPDFObjectHandle image_dict = image.getDict();
        QPDFObjectHandle color_space = image_dict.getKey("/ColorSpace");
        QPDFObjectHandle filter = image_dict.getKey("/Filter");
        bool this_errors = false;
        if (! (filter.isName() && (filter.getName() == desired_filter)))
        {
            this_errors = errors = true;
            std::cout << "page " << pageno << ": expected filter "
                      << desired_filter << "; actual filter = "
                      << filter.unparse() << std::endl;
        }
        if (! (color_space.isName() &&
               (color_space.getName() == desired_color_space)))
        {
            this_errors = errors = true;
            std::cout << "page " << pageno << ": expected color space "
                      << desired_color_space << "; actual color space = "
                      << color_space.unparse() << std::endl;
        }

        if (! this_errors)
        {
            // Check image data
            PointerHolder<Buffer> actual_data =
                image.getStreamData(qpdf_dl_all);
            ImageProvider* p = new ImageProvider(desired_color_space, "null");
            PointerHolder<QPDFObjectHandle::StreamDataProvider> provider(p);
            Pl_Buffer b_p("get image data");
            provider->provideStreamData(0, 0, &b_p);
            PointerHolder<Buffer> desired_data(b_p.getBuffer());

            if (desired_data->getSize() != actual_data->getSize())
            {
                std::cout << "page " << pageno
                          << ": image data length mismatch" << std::endl;
                this_errors = errors = true;
            }
            else
            {
                // Compare bytes. For JPEG, allow a certain number of
                // the bytes to be off desired by more than a given
                // tolerance. Any of the samples may be a little off
                // because of lossy compression, and around sharp
                // edges, things can be quite off. For non-lossy
                // compression, do not allow any tolerance.
                unsigned char const* actual_bytes = actual_data->getBuffer();
                unsigned char const* desired_bytes = desired_data->getBuffer();
                size_t len = actual_data->getSize();
                unsigned int mismatches = 0;
                int tolerance = (
                    desired_filter == "/DCTDecode" ? 10 : 0);
                unsigned int threshold = (
                    desired_filter == "/DCTDecode" ? len / 40 : 0);
                for (size_t i = 0; i < len; ++i)
                {
                    int delta = actual_bytes[i] - desired_bytes[i];
                    if ((delta > tolerance) || (delta < -tolerance))
                    {
                        ++mismatches;
                    }
                }
                if (mismatches > threshold)
                {
                    std::cout << "page " << pageno
                              << ": " << desired_color_space << ", "
                              << desired_filter
                              << ": mismatches: " << mismatches
                              << " of " << len << std::endl;
                    this_errors = errors = true;
                }
            }
        }

        ++pageno;
    }
    if (errors)
    {
        throw std::logic_error("errors found");
    }
    else
    {
        std::cout << "all checks passed" << std::endl;
    }
}