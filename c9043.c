void RGWCORSRule::format_exp_headers(string& s) {
  s = "";
  for(list<string>::iterator it = exposable_hdrs.begin();
      it != exposable_hdrs.end(); ++it) {
      if (s.length() > 0)
        s.append(",");
      s.append((*it));
  }
}