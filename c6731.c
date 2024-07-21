int RGWListMultipart_ObjStore::get_params()
{
  upload_id = s->info.args.get("uploadId");

  if (upload_id.empty()) {
    op_ret = -ENOTSUP;
  }
  string marker_str = s->info.args.get("part-number-marker");

  if (!marker_str.empty()) {
    string err;
    marker = strict_strtol(marker_str.c_str(), 10, &err);
    if (!err.empty()) {
      ldout(s->cct, 20) << "bad marker: "  << marker << dendl;
      op_ret = -EINVAL;
      return op_ret;
    }
  }
  
  string str = s->info.args.get("max-parts");
  op_ret = parse_value_and_bound(str, &max_parts, 0, g_conf()->rgw_max_listing_results, max_parts);

  return op_ret;
}