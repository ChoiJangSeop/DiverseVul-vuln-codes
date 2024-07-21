NO_INLINE JsVar *jspeStatementFor() {
  JSP_ASSERT_MATCH(LEX_R_FOR);
  JSP_MATCH('(');
  bool wasInLoop = (execInfo.execute&EXEC_IN_LOOP)!=0;
  execInfo.execute |= EXEC_FOR_INIT;
  // initialisation
  JsVar *forStatement = 0;
  // we could have 'for (;;)' - so don't munch up our semicolon if that's all we have
  if (lex->tk != ';')
    forStatement = jspeStatement();
  if (jspIsInterrupted()) {
    jsvUnLock(forStatement);
    return 0;
  }
  execInfo.execute &= (JsExecFlags)~EXEC_FOR_INIT;
  if (lex->tk == LEX_R_IN) {
    // for (i in array)
    // where i = jsvUnLock(forStatement);
    if (JSP_SHOULD_EXECUTE && !jsvIsName(forStatement)) {
      jsvUnLock(forStatement);
      jsExceptionHere(JSET_ERROR, "FOR a IN b - 'a' must be a variable name, not %t", forStatement);
      return 0;
    }
    bool addedIteratorToScope = false;
    if (JSP_SHOULD_EXECUTE && !jsvGetRefs(forStatement)) {
      // if the variable did not exist, add it to the scope
      addedIteratorToScope = true;
      jsvAddName(execInfo.root, forStatement);
    }
    JSP_MATCH_WITH_CLEANUP_AND_RETURN(LEX_R_IN, jsvUnLock(forStatement), 0);
    JsVar *array = jsvSkipNameAndUnLock(jspeExpression());
    JSP_MATCH_WITH_CLEANUP_AND_RETURN(')', jsvUnLock2(forStatement, array), 0);
    JslCharPos forBodyStart = jslCharPosClone(&lex->tokenStart);
    JSP_SAVE_EXECUTE();
    jspSetNoExecute();
    execInfo.execute |= EXEC_IN_LOOP;
    jsvUnLock(jspeBlockOrStatement());
    JslCharPos forBodyEnd = jslCharPosClone(&lex->tokenStart);
    if (!wasInLoop) execInfo.execute &= (JsExecFlags)~EXEC_IN_LOOP;
    JSP_RESTORE_EXECUTE();

    if (JSP_SHOULD_EXECUTE) {
      if (jsvIsIterable(array)) {
        JsvIsInternalChecker checkerFunction = jsvGetInternalFunctionCheckerFor(array);
        JsVar *foundPrototype = 0;

        JsvIterator it;
        jsvIteratorNew(&it, array, JSIF_DEFINED_ARRAY_ElEMENTS);
        bool hasHadBreak = false;
        while (JSP_SHOULD_EXECUTE && jsvIteratorHasElement(&it) && !hasHadBreak) {
          JsVar *loopIndexVar = jsvIteratorGetKey(&it);
          bool ignore = false;
          if (checkerFunction && checkerFunction(loopIndexVar)) {
            ignore = true;
            if (jsvIsString(loopIndexVar) &&
                jsvIsStringEqual(loopIndexVar, JSPARSE_INHERITS_VAR))
              foundPrototype = jsvSkipName(loopIndexVar);
          }
          if (!ignore) {
            JsVar *indexValue = jsvIsName(loopIndexVar) ?
                jsvCopyNameOnly(loopIndexVar, false/*no copy children*/, false/*not a name*/) :
                loopIndexVar;
            if (indexValue) { // could be out of memory
              assert(!jsvIsName(indexValue) && jsvGetRefs(indexValue)==0);
              jsvSetValueOfName(forStatement, indexValue);
              if (indexValue!=loopIndexVar) jsvUnLock(indexValue);

              jsvIteratorNext(&it);

              jslSeekToP(&forBodyStart);
              execInfo.execute |= EXEC_IN_LOOP;
              jspDebuggerLoopIfCtrlC();
              jsvUnLock(jspeBlockOrStatement());
              if (!wasInLoop) execInfo.execute &= (JsExecFlags)~EXEC_IN_LOOP;

              if (execInfo.execute & EXEC_CONTINUE)
                execInfo.execute = EXEC_YES;
              else if (execInfo.execute & EXEC_BREAK) {
                execInfo.execute = EXEC_YES;
                hasHadBreak = true;
              }
            }
          } else
            jsvIteratorNext(&it);
          jsvUnLock(loopIndexVar);

          if (!jsvIteratorHasElement(&it) && foundPrototype) {
            jsvIteratorFree(&it);
            jsvIteratorNew(&it, foundPrototype, JSIF_DEFINED_ARRAY_ElEMENTS);
            jsvUnLock(foundPrototype);
            foundPrototype = 0;
          }
        }
        assert(!foundPrototype);
        jsvIteratorFree(&it);
      } else if (!jsvIsUndefined(array)) {
        jsExceptionHere(JSET_ERROR, "FOR loop can only iterate over Arrays, Strings or Objects, not %t", array);
      }
    }
    jslSeekToP(&forBodyEnd);
    jslCharPosFree(&forBodyStart);
    jslCharPosFree(&forBodyEnd);

    if (addedIteratorToScope) {
      jsvRemoveChild(execInfo.root, forStatement);
    }
    jsvUnLock2(forStatement, array);
  } else { // ----------------------------------------------- NORMAL FOR LOOP
#ifdef JSPARSE_MAX_LOOP_ITERATIONS
    int loopCount = JSPARSE_MAX_LOOP_ITERATIONS;
#endif
    bool loopCond = true;
    bool hasHadBreak = false;

    jsvUnLock(forStatement);
    JSP_MATCH(';');
    JslCharPos forCondStart = jslCharPosClone(&lex->tokenStart);
    if (lex->tk != ';') {
      JsVar *cond = jspeAssignmentExpression(); // condition
      loopCond = JSP_SHOULD_EXECUTE && jsvGetBoolAndUnLock(jsvSkipName(cond));
      jsvUnLock(cond);
    }
    JSP_MATCH_WITH_CLEANUP_AND_RETURN(';',jslCharPosFree(&forCondStart);,0);
    JslCharPos forIterStart = jslCharPosClone(&lex->tokenStart);
    if (lex->tk != ')')  { // we could have 'for (;;)'
      JSP_SAVE_EXECUTE();
      jspSetNoExecute();
      jsvUnLock(jspeExpression()); // iterator
      JSP_RESTORE_EXECUTE();
    }
    JSP_MATCH_WITH_CLEANUP_AND_RETURN(')',jslCharPosFree(&forCondStart);jslCharPosFree(&forIterStart);,0);

    JslCharPos forBodyStart = jslCharPosClone(&lex->tokenStart); // actual for body
    JSP_SAVE_EXECUTE();
    if (!loopCond) jspSetNoExecute();
    execInfo.execute |= EXEC_IN_LOOP;
    jsvUnLock(jspeBlockOrStatement());
    JslCharPos forBodyEnd = jslCharPosClone(&lex->tokenStart);
    if (!wasInLoop) execInfo.execute &= (JsExecFlags)~EXEC_IN_LOOP;
    if (loopCond || !JSP_SHOULD_EXECUTE) {
      if (execInfo.execute & EXEC_CONTINUE)
        execInfo.execute = EXEC_YES;
      else if (execInfo.execute & EXEC_BREAK) {
        execInfo.execute = EXEC_YES;
        hasHadBreak = true;
      }
    }
    if (!loopCond) JSP_RESTORE_EXECUTE();
    if (loopCond) {
      jslSeekToP(&forIterStart);
      if (lex->tk != ')') jsvUnLock(jspeExpression());
    }
    while (!hasHadBreak && JSP_SHOULD_EXECUTE && loopCond
#ifdef JSPARSE_MAX_LOOP_ITERATIONS
        && loopCount-->0
#endif
    ) {
      jslSeekToP(&forCondStart);
      ;
      if (lex->tk == ';') {
        loopCond = true;
      } else {
        JsVar *cond = jspeAssignmentExpression();
        loopCond = jsvGetBoolAndUnLock(jsvSkipName(cond));
        jsvUnLock(cond);
      }
      if (JSP_SHOULD_EXECUTE && loopCond) {
        jslSeekToP(&forBodyStart);
        execInfo.execute |= EXEC_IN_LOOP;
        jspDebuggerLoopIfCtrlC();
        jsvUnLock(jspeBlockOrStatement());
        if (!wasInLoop) execInfo.execute &= (JsExecFlags)~EXEC_IN_LOOP;
        if (execInfo.execute & EXEC_CONTINUE)
          execInfo.execute = EXEC_YES;
        else if (execInfo.execute & EXEC_BREAK) {
          execInfo.execute = EXEC_YES;
          hasHadBreak = true;
        }
      }
      if (JSP_SHOULD_EXECUTE && loopCond && !hasHadBreak) {
        jslSeekToP(&forIterStart);
        if (lex->tk != ')') jsvUnLock(jspeExpression());
      }
    }
    jslSeekToP(&forBodyEnd);

    jslCharPosFree(&forCondStart);
    jslCharPosFree(&forIterStart);
    jslCharPosFree(&forBodyStart);
    jslCharPosFree(&forBodyEnd);

#ifdef JSPARSE_MAX_LOOP_ITERATIONS
    if (loopCount<=0) {
      jsExceptionHere(JSET_ERROR, "FOR Loop exceeded the maximum number of iterations ("STRINGIFY(JSPARSE_MAX_LOOP_ITERATIONS)")");
    }
#endif
  }
  return 0;
}