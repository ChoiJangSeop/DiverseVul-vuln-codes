ArgParser::argKeepFilesOpenThreshold(char* parameter)
{
    o.keep_files_open_threshold =
        static_cast<size_t>(QUtil::string_to_int(parameter));
}