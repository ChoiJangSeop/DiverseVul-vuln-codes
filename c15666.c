Function *ESTreeIRGen::genGeneratorFunction(
    Identifier originalName,
    Variable *lazyClosureAlias,
    ESTree::FunctionLikeNode *functionNode) {
  assert(functionNode && "Function AST cannot be null");

  // Build the outer function which creates the generator.
  // Does not have an associated source range.
  auto *outerFn = Builder.createGeneratorFunction(
      originalName,
      Function::DefinitionKind::ES5Function,
      ESTree::isStrict(functionNode->strictness),
      /* insertBefore */ nullptr);

  auto *innerFn = genES5Function(
      genAnonymousLabelName(originalName.isValid() ? originalName.str() : ""),
      lazyClosureAlias,
      functionNode,
      true);

  {
    FunctionContext outerFnContext{this, outerFn, functionNode->getSemInfo()};
    emitFunctionPrologue(
        functionNode,
        Builder.createBasicBlock(outerFn),
        InitES5CaptureState::Yes,
        DoEmitParameters::No);

    // Create a generator function, which will store the arguments.
    auto *gen = Builder.createCreateGeneratorInst(innerFn);

    if (!hasSimpleParams(functionNode)) {
      // If there are non-simple params, step the inner function once to
      // initialize them.
      Value *next = Builder.createLoadPropertyInst(gen, "next");
      Builder.createCallInst(next, gen, {});
    }

    emitFunctionEpilogue(gen);
  }

  return outerFn;
}