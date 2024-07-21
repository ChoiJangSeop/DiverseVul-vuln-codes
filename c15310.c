static void write_outfile(QPDF& pdf, Options& o)
{
    if (o.split_pages)
    {
        // Generate output file pattern
        std::string before;
        std::string after;
        size_t len = strlen(o.outfilename);
        char* num_spot = strstr(const_cast<char*>(o.outfilename), "%d");
        if (num_spot != 0)
        {
            QTC::TC("qpdf", "qpdf split-pages %d");
            before = std::string(o.outfilename, (num_spot - o.outfilename));
            after = num_spot + 2;
        }
        else if ((len >= 4) &&
                 (QUtil::strcasecmp(o.outfilename + len - 4, ".pdf") == 0))
        {
            QTC::TC("qpdf", "qpdf split-pages .pdf");
            before = std::string(o.outfilename, len - 4) + "-";
            after = o.outfilename + len - 4;
        }
        else
        {
            QTC::TC("qpdf", "qpdf split-pages other");
            before = std::string(o.outfilename) + "-";
        }

        if (! o.preserve_unreferenced_page_resources)
        {
            QPDFPageDocumentHelper dh(pdf);
            dh.removeUnreferencedResources();
        }
        QPDFPageLabelDocumentHelper pldh(pdf);
        std::vector<QPDFObjectHandle> const& pages = pdf.getAllPages();
        int pageno_len = QUtil::int_to_string(pages.size()).length();
        unsigned int num_pages = pages.size();
        for (unsigned int i = 0; i < num_pages; i += o.split_pages)
        {
            unsigned int first = i + 1;
            unsigned int last = i + o.split_pages;
            if (last > num_pages)
            {
                last = num_pages;
            }
            QPDF outpdf;
            outpdf.emptyPDF();
            for (unsigned int pageno = first; pageno <= last; ++pageno)
            {
                QPDFObjectHandle page = pages.at(pageno - 1);
                outpdf.addPage(page, false);
            }
            if (pldh.hasPageLabels())
            {
                std::vector<QPDFObjectHandle> labels;
                pldh.getLabelsForPageRange(first - 1, last - 1, 0, labels);
                QPDFObjectHandle page_labels =
                    QPDFObjectHandle::newDictionary();
                page_labels.replaceKey(
                    "/Nums", QPDFObjectHandle::newArray(labels));
                outpdf.getRoot().replaceKey("/PageLabels", page_labels);
            }
            std::string page_range = QUtil::int_to_string(first, pageno_len);
            if (o.split_pages > 1)
            {
                page_range += "-" + QUtil::int_to_string(last, pageno_len);
            }
            std::string outfile = before + page_range + after;
            QPDFWriter w(outpdf, outfile.c_str());
            set_writer_options(outpdf, o, w);
            w.write();
            if (o.verbose)
            {
                std::cout << whoami << ": wrote file " << outfile << std::endl;
            }
        }
    }
    else
    {
        if (strcmp(o.outfilename, "-") == 0)
        {
            o.outfilename = 0;
        }
        QPDFWriter w(pdf, o.outfilename);
        set_writer_options(pdf, o, w);
        w.write();
        if (o.verbose)
        {
            std::cout << whoami << ": wrote file "
                      << o.outfilename << std::endl;
        }
    }
}