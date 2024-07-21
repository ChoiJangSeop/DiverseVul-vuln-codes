static table_row *row_from_string(cmark_syntax_extension *self,
                                  cmark_parser *parser, unsigned char *string,
                                  int len) {
  table_row *row = NULL;
  bufsize_t cell_matched = 1, pipe_matched = 1, offset;

  row = (table_row *)parser->mem->calloc(1, sizeof(table_row));
  row->n_columns = 0;
  row->cells = NULL;

  offset = scan_table_cell_end(string, len, 0);

  // Parse the cells of the row. Stop if we reach the end of the input, or if we
  // cannot detect any more cells.
  while (offset < len && (cell_matched || pipe_matched)) {
    cell_matched = scan_table_cell(string, len, offset);
    pipe_matched = scan_table_cell_end(string, len, offset + cell_matched);

    if (cell_matched || pipe_matched) {
      cmark_strbuf *cell_buf = unescape_pipes(parser->mem, string + offset,
          cell_matched);
      cmark_strbuf_trim(cell_buf);

      node_cell *cell = (node_cell *)parser->mem->calloc(1, sizeof(*cell));
      cell->buf = cell_buf;
      cell->start_offset = offset;
      cell->end_offset = offset + cell_matched - 1;
      while (cell->start_offset > 0 && string[cell->start_offset - 1] != '|') {
        --cell->start_offset;
        ++cell->internal_offset;
      }
      row->n_columns += 1;
      row->cells = cmark_llist_append(parser->mem, row->cells, cell);
    }

    offset += cell_matched + pipe_matched;

    if (!pipe_matched) {
      pipe_matched = scan_table_row_end(string, len, offset);
      offset += pipe_matched;
    }
  }

  if (offset != len || !row->n_columns) {
    free_table_row(parser->mem, row);
    row = NULL;
  }

  return row;
}