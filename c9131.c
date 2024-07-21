TEST_P(ClusterMemoryTestRunner, MemoryLargeClusterSizeWithRealSymbolTable) {
  symbol_table_creator_test_peer_.setUseFakeSymbolTables(false);

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
  // 2019/08/09  7882     35489       36000   Initial version
  // 2019/09/02  8118     34585       34500   Share symbol-tables in cluster/host stats.
  // 2019/09/16  8100     34585       34500   Add transport socket matcher in cluster.
  // 2019/09/25  8226     34777       35000   dns: enable dns failure refresh rate configuration
  // 2019/09/30  8354     34969       35000   Implement transport socket match.
  // 2019/10/17  8537     34966       35000   add new enum value HTTP3
  // 2019/10/17  8484     34998       35000   stats: add unit support to histogram
  // 2019/11/01  8859     35221       36000   build: switch to libc++ by default
  // 2019/11/15  9040     35029       35500   build: update protobuf to 3.10.1
  // 2019/11/15  9031     35061       35500   upstream: track whether cluster is local
  // 2019/12/10  8779     35053       35000   use var-length coding for name lengths
  // 2020/01/07  9069     35548       35700   upstream: Implement retry concurrency budgets
  // 2020/01/07  9564     35580       36000   RefcountPtr for CentralCache.
  // 2020/01/09  8889     35644       36000   api: add UpstreamHttpProtocolOptions message
  // 2019/01/09  9227     35772       36500   router: per-cluster histograms w/ timeout budget
  // 2020/01/12  9633     35932       36500   config: support recovery of original message when
  //                                          upgrading.
  // 2020/03/16  9964     36220       36800   http2: support custom SETTINGS parameters.
  // 2020/03/24  10501    36300       36800   upstream: upstream_rq_retry_limit_exceeded.
  // 2020/04/02  10624    35564       36000   Use 100 clusters rather than 1000 to avoid timeouts
  // 2020/04/07  10661    35557       36000   fix clang tidy on master
  // 2020/04/23  10531    36281       36800   http: max stream duration upstream support.
  // 2020/05/05  10908    36345       36800   router: add InternalRedirectPolicy and predicate
  // 2020/05/13  10531    36537       36800   Refactor resource manager
  // 2020/05/20  11223    36603       36800   Add primary clusters tracking to cluster manager.
  // 2020/06/10  11561    36603       36923   Make upstreams pluggable

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
  EXPECT_MEMORY_EQ(m_per_cluster, 36603);
  EXPECT_MEMORY_LE(m_per_cluster, 38000); // Round up to allow platform variations.
}