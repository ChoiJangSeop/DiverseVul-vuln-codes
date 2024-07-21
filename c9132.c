TEST_P(ClusterMemoryTestRunner, MemoryLargeClusterSizeWithFakeSymbolTable) {
  symbol_table_creator_test_peer_.setUseFakeSymbolTables(true);

  // A unique instance of ClusterMemoryTest allows for multiple runs of Envoy with
  // differing configuration. This is necessary for measuring the memory consumption
  // between the different instances within the same test.
  const size_t m100 = ClusterMemoryTestHelper::computeMemoryDelta(1, 0, 101, 0, true);
  const size_t m_per_cluster = (m100) / 100;

  // Note: if you are increasing this golden value because you are adding a
  // stat, please confirm that this will be generally useful to most Envoy
  // users. Otherwise you are adding to the per-cluster memory overhead, which
  // will be significant for Envoy installations that are massively
  // multi-tenant.
  //
  // History of golden values:
  //
  // Date        PR       Bytes Per Cluster   Notes
  //                      exact upper-bound
  // ----------  -----    -----------------   -----
  // 2019/03/20  6329     59015               Initial version
  // 2019/04/12  6477     59576               Implementing Endpoint lease...
  // 2019/04/23  6659     59512               Reintroduce dispatcher stats...
  // 2019/04/24  6161     49415               Pack tags and tag extracted names
  // 2019/05/07  6794     49957               Stats for excluded hosts in cluster
  // 2019/04/27  6733     50213               Use SymbolTable API for HTTP codes
  // 2019/05/31  6866     50157               libstdc++ upgrade in CI
  // 2019/06/03  7199     49393               absl update
  // 2019/06/06  7208     49650               make memory targets approximate
  // 2019/06/17  7243     49412       49700   macros for exact/upper-bound memory checks
  // 2019/06/29  7364     45685       46000   combine 2 levels of stat ref-counting into 1
  // 2019/06/30  7428     42742       43000   remove stats multiple inheritance, inline HeapStatData
  // 2019/07/06  7477     42742       43000   fork gauge representation to drop pending_increment_
  // 2019/07/15  7555     42806       43000   static link libstdc++ in tests
  // 2019/07/24  7503     43030       44000   add upstream filters to clusters
  // 2019/08/13  7877     42838       44000   skip EdfScheduler creation if all host weights equal
  // 2019/09/02  8118     42830       43000   Share symbol-tables in cluster/host stats.
  // 2019/09/16  8100     42894       43000   Add transport socket matcher in cluster.
  // 2019/09/25  8226     43022       44000   dns: enable dns failure refresh rate configuration
  // 2019/09/30  8354     43310       44000   Implement transport socket match.
  // 2019/10/17  8537     43308       44000   add new enum value HTTP3
  // 2019/10/17  8484     43340       44000   stats: add unit support to histogram
  // 2019/11/01  8859     43563       44000   build: switch to libc++ by default
  // 2019/11/15  9040     43371       44000   build: update protobuf to 3.10.1
  // 2019/11/15  9031     43403       44000   upstream: track whether cluster is local
  // 2019/12/10  8779     42919       43500   use var-length coding for name length
  // 2020/01/07  9069     43413       44000   upstream: Implement retry concurrency budgets
  // 2020/01/07  9564     43445       44000   use RefcountPtr for CentralCache.
  // 2020/01/09  8889     43509       44000   api: add UpstreamHttpProtocolOptions message
  // 2020/01/09  9227     43637       44000   router: per-cluster histograms w/ timeout budget
  // 2020/01/12  9633     43797       44104   config: support recovery of original message when
  //                                          upgrading.
  // 2020/02/13  10042    43797       44136   Metadata: Metadata are shared across different
  //                                          clusters and hosts.
  // 2020/03/16  9964     44085       44600   http2: support custom SETTINGS parameters.
  // 2020/03/24  10501    44261       44600   upstream: upstream_rq_retry_limit_exceeded.
  // 2020/04/02  10624    43356       44000   Use 100 clusters rather than 1000 to avoid timeouts
  // 2020/04/07  10661    43349       44000   fix clang tidy on master
  // 2020/04/23  10531    44169       44600   http: max stream duration upstream support.
  // 2020/05/05  10908    44233       44600   router: add InternalRedirectPolicy and predicate
  // 2020/05/13  10531    44425       44600   Refactor resource manager
  // 2020/05/20  11223    44491       44600   Add primary clusters tracking to cluster manager.
  // 2020/06/10  11561    44491       44811   Make upstreams pluggable

  // Note: when adjusting this value: EXPECT_MEMORY_EQ is active only in CI
  // 'release' builds, where we control the platform and tool-chain. So you
  // will need to find the correct value only after failing CI and looking
  // at the logs.
  //
  // On a local clang8/libstdc++/linux flow, the memory usage was observed in
  // June 2019 to be 64 bytes higher than it is in CI/release. Your mileage may
  // vary.
  //
  // If you encounter a failure here, please see
  // https://github.com/envoyproxy/envoy/blob/master/source/docs/stats.md#stats-memory-tests
  // for details on how to fix.
  EXPECT_MEMORY_EQ(m_per_cluster, 44491);
  EXPECT_MEMORY_LE(m_per_cluster, 46000); // Round up to allow platform variations.
}