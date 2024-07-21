bool PM_io_parser<PMDEC>::read_vertex(Vertex_handle v)
{
  // precondition: nodes exist
  // syntax: index { mark, point, isolated object }
  int n; bool iso; int f; Mark m; Point p;
  if ( !(in >> n) ||
       !check_sep("{") ||
       !(in >> iso) ||
       !(in >> f) ||
       !check_sep(",") ||
       !(in >> m) ||
       !check_sep(",") ||
       !(in >> p) ||
       !check_sep("}") ) return false;

  if (iso) v->set_face(Face_of[f]);
  else     v->set_halfedge(Halfedge_of[f]);
  mark(v) = m; point(v) = p;
  return true;
}