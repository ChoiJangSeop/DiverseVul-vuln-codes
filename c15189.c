int wmain(int argc, wchar_t* argv[])
{
    // If wmain is supported, argv contains UTF-16-encoded strings
    // with a 16-bit wchar_t. Convert this to UTF-8-encoded strings
    // for compatibility with other systems. That way the rest of
    // qpdf.cc can just act like arguments are UTF-8.
    std::vector<PointerHolder<char> > utf8_argv;
    for (int i = 0; i < argc; ++i)
    {
        std::string utf16;
        for (size_t j = 0; j < wcslen(argv[i]); ++j)
        {
            unsigned short codepoint = static_cast<unsigned short>(argv[i][j]);
            utf16.append(1, static_cast<unsigned char>(codepoint >> 8));
            utf16.append(1, static_cast<unsigned char>(codepoint & 0xff));
        }
        std::string utf8 = QUtil::utf16_to_utf8(utf16);
        utf8_argv.push_back(
            PointerHolder<char>(true, QUtil::copy_string(utf8.c_str())));
    }
    PointerHolder<char*> utf8_argv_ph =
        PointerHolder<char*>(true, new char*[1+utf8_argv.size()]);
    char** new_argv = utf8_argv_ph.getPointer();
    for (size_t i = 0; i < utf8_argv.size(); ++i)
    {
        new_argv[i] = utf8_argv.at(i).getPointer();
    }
    argc = static_cast<int>(utf8_argv.size());
    new_argv[argc] = 0;
    return realmain(argc, new_argv);
}