static bool allowed_problem_dir(const char *dir_name)
{
//HACK HACK HACK! Disabled for now until we fix clients (abrt-gui) to not pass /home/user/.cache/abrt/spool
#if 0
    unsigned len = strlen(g_settings_dump_location);

    /* If doesn't start with "g_settings_dump_location[/]"... */
    if (strncmp(dir_name, g_settings_dump_location, len) != 0
     || (dir_name[len] != '/' && dir_name[len] != '\0')
    /* or contains "/." anywhere (-> might contain ".." component) */
     || strstr(dir_name + len, "/.")
    ) {
        return false;
    }
#endif
    return true;
}