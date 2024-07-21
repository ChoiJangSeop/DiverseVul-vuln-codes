void SM_io_parser<Decorator_>::read()
{
  if ( !check_sep("Sphere_map_2") )
    CGAL_error_msg("SM_io_parser::read: no embedded_PM header.");
  if ( !(check_sep("vertices") && (in >> vn)) )
    CGAL_error_msg("SM_io_parser::read: wrong vertex line.");
  if ( !(check_sep("edges") && (in >> en) && (en%2==0)) )
    CGAL_error_msg("SM_io_parser::read: wrong edge line.");
  if ( !(check_sep("loops") && (in >> ln)) )
    CGAL_error_msg("SM_io_parser::read: wrong loop line.");
  if ( !(check_sep("faces") && (in >> fn)) )
    CGAL_error_msg("SM_io_parser::read: wrong face line.");

  SVertex_of.resize(vn);
  Edge_of.resize(en);
  SFace_of.resize(fn);
  for(i=0; i<vn; i++)  SVertex_of[i] =   this->new_svertex();
  for(i=0; i<en; i++)
    if (i%2==0) Edge_of[i] = this->new_shalfedge_pair();
    else Edge_of[i] = Edge_of[i-1]->twin();
  for(i=0; i<fn; i++)  SFace_of[i] =     this->new_sface();
  if ( ln == 2 ) {
    Loop_of[0] = this->new_shalfloop_pair();
    Loop_of[1] = this->shalfloop()->twin();
  }

  for(i=0; i<vn; i++) {
    if (!read_vertex(SVertex_of[i]))
      CGAL_error_msg("SM_io_parser::read: error in node line");
  }
  for(i=0; i<en; i++) {
    if (!read_edge(Edge_of[i]))
      CGAL_error_msg("SM_io_parser::read: error in edge line");
  }
  if ( ln == 2 ) {
    read_loop(Loop_of[0]); read_loop(Loop_of[1]);
  }
  for(i=0; i<fn; i++) {
    if (!read_face(SFace_of[i]))
      CGAL_error_msg("SM_io_parser::read: error in face line");
  }
}