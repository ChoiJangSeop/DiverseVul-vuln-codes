bool SM_io_parser<Decorator_>::read_loop(SHalfloop_handle l)
{ // syntax: index { twin, face, mark, circle }
  int n, lo, f; bool m; Sphere_circle k;
  if ( !(in >> n) ||
       !check_sep("{") ||
       !(in >> lo) || !check_sep(",") ||
       !(in >> f) || !check_sep(",") ||
       !(in >> m) || !check_sep(",") ||
       !(in >> k) || !check_sep("}") )
    return false;
  CGAL_assertion_msg(
    (lo >= 0 && lo < 2 && f >= 0 && f < fn),"wrong index in read_edge");

  set_face(l,SFace_of[f]);
  l->mark() = m;
  l->circle() = k;
  return true;
}