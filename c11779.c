static void new_huffman_tree(x3f_hufftree_t *HTP, int bits)
{
  int leaves = 1 << bits;

  HTP->free_node_index = 0;
  HTP->nodes = (x3f_huffnode_t *)calloc(1, HUF_TREE_MAX_NODES(leaves) *
                                               sizeof(x3f_huffnode_t));
}