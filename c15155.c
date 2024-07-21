ArgParser::handleBashArguments()
{
    // Do a minimal job of parsing bash_line into arguments. This
    // doesn't do everything the shell does (e.g. $(...), variable
    // expansion, arithmetic, globs, etc.), but it should be good
    // enough for purposes of handling completion. As we build up the
    // new argv, we can't use this->new_argv because this code has to
    // interoperate with @file arguments, so memory for both ways of
    // fabricating argv has to be protected.

    bool last_was_backslash = false;
    enum { st_top, st_squote, st_dquote } state = st_top;
    std::string arg;
    for (std::string::iterator iter = bash_line.begin();
         iter != bash_line.end(); ++iter)
    {
        char ch = (*iter);
        if (last_was_backslash)
        {
            arg.append(1, ch);
            last_was_backslash = false;
        }
        else if (ch == '\\')
        {
            last_was_backslash = true;
        }
        else
        {
            bool append = false;
            switch (state)
            {
              case st_top:
                if (QUtil::is_space(ch))
                {
                    if (! arg.empty())
                    {
                        bash_argv.push_back(
                            PointerHolder<char>(
                                true, QUtil::copy_string(arg.c_str())));
                        arg.clear();
                    }
                }
                else if (ch == '"')
                {
                    state = st_dquote;
                }
                else if (ch == '\'')
                {
                    state = st_squote;
                }
                else
                {
                    append = true;
                }
                break;

              case st_squote:
                if (ch == '\'')
                {
                    state = st_top;
                }
                else
                {
                    append = true;
                }
                break;

              case st_dquote:
                if (ch == '"')
                {
                    state = st_top;
                }
                else
                {
                    append = true;
                }
                break;
            }
            if (append)
            {
                arg.append(1, ch);
            }
        }
    }
    if (bash_argv.empty())
    {
        // This can't happen if properly invoked by bash, but ensure
        // we have a valid argv[0] regardless.
        bash_argv.push_back(
            PointerHolder<char>(
                true, QUtil::copy_string(argv[0])));
    }
    // Explicitly discard any non-space-terminated word. The "current
    // word" is handled specially.
    bash_argv_ph = PointerHolder<char*>(true, new char*[1+bash_argv.size()]);
    argv = bash_argv_ph.getPointer();
    for (size_t i = 0; i < bash_argv.size(); ++i)
    {
        argv[i] = bash_argv.at(i).getPointer();
    }
    argc = static_cast<int>(bash_argv.size());
    argv[argc] = 0;
}