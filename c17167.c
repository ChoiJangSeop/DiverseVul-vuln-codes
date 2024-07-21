static int ia5casecmp(const char *s1, const char *s2)
{
    return ia5ncasecmp(s1, s2, SIZE_MAX);
}