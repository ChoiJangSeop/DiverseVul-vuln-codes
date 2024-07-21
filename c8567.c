lys_getnext_data(const struct lys_module *mod, const struct lys_node *parent, const char *name, int nam_len,
                 LYS_NODE type, const struct lys_node **ret)
{
    const struct lys_node *node;

    assert((mod || parent) && name);
    assert(!(type & (LYS_AUGMENT | LYS_USES | LYS_GROUPING | LYS_CHOICE | LYS_CASE | LYS_INPUT | LYS_OUTPUT)));

    if (!mod) {
        mod = lys_node_module(parent);
    }

    /* try to find the node */
    node = NULL;
    while ((node = lys_getnext(node, parent, mod, 0))) {
        if (!type || (node->nodetype & type)) {
            /* module check */
            if (lys_node_module(node) != lys_main_module(mod)) {
                continue;
            }

            /* direct name check */
            if (!strncmp(node->name, name, nam_len) && !node->name[nam_len]) {
                if (ret) {
                    *ret = node;
                }
                return EXIT_SUCCESS;
            }
        }
    }

    return EXIT_FAILURE;
}