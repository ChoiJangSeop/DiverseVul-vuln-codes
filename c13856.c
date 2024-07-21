static bool bad_domain(const char *domain)
{
  return !strchr(domain, '.') && !strcasecompare(domain, "localhost");
}