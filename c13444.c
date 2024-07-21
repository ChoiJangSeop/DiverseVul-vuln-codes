bool SM_io_parser<Decorator_>::read_vertex(SVertex_handle v)
{
  // precondition: nodes exist
  // syntax: index { isolated incident_object, mark, point}
  int n; bool iso; int f; Mark m; Sphere_point p;
  if ( !(in >> n) ||
       !check_sep("{") ||
       !(in >> iso) ||
       !(in >> f) ||
       !check_sep(",") ||
       !(in >> m) ||
       !check_sep(",") ||
       !(in >> p) ||
       !check_sep("}") ) return false;

  if (iso) set_face(v,SFace_of[f]);
  else     set_first_out_edge(v,Edge_of[f]);
  v->mark() = m; v->point() = p;
  return true;
}