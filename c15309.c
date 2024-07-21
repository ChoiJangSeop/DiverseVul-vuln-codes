ArgParser::checkCompletion()
{
    // See if we're being invoked from bash completion.
    std::string bash_point_env;
    if (QUtil::get_env("COMP_LINE", &bash_line) &&
        QUtil::get_env("COMP_POINT", &bash_point_env))
    {
        int p = QUtil::string_to_int(bash_point_env.c_str());
        if ((p > 0) && (p <= static_cast<int>(bash_line.length())))
        {
            // Truncate the line. We ignore everything at or after the
            // cursor for completion purposes.
            bash_line = bash_line.substr(0, p);
        }
        // Set bash_cur and bash_prev based on bash_line rather than
        // relying on argv. This enables us to use bashcompinit to get
        // completion in zsh too since bashcompinit sets COMP_LINE and
        // COMP_POINT but doesn't invoke the command with options like
        // bash does.

        // p is equal to length of the string. Walk backwards looking
        // for the first separator. bash_cur is everything after the
        // last separator, possibly empty.
        char sep(0);
        while (--p > 0)
        {
            char ch = bash_line.at(p);
            if ((ch == ' ') || (ch == '=') || (ch == ':'))
            {
                sep = ch;
                break;
            }
        }
        bash_cur = bash_line.substr(1+p, std::string::npos);
        if ((sep == ':') || (sep == '='))
        {
            // Bash sets prev to the non-space separator if any.
            // Actually, if there are multiple separators in a row,
            // they are all included in prev, but that detail is not
            // important to us and not worth coding.
            bash_prev = bash_line.substr(p, 1);
        }
        else
        {
            // Go back to the last separator and set prev based on
            // that.
            int p1 = p;
            while (--p1 > 0)
            {
                char ch = bash_line.at(p1);
                if ((ch == ' ') || (ch == ':') || (ch == '='))
                {
                    bash_prev = bash_line.substr(p1 + 1, p - p1 - 1);
                    break;
                }
            }
        }
        if (bash_prev.empty())
        {
            bash_prev = bash_line.substr(0, p);
        }
        if (argc == 1)
        {
            // This is probably zsh using bashcompinit. There are a
            // few differences in the expected output.
            zsh_completion = true;
        }
        handleBashArguments();
        bash_completion = true;
    }
}