bool SNC_io_parser<EW>::
read_sface(SFace_handle sfh) {

  bool OK = true;
  int index;
  char cc;

  in >> index;
  OK = OK && test_string("{");

  in >> index;
  sfh->center_vertex() = Vertex_of[index];
  OK = OK && test_string(",");

  in >> cc;
  while(isdigit(cc)) {
    in.putback(cc);
    in >> index;
    //    sfh->boundary_entry_objects().push_back(SEdge_of[index]);
    SM_decorator SD(&*sfh->center_vertex());
    SD.link_as_face_cycle(SEdge_of[index],sfh);
    in >> cc;
  }

  in >> cc;
  while(isdigit(cc)) {
    in.putback(cc);
    in >> index;
    sfh->boundary_entry_objects().push_back(make_object(Edge_of[index]));
    this->sncp()->store_sm_boundary_item(Edge_of[index], --(sfh->sface_cycles_end()));
    in >> cc;
  }

  in >> cc;
  while(isdigit(cc)) {
    in.putback(cc);
    in >> index;
    sfh->boundary_entry_objects().push_back(make_object(SLoop_of[index]));
    this->sncp()->store_sm_boundary_item(SLoop_of[index], --(sfh->sface_cycles_end()));
    in >> cc;
  }

  in >> index;
  sfh->volume() = Volume_of[index+addInfiBox];
  OK = OK && test_string("}");
  in >> sfh->mark();

  return OK;
}