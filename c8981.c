static void nodeDestruct(struct SaveNode* node)
{
    if (node->v == &node->sorted)
    {
        tr_free(node->sorted.val.l.vals);
    }
}