lys_submodule_module_data_free(struct lys_submodule *submodule)
{
    struct lys_node *next, *elem;

    /* remove parsed data */
    LY_TREE_FOR_SAFE(submodule->belongsto->data, next, elem) {
        if (elem->module == (struct lys_module *)submodule) {
            lys_node_free(elem, NULL, 0);
        }
    }
}