std::unique_ptr<BytecodeModule> BytecodeModuleGenerator::generate() {
  assert(
      valid_ &&
      "BytecodeModuleGenerator::generate() cannot be called more than once");
  valid_ = false;

  assert(
      functionIDMap_.getElements().size() == functionGenerators_.size() &&
      "Missing functions.");

  auto kinds = stringTable_.getStringKinds();
  auto hashes = stringTable_.getIdentifierHashes();

  BytecodeOptions bytecodeOptions;
  bytecodeOptions.staticBuiltins = options_.staticBuiltinsEnabled;
  bytecodeOptions.cjsModulesStaticallyResolved = !cjsModulesStatic_.empty();
  std::unique_ptr<BytecodeModule> BM{new BytecodeModule(
      functionGenerators_.size(),
      std::move(kinds),
      std::move(hashes),
      stringTable_.acquireStringTable(),
      stringTable_.acquireStringStorage(),
      regExpTable_.getEntryList(),
      regExpTable_.getBytecodeBuffer(),
      entryPointIndex_,
      std::move(arrayBuffer_),
      std::move(objKeyBuffer_),
      std::move(objValBuffer_),
      cjsModuleOffset_,
      std::move(cjsModules_),
      std::move(cjsModulesStatic_),
      bytecodeOptions)};

  DebugInfoGenerator debugInfoGen{std::move(filenameTable_)};

  const uint32_t strippedFunctionNameId =
      options_.stripFunctionNames ? getStringID(kStrippedFunctionName) : 0;
  auto functions = functionIDMap_.getElements();
  std::shared_ptr<Context> contextIfNeeded;
  for (unsigned i = 0, e = functions.size(); i < e; ++i) {
    auto *F = functions[i];
    auto &BFG = *functionGenerators_[F];

    uint32_t functionNameId = options_.stripFunctionNames
        ? strippedFunctionNameId
        : getStringID(functions[i]->getOriginalOrInferredName().str());

    std::unique_ptr<BytecodeFunction> func = BFG.generateBytecodeFunction(
        F->getDefinitionKind(),
        F->getKind(),
        F->isStrictMode(),
        F->getExpectedParamCountIncludingThis(),
        F->getFunctionScope()->getVariables().size(),
        functionNameId);

#ifndef HERMESVM_LEAN
    if (F->getParent()
            ->shareContext()
            ->allowFunctionToStringWithRuntimeSource() ||
        F->isLazy()) {
      auto context = F->getParent()->shareContext();
      assert(
          (!contextIfNeeded || contextIfNeeded.get() == context.get()) &&
          "Different instances of Context seen");
      contextIfNeeded = context;
      BM->setFunctionSourceRange(i, F->getSourceRange());
    }
#endif

    if (F->isLazy()) {
#ifdef HERMESVM_LEAN
      llvm_unreachable("Lazy support compiled out");
#else
      auto lazyData = llvh::make_unique<LazyCompilationData>();
      lazyData->parentScope = F->getLazyScope();
      lazyData->nodeKind = F->getLazySource().nodeKind;
      lazyData->bufferId = F->getLazySource().bufferId;
      lazyData->originalName = F->getOriginalOrInferredName();
      lazyData->closureAlias = F->getLazyClosureAlias()
          ? F->getLazyClosureAlias()->getName()
          : Identifier();
      lazyData->strictMode = F->isStrictMode();
      func->setLazyCompilationData(std::move(lazyData));
#endif
    }

    if (BFG.hasDebugInfo()) {
      uint32_t sourceLocOffset = debugInfoGen.appendSourceLocations(
          BFG.getSourceLocation(), i, BFG.getDebugLocations());
      uint32_t lexicalDataOffset = debugInfoGen.appendLexicalData(
          BFG.getLexicalParentID(), BFG.getDebugVariableNames());
      func->setDebugOffsets({sourceLocOffset, lexicalDataOffset});
    }
    BM->setFunction(i, std::move(func));
  }

  BM->setContext(contextIfNeeded);

  BM->setDebugInfo(debugInfoGen.serializeWithMove());
  return BM;
}