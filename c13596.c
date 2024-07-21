static uint32_t estimate_re_size(const RE *re0, std::vector<StackItem> &stack)
{
    // the estimated size of the last sub-RE visited by DFS
    uint32_t size = 0;

    const StackItem i0 = {re0, 0, 0};
    stack.push_back(i0);

    while (!stack.empty()) {
        const StackItem i = stack.back();
        stack.pop_back();

        const RE *re = i.re;
        if (re->type == RE::NIL) {
            size = 0;
        }
        else if (re->type == RE::SYM || re->type == RE::TAG) {
            size = 1;
        }
        else if (re->type == RE::ALT) {
            if (i.succ == 0) {
                // recurse into the left sub-RE
                StackItem k = {re, 0, 1};
                stack.push_back(k);
                StackItem j = {re->alt.re1, 0, 0};
                stack.push_back(j);
            }
            else if (i.succ == 1) {
                // recurse into the right sub-RE
                StackItem k = {re, size, 2};
                stack.push_back(k);
                StackItem j = {re->alt.re2, 0, 0};
                stack.push_back(j);
            }
            else {
                // both sub-RE visited, recursive return
                size = i.size // left sub-RE (saved on stack)
                    + size    // right sub-RE (just visited by DFS)
                    + 1;      // additional state for alternative
            }
        }
        else if (re->type == RE::CAT) {
            if (i.succ == 0) {
                // recurse into the left sub-RE
                StackItem k = {re, 0, 1};
                stack.push_back(k);
                StackItem j = {re->cat.re1, 0, 0};
                stack.push_back(j);
            }
            else if (i.succ == 1) {
                // recurse into the right sub-RE
                StackItem k = {re, size, 2};
                stack.push_back(k);
                StackItem j = {re->cat.re2, 0, 0};
                stack.push_back(j);
            }
            else {
                // both sub-RE visited, recursive return
                size = i.size // left sub-RE (saved on stack)
                    + size;   // right sub-RE (just visited by DFS)
            }
        }
        else if (re->type == RE::ITER) {
            if (i.succ == 0) {
                // recurse into the sub-RE
                StackItem k = {re, 0, 1};
                stack.push_back(k);
                StackItem j = {re->iter.re, 0, 0};
                stack.push_back(j);
            }
            else {
                // sub-RE visited, recursive return
                const uint32_t min = re->iter.min, max = re->iter.max;
                size = max == AST::MANY
                    ? size * min + 1
                    : size * max + (max - min);
            }
        }
    }

    DASSERT(stack.empty());
    return size;
}