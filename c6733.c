int RGWListBucket::parse_max_keys()
{
  // Bound max value of max-keys to configured value for security
  // Bound min value of max-keys to '0'
  // Some S3 clients explicitly send max-keys=0 to detect if the bucket is
  // empty without listing any items.
  op_ret = parse_value_and_bound(max_keys, &max, 0, g_conf()->rgw_max_listing_results, default_max);
}