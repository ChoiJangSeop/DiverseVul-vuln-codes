Function *ESTreeIRGen::genES5Function(
    Identifier originalName,
    Variable *lazyClosureAlias,
    ESTree::FunctionLikeNode *functionNode,
    bool isGeneratorInnerFunction) {
  assert(functionNode && "Function AST cannot be null");

  auto *body = ESTree::getBlockStatement(functionNode);
  assert(body && "body of ES5 function cannot be null");

  Function *newFunction = isGeneratorInnerFunction
      ? Builder.createGeneratorInnerFunction(
            originalName,
            Function::DefinitionKind::ES5Function,
            ESTree::isStrict(functionNode->strictness),
            functionNode->getSourceRange(),
            /* insertBefore */ nullptr)
      : Builder.createFunction(
            originalName,
            Function::DefinitionKind::ES5Function,
            ESTree::isStrict(functionNode->strictness),
            functionNode->getSourceRange(),
            /* isGlobal */ false,
            /* insertBefore */ nullptr);

  newFunction->setLazyClosureAlias(lazyClosureAlias);

  if (auto *bodyBlock = llvh::dyn_cast<ESTree::BlockStatementNode>(body)) {
    if (bodyBlock->isLazyFunctionBody) {
      // Set the AST position and variable context so we can continue later.
      newFunction->setLazyScope(saveCurrentScope());
      auto &lazySource = newFunction->getLazySource();
      lazySource.bufferId = bodyBlock->bufferId;
      lazySource.nodeKind = getLazyFunctionKind(functionNode);
      lazySource.functionRange = functionNode->getSourceRange();

      // Set the function's .length.
      newFunction->setExpectedParamCountIncludingThis(
          countExpectedArgumentsIncludingThis(functionNode));
      return newFunction;
    }
  }

  FunctionContext newFunctionContext{
      this, newFunction, functionNode->getSemInfo()};

  if (isGeneratorInnerFunction) {
    // StartGeneratorInst
    // ResumeGeneratorInst
    // at the beginning of the function, to allow for the first .next() call.
    auto *initGenBB = Builder.createBasicBlock(newFunction);
    Builder.setInsertionBlock(initGenBB);
    Builder.createStartGeneratorInst();
    auto *prologueBB = Builder.createBasicBlock(newFunction);
    auto *prologueResumeIsReturn = Builder.createAllocStackInst(
        genAnonymousLabelName("isReturn_prologue"));
    genResumeGenerator(nullptr, prologueResumeIsReturn, prologueBB);

    if (hasSimpleParams(functionNode)) {
      // If there are simple params, then we don't need an extra yield/resume.
      // They can simply be initialized on the first call to `.next`.
      Builder.setInsertionBlock(prologueBB);
      emitFunctionPrologue(
          functionNode,
          prologueBB,
          InitES5CaptureState::Yes,
          DoEmitParameters::Yes);
    } else {
      // If there are non-simple params, then we must add a new yield/resume.
      // The `.next()` call will occur once in the outer function, before
      // the iterator is returned to the caller of the `function*`.
      auto *entryPointBB = Builder.createBasicBlock(newFunction);
      auto *entryPointResumeIsReturn =
          Builder.createAllocStackInst(genAnonymousLabelName("isReturn_entry"));

      // Initialize parameters.
      Builder.setInsertionBlock(prologueBB);
      emitFunctionPrologue(
          functionNode,
          prologueBB,
          InitES5CaptureState::Yes,
          DoEmitParameters::Yes);
      Builder.createSaveAndYieldInst(
          Builder.getLiteralUndefined(), entryPointBB);

      // Actual entry point of function from the caller's perspective.
      Builder.setInsertionBlock(entryPointBB);
      genResumeGenerator(
          nullptr,
          entryPointResumeIsReturn,
          Builder.createBasicBlock(newFunction));
    }
  } else {
    emitFunctionPrologue(
        functionNode,
        Builder.createBasicBlock(newFunction),
        InitES5CaptureState::Yes,
        DoEmitParameters::Yes);
  }

  genStatement(body);
  emitFunctionEpilogue(Builder.getLiteralUndefined());

  return curFunction()->function;
}