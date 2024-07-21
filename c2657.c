static gboolean is_correct_filename(const char *value)
{
    return printable_str(value) && !strchr(value, '/') && !strchr(value, '.');
}