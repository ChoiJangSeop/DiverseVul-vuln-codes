int CLua::loadfile(lua_State *ls, const char *filename, bool trusted,
                   bool die_on_fail)
{
    if (!ls)
        return -1;

    if (!is_path_safe(filename, trusted))
    {
        lua_pushstring(
            ls,
            make_stringf("invalid filename: %s", filename).c_str());
        return -1;
    }

    string file = datafile_path(filename, die_on_fail);
    if (file.empty())
    {
        lua_pushstring(ls,
                       make_stringf("Can't find \"%s\"", filename).c_str());
        return -1;
    }

    FileLineInput f(file.c_str());
    string script;
    while (!f.eof())
        script += f.get_line() + "\n";

    // prefixing with @ stops lua from adding [string "%s"]
    return luaL_loadbuffer(ls, &script[0], script.length(),
                           ("@" + file).c_str());
}