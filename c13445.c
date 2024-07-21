bool SM_io_parser<Decorator_>::read_face(SFace_handle f)
{ // syntax: index { fclist, ivlist, loop, mark }
  int n, ei, vi, li; Mark m;
  if ( !(in >> n) || !check_sep("{") ) return false;
  while (in >> ei) {
    CGAL_assertion_msg(ei >= 0 && ei < en,
                           "wrong index in face cycle list.");
    store_sm_boundary_object(Edge_of[ei],f);
  } in.clear();
  if (!check_sep(",")) { return false; }
  while (in >> vi) {
    CGAL_assertion_msg(vi >= 0 && vi < vn,
                           "wrong index in iso vertex list.");
    store_sm_boundary_object(SVertex_of[vi],f);
  } in.clear();
  if (!check_sep(",")) { return false; }
  while (in >> li) {
    CGAL_assertion_msg(li >= 0 && li < 2,
                           "wrong index in iso vertex list.");
    store_sm_boundary_object(Loop_of[li],f);
  } in.clear();
  if (!check_sep(",") || !(in >> m) || !check_sep("}") )
    return false;
  f->mark() = m;
  return true;
}