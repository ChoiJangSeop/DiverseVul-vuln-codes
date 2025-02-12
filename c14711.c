static cmark_node *try_opening_table_header(cmark_syntax_extension *self,
                                            cmark_parser *parser,
                                            cmark_node *parent_container,
                                            unsigned char *input, int len) {
  bufsize_t matched =
      scan_table_start(input, len, cmark_parser_get_first_nonspace(parser));
  cmark_node *table_header;
  table_row *header_row = NULL;
  table_row *marker_row = NULL;
  node_table_row *ntr;
  const char *parent_string;
  uint16_t i;

  if (!matched)
    return parent_container;

  parent_string = cmark_node_get_string_content(parent_container);

  cmark_arena_push();

  header_row = row_from_string(self, parser, (unsigned char *)parent_string,
                               (int)strlen(parent_string));

  if (!header_row) {
    free_table_row(parser->mem, header_row);
    cmark_arena_pop();
    return parent_container;
  }

  marker_row = row_from_string(self, parser,
                               input + cmark_parser_get_first_nonspace(parser),
                               len - cmark_parser_get_first_nonspace(parser));

  assert(marker_row);

  if (header_row->n_columns != marker_row->n_columns) {
    free_table_row(parser->mem, header_row);
    free_table_row(parser->mem, marker_row);
    cmark_arena_pop();
    return parent_container;
  }

  if (cmark_arena_pop()) {
    header_row = row_from_string(self, parser, (unsigned char *)parent_string,
                                 (int)strlen(parent_string));
    marker_row = row_from_string(self, parser,
                                 input + cmark_parser_get_first_nonspace(parser),
                                 len - cmark_parser_get_first_nonspace(parser));
  }

  if (!cmark_node_set_type(parent_container, CMARK_NODE_TABLE)) {
    free_table_row(parser->mem, header_row);
    free_table_row(parser->mem, marker_row);
    return parent_container;
  }

  cmark_node_set_syntax_extension(parent_container, self);

  parent_container->as.opaque = parser->mem->calloc(1, sizeof(node_table));

  set_n_table_columns(parent_container, header_row->n_columns);

  uint8_t *alignments =
      (uint8_t *)parser->mem->calloc(header_row->n_columns, sizeof(uint8_t));
  cmark_llist *it = marker_row->cells;
  for (i = 0; it; it = it->next, ++i) {
    node_cell *node = (node_cell *)it->data;
    bool left = node->buf->ptr[0] == ':', right = node->buf->ptr[node->buf->size - 1] == ':';

    if (left && right)
      alignments[i] = 'c';
    else if (left)
      alignments[i] = 'l';
    else if (right)
      alignments[i] = 'r';
  }
  set_table_alignments(parent_container, alignments);

  table_header =
      cmark_parser_add_child(parser, parent_container, CMARK_NODE_TABLE_ROW,
                             parent_container->start_column);
  cmark_node_set_syntax_extension(table_header, self);
  table_header->end_column = parent_container->start_column + (int)strlen(parent_string) - 2;
  table_header->start_line = table_header->end_line = parent_container->start_line;

  table_header->as.opaque = ntr = (node_table_row *)parser->mem->calloc(1, sizeof(node_table_row));
  ntr->is_header = true;

  {
    cmark_llist *tmp;

    for (tmp = header_row->cells; tmp; tmp = tmp->next) {
      node_cell *cell = (node_cell *) tmp->data;
      cmark_node *header_cell = cmark_parser_add_child(parser, table_header,
          CMARK_NODE_TABLE_CELL, parent_container->start_column + cell->start_offset);
      header_cell->start_line = header_cell->end_line = parent_container->start_line;
      header_cell->internal_offset = cell->internal_offset;
      header_cell->end_column = parent_container->start_column + cell->end_offset;
      cmark_node_set_string_content(header_cell, (char *) cell->buf->ptr);
      cmark_node_set_syntax_extension(header_cell, self);
    }
  }

  cmark_parser_advance_offset(
      parser, (char *)input,
      (int)strlen((char *)input) - 1 - cmark_parser_get_offset(parser), false);

  free_table_row(parser->mem, header_row);
  free_table_row(parser->mem, marker_row);
  return parent_container;
}