create_node(Subnet *parent_subnet, int the_entry)
{
  Node *new_node;
  new_node = MallocNew(Node);
  parent_subnet->entry[the_entry] = (void *) new_node;
  clear_node(new_node);

  if (n_nodes == max_nodes) {
    if (nodes) {
      max_nodes += NODE_TABLE_INCREMENT;
      nodes = ReallocArray(Node *, max_nodes, nodes);
    } else {
      if (max_nodes != 0) {
        CROAK("max_nodes should be 0");
      }
      max_nodes = NODE_TABLE_INCREMENT;
      nodes = MallocArray(Node *, max_nodes);
    }
  }
  nodes[n_nodes++] = (Node *) new_node;
}