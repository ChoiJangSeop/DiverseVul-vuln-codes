size_t estimate_size(const std::vector<RE*> &res)
{
    std::vector<StackItem> stack;

    const size_t nre = res.size();
    DASSERT(nre > 0);
    size_t size = nre - 1;

    for (size_t i = 0; i < nre; ++i) {
        size += estimate_re_size(res[i], stack) + 1;
    }

    return size;
}