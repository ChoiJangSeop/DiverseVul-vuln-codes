bool PM_io_parser<PMDEC>::read_face(Face_handle f)
{ // syntax: index { halfedge, fclist, ivlist, mark }
  int n, ei, vi; Mark m;
  if ( !(in >> n) || !check_sep("{") ) return false;
  if ( !(in >> ei) || !check_sep(",") ) return false;
  if (ei >= 0) f->set_halfedge(Halfedge_of[ei]);
  while (in >> ei) {
    CGAL_assertion_msg(ei >= 0 && (std::size_t) ei < en, "wrong index in face cycle list.");
    f->store_fc(Halfedge_of[ei]);
  } in.clear();
  if (!check_sep(",")) { return false; }
  while (in >> vi) {
    CGAL_assertion_msg(vi >= 0 && (std::size_t) vi < vn, "wrong index in iso vertex list.");
    f->store_iv(Vertex_of[vi]);
  } in.clear();
  if (!check_sep(",") || !(in >> m) || !check_sep("}") )
    return false;
  mark(f) = m;
  return true;
}