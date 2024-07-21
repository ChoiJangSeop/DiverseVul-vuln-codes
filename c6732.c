int RGWListBucketMultiparts_ObjStore::get_params()
{
  delimiter = s->info.args.get("delimiter");
  prefix = s->info.args.get("prefix");
  string str = s->info.args.get("max-uploads");
  op_ret = parse_value_and_bound(str, &max_uploads, 0, g_conf()->rgw_max_listing_results, default_max);
  if (op_ret < 0) {
    return op_ret;
  }

  string key_marker = s->info.args.get("key-marker");
  string upload_id_marker = s->info.args.get("upload-id-marker");
  if (!key_marker.empty())
    marker.init(key_marker, upload_id_marker);

  return 0;
}