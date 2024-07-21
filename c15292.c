static void check_page_contents(int pageno, QPDFObjectHandle page)
{
    PointerHolder<Buffer> buf =
        page.getKey("/Contents").getStreamData();
    std::string actual_contents =
        std::string(reinterpret_cast<char *>(buf->getBuffer()),
                    buf->getSize());
    std::string expected_contents = generate_page_contents(pageno);
    if (expected_contents != actual_contents)
    {
        std::cout << "page contents wrong for page " << pageno << std::endl
                  << "ACTUAL: " << actual_contents
                  << "EXPECTED: " << expected_contents
                  << "----\n";
    }
}