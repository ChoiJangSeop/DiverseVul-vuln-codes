TEST(ImmutableConstantOpTest, FromFile) {
  const TensorShape kFileTensorShape({1000, 1});
  Env* env = Env::Default();
  auto root = Scope::NewRootScope().ExitOnError();

  string two_file, three_file;
  TF_ASSERT_OK(CreateTempFile(env, 2.0f, 1000, &two_file));
  TF_ASSERT_OK(CreateTempFile(env, 3.0f, 1000, &three_file));
  auto node1 = ops::ImmutableConst(root, DT_FLOAT, kFileTensorShape, two_file);
  auto node2 =
      ops::ImmutableConst(root, DT_FLOAT, kFileTensorShape, three_file);
  auto result = ops::MatMul(root, node1, node2, ops::MatMul::TransposeB(true));

  GraphDef graph_def;
  TF_ASSERT_OK(root.ToGraphDef(&graph_def));
  SessionOptions session_options;
  session_options.config.mutable_graph_options()
      ->mutable_optimizer_options()
      ->set_opt_level(OptimizerOptions::L0);
  std::unique_ptr<Session> session(NewSession(session_options));
  ASSERT_TRUE(session != nullptr) << "Failed to create session";
  TF_ASSERT_OK(session->Create(graph_def)) << "Can't create test graph";
  std::vector<Tensor> outputs;
  TF_ASSERT_OK(session->Run({}, {result.node()->name() + ":0"}, {}, &outputs));
  ASSERT_EQ(outputs.size(), 1);
  EXPECT_EQ(outputs.front().flat<float>()(0), 2.0f * 3.0f);
  EXPECT_EQ(outputs.front().flat<float>()(1), 2.0f * 3.0f);
  EXPECT_EQ(outputs.front().flat<float>()(2), 2.0f * 3.0f);
}