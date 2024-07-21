static cmark_node *try_opening_table_row(cmark_syntax_extension *self,
                                         cmark_parser *parser,
                                         cmark_node *parent_container,
                                         unsigned char *input, int len) {
  cmark_node *table_row_block;
  table_row *row;

  if (cmark_parser_is_blank(parser))
    return NULL;

  table_row_block =
      cmark_parser_add_child(parser, parent_container, CMARK_NODE_TABLE_ROW,
                             parent_container->start_column);
  cmark_node_set_syntax_extension(table_row_block, self);
  table_row_block->end_column = parent_container->end_column;
  table_row_block->as.opaque = parser->mem->calloc(1, sizeof(node_table_row));

  row = row_from_string(self, parser, input + cmark_parser_get_first_nonspace(parser),
      len - cmark_parser_get_first_nonspace(parser));

  {
    cmark_llist *tmp;
    int i, table_columns = get_n_table_columns(parent_container);

    for (tmp = row->cells, i = 0; tmp && i < table_columns; tmp = tmp->next, ++i) {
      node_cell *cell = (node_cell *) tmp->data;
      cmark_node *node = cmark_parser_add_child(parser, table_row_block,
          CMARK_NODE_TABLE_CELL, parent_container->start_column + cell->start_offset);
      node->internal_offset = cell->internal_offset;
      node->end_column = parent_container->start_column + cell->end_offset;
      cmark_node_set_string_content(node, (char *) cell->buf->ptr);
      cmark_node_set_syntax_extension(node, self);
    }

    for (; i < table_columns; ++i) {
      cmark_node *node = cmark_parser_add_child(
          parser, table_row_block, CMARK_NODE_TABLE_CELL, 0);
      cmark_node_set_syntax_extension(node, self);
    }
  }

  free_table_row(parser->mem, row);

  cmark_parser_advance_offset(parser, (char *)input,
                              len - 1 - cmark_parser_get_offset(parser), false);

  return table_row_block;
}