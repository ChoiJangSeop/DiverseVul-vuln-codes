std::string generate_page_contents(int pageno)
{
    std::string contents =
        "BT /F1 24 Tf 72 720 Td (page " +  QUtil::int_to_string(pageno) +
        ") Tj ET\n"
        "q 468 0 0 468 72 72 cm /Im1 Do Q\n";
    return contents;
}