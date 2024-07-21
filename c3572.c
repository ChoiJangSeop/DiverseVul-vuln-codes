string_compare_by_pointer(const uschar **a, const uschar **b)
{
return Ustrcmp(CUS *a, CUS *b);
}