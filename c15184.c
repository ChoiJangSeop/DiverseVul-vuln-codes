static void handle_page_specs(QPDF& pdf, Options& o)
{
    // Parse all page specifications and translate them into lists of
    // actual pages.

    // Handle "." as a shortcut for the input file
    for (std::vector<PageSpec>::iterator iter = o.page_specs.begin();
         iter != o.page_specs.end(); ++iter)
    {
        PageSpec& page_spec = *iter;
        if (page_spec.filename == ".")
        {
            page_spec.filename = o.infilename;
        }
    }

    if (! o.keep_files_open_set)
    {
        // Count the number of distinct files to determine whether we
        // should keep files open or not. Rather than trying to code
        // some portable heuristic based on OS limits, just hard-code
        // this at a given number and allow users to override.
        std::set<std::string> filenames;
        for (std::vector<PageSpec>::iterator iter = o.page_specs.begin();
             iter != o.page_specs.end(); ++iter)
        {
            PageSpec& page_spec = *iter;
            filenames.insert(page_spec.filename);
        }
        if (filenames.size() > o.keep_files_open_threshold)
        {
            QTC::TC("qpdf", "qpdf disable keep files open");
            if (o.verbose)
            {
                std::cout << whoami << ": selecting --keep-open-files=n"
                          << std::endl;
            }
            o.keep_files_open = false;
        }
        else
        {
            if (o.verbose)
            {
                std::cout << whoami << ": selecting --keep-open-files=y"
                          << std::endl;
            }
            o.keep_files_open = true;
            QTC::TC("qpdf", "qpdf don't disable keep files open");
        }
    }

    // Create a QPDF object for each file that we may take pages from.
    std::vector<PointerHolder<QPDF> > page_heap;
    std::map<std::string, QPDF*> page_spec_qpdfs;
    std::map<std::string, ClosedFileInputSource*> page_spec_cfis;
    page_spec_qpdfs[o.infilename] = &pdf;
    std::vector<QPDFPageData> parsed_specs;
    std::map<unsigned long long, std::set<QPDFObjGen> > copied_pages;
    for (std::vector<PageSpec>::iterator iter = o.page_specs.begin();
         iter != o.page_specs.end(); ++iter)
    {
        PageSpec& page_spec = *iter;
        if (page_spec_qpdfs.count(page_spec.filename) == 0)
        {
            // Open the PDF file and store the QPDF object. Throw a
            // PointerHolder to the qpdf into a heap so that it
            // survives through copying to the output but gets cleaned up
            // automatically at the end. Do not canonicalize the file
            // name. Using two different paths to refer to the same
            // file is a document workaround for duplicating a page.
            // If you are using this an example of how to do this with
            // the API, you can just create two different QPDF objects
            // to the same underlying file with the same path to
            // achieve the same affect.
            char const* password = page_spec.password;
            if (o.encryption_file && (password == 0) &&
                (page_spec.filename == o.encryption_file))
            {
                QTC::TC("qpdf", "qpdf pages encryption password");
                password = o.encryption_file_password;
            }
            if (o.verbose)
            {
                std::cout << whoami << ": processing "
                          << page_spec.filename << std::endl;
            }
            PointerHolder<InputSource> is;
            ClosedFileInputSource* cis = 0;
            if (! o.keep_files_open)
            {
                QTC::TC("qpdf", "qpdf keep files open n");
                cis = new ClosedFileInputSource(page_spec.filename.c_str());
                is = cis;
                cis->stayOpen(true);
            }
            else
            {
                QTC::TC("qpdf", "qpdf keep files open y");
                FileInputSource* fis = new FileInputSource();
                is = fis;
                fis->setFilename(page_spec.filename.c_str());
            }
            PointerHolder<QPDF> qpdf_ph = process_input_source(is, password, o);
            page_heap.push_back(qpdf_ph);
            page_spec_qpdfs[page_spec.filename] = qpdf_ph.getPointer();
            if (cis)
            {
                cis->stayOpen(false);
                page_spec_cfis[page_spec.filename] = cis;
            }
        }

        // Read original pages from the PDF, and parse the page range
        // associated with this occurrence of the file.
        parsed_specs.push_back(
            QPDFPageData(page_spec.filename,
                         page_spec_qpdfs[page_spec.filename],
                         page_spec.range));
    }

    if (! o.preserve_unreferenced_page_resources)
    {
        for (std::map<std::string, QPDF*>::iterator iter =
                 page_spec_qpdfs.begin();
             iter != page_spec_qpdfs.end(); ++iter)
        {
            std::string const& filename = (*iter).first;
            ClosedFileInputSource* cis = 0;
            if (page_spec_cfis.count(filename))
            {
                cis = page_spec_cfis[filename];
                cis->stayOpen(true);
            }
            QPDFPageDocumentHelper dh(*((*iter).second));
            dh.removeUnreferencedResources();
            if (cis)
            {
                cis->stayOpen(false);
            }
        }
    }

    // Clear all pages out of the primary QPDF's pages tree but leave
    // the objects in place in the file so they can be re-added
    // without changing their object numbers. This enables other
    // things in the original file, such as outlines, to continue to
    // work.
    if (o.verbose)
    {
        std::cout << whoami
                  << ": removing unreferenced pages from primary input"
                  << std::endl;
    }
    QPDFPageDocumentHelper dh(pdf);
    std::vector<QPDFPageObjectHelper> orig_pages = dh.getAllPages();
    for (std::vector<QPDFPageObjectHelper>::iterator iter =
             orig_pages.begin();
         iter != orig_pages.end(); ++iter)
    {
        dh.removePage(*iter);
    }

    if (o.collate && (parsed_specs.size() > 1))
    {
        // Collate the pages by selecting one page from each spec in
        // order. When a spec runs out of pages, stop selecting from
        // it.
        std::vector<QPDFPageData> new_parsed_specs;
        size_t nspecs = parsed_specs.size();
        size_t cur_page = 0;
        bool got_pages = true;
        while (got_pages)
        {
            got_pages = false;
            for (size_t i = 0; i < nspecs; ++i)
            {
                QPDFPageData& page_data = parsed_specs.at(i);
                if (cur_page < page_data.selected_pages.size())
                {
                    got_pages = true;
                    new_parsed_specs.push_back(
                        QPDFPageData(
                            page_data,
                            page_data.selected_pages.at(cur_page)));
                }
            }
            ++cur_page;
        }
        parsed_specs = new_parsed_specs;
    }

    // Add all the pages from all the files in the order specified.
    // Keep track of any pages from the original file that we are
    // selecting.
    std::set<int> selected_from_orig;
    std::vector<QPDFObjectHandle> new_labels;
    bool any_page_labels = false;
    int out_pageno = 0;
    for (std::vector<QPDFPageData>::iterator iter =
             parsed_specs.begin();
         iter != parsed_specs.end(); ++iter)
    {
        QPDFPageData& page_data = *iter;
        ClosedFileInputSource* cis = 0;
        if (page_spec_cfis.count(page_data.filename))
        {
            cis = page_spec_cfis[page_data.filename];
            cis->stayOpen(true);
        }
        QPDFPageLabelDocumentHelper pldh(*page_data.qpdf);
        if (pldh.hasPageLabels())
        {
            any_page_labels = true;
        }
        if (o.verbose)
        {
            std::cout << whoami << ": adding pages from "
                      << page_data.filename << std::endl;
        }
        for (std::vector<int>::iterator pageno_iter =
                 page_data.selected_pages.begin();
             pageno_iter != page_data.selected_pages.end();
             ++pageno_iter, ++out_pageno)
        {
            // Pages are specified from 1 but numbered from 0 in the
            // vector
            int pageno = *pageno_iter - 1;
            pldh.getLabelsForPageRange(pageno, pageno, out_pageno,
                                       new_labels);
            QPDFPageObjectHelper to_copy = page_data.orig_pages.at(pageno);
            QPDFObjGen to_copy_og = to_copy.getObjectHandle().getObjGen();
            unsigned long long from_uuid = page_data.qpdf->getUniqueId();
            if (copied_pages[from_uuid].count(to_copy_og))
            {
                QTC::TC("qpdf", "qpdf copy same page more than once",
                        (page_data.qpdf == &pdf) ? 0 : 1);
                to_copy = to_copy.shallowCopyPage();
            }
            else
            {
                copied_pages[from_uuid].insert(to_copy_og);
            }
            dh.addPage(to_copy, false);
            if (page_data.qpdf == &pdf)
            {
                // This is a page from the original file. Keep track
                // of the fact that we are using it.
                selected_from_orig.insert(pageno);
            }
        }
        if (cis)
        {
            cis->stayOpen(false);
        }
    }
    if (any_page_labels)
    {
        QPDFObjectHandle page_labels =
            QPDFObjectHandle::newDictionary();
        page_labels.replaceKey(
            "/Nums", QPDFObjectHandle::newArray(new_labels));
        pdf.getRoot().replaceKey("/PageLabels", page_labels);
    }

    // Delete page objects for unused page in primary. This prevents
    // those objects from being preserved by being referred to from
    // other places, such as the outlines dictionary.
    for (size_t pageno = 0; pageno < orig_pages.size(); ++pageno)
    {
        if (selected_from_orig.count(pageno) == 0)
        {
            pdf.replaceObject(
                orig_pages.at(pageno).getObjectHandle().getObjGen(),
                QPDFObjectHandle::newNull());
        }
    }
}