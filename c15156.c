ValueSetter::writeAppearance()
{
    this->replaced = true;

    // This code does not take quadding into consideration because
    // doing so requires font metric information, which we don't
    // have in many cases.

    double tfh = 1.2 * tf;
    int dx = 1;

    // Write one or more lines, centered vertically, possibly with
    // one row highlighted.

    size_t max_rows = static_cast<size_t>((bbox.ury - bbox.lly) / tfh);
    bool highlight = false;
    size_t highlight_idx = 0;

    std::vector<std::string> lines;
    if (opt.empty() || (max_rows < 2))
    {
        lines.push_back(V);
    }
    else
    {
        // Figure out what rows to write
        size_t nopt = opt.size();
        size_t found_idx = 0;
        bool found = false;
        for (found_idx = 0; found_idx < nopt; ++found_idx)
        {
            if (opt.at(found_idx) == V)
            {
                found = true;
                break;
            }
        }
        if (found)
        {
            // Try to make the found item the second one, but
            // adjust for under/overflow.
            int wanted_first = found_idx - 1;
            int wanted_last = found_idx + max_rows - 2;
            QTC::TC("qpdf", "QPDFFormFieldObjectHelper list found");
            while (wanted_first < 0)
            {
                QTC::TC("qpdf", "QPDFFormFieldObjectHelper list first too low");
                ++wanted_first;
                ++wanted_last;
            }
            while (wanted_last >= static_cast<int>(nopt))
            {
                QTC::TC("qpdf", "QPDFFormFieldObjectHelper list last too high");
                if (wanted_first > 0)
                {
                    --wanted_first;
                }
                --wanted_last;
            }
            highlight = true;
            highlight_idx = found_idx - wanted_first;
            for (int i = wanted_first; i <= wanted_last; ++i)
            {
                lines.push_back(opt.at(i));
            }
        }
        else
        {
            QTC::TC("qpdf", "QPDFFormFieldObjectHelper list not found");
            // include our value and the first n-1 rows
            highlight_idx = 0;
            highlight = true;
            lines.push_back(V);
            for (size_t i = 0; ((i < nopt) && (i < (max_rows - 1))); ++i)
            {
                lines.push_back(opt.at(i));
            }
        }
    }

    // Write the lines centered vertically, highlighting if needed
    size_t nlines = lines.size();
    double dy = bbox.ury - ((bbox.ury - bbox.lly - (nlines * tfh)) / 2.0);
    if (highlight)
    {
        write("q\n0.85 0.85 0.85 rg\n" +
              QUtil::double_to_string(bbox.llx) + " " +
              QUtil::double_to_string(bbox.lly + dy -
                                      (tfh * (highlight_idx + 1))) + " " +
              QUtil::double_to_string(bbox.urx - bbox.llx) + " " +
              QUtil::double_to_string(tfh) +
              " re f\nQ\n");
    }
    dy -= tf;
    write("q\nBT\n" + DA + "\n");
    for (size_t i = 0; i < nlines; ++i)
    {
        // We could adjust Tm to translate to the beginning the first
        // line, set TL to tfh, and use T* for each subsequent line,
        // but doing this would require extracting any Tm from DA,
        // which doesn't seem really worth the effort.
        if (i == 0)
        {
            write(QUtil::double_to_string(bbox.llx + dx) + " " +
                  QUtil::double_to_string(bbox.lly + dy) + " Td\n");
        }
        else
        {
            write("0 " + QUtil::double_to_string(-tfh) + " Td\n");
        }
        write(QPDFObjectHandle::newString(lines.at(i)).unparse() + " Tj\n");
    }
    write("ET\nQ\nEMC");
}