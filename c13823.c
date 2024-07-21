NO_INLINE JsVar *jspeStatement() {
#ifdef USE_DEBUGGER
  if (execInfo.execute&EXEC_DEBUGGER_NEXT_LINE &&
      lex->tk!=';' &&
      JSP_SHOULD_EXECUTE) {
    lex->tokenLastStart = jsvStringIteratorGetIndex(&lex->tokenStart.it)-1;
    jsiDebuggerLoop();
  }
#endif
  if (lex->tk==LEX_ID ||
      lex->tk==LEX_INT ||
      lex->tk==LEX_FLOAT ||
      lex->tk==LEX_STR ||
      lex->tk==LEX_TEMPLATE_LITERAL ||
      lex->tk==LEX_REGEX ||
      lex->tk==LEX_R_NEW ||
      lex->tk==LEX_R_NULL ||
      lex->tk==LEX_R_UNDEFINED ||
      lex->tk==LEX_R_TRUE ||
      lex->tk==LEX_R_FALSE ||
      lex->tk==LEX_R_THIS ||
      lex->tk==LEX_R_DELETE ||
      lex->tk==LEX_R_TYPEOF ||
      lex->tk==LEX_R_VOID ||
      lex->tk==LEX_R_SUPER ||
      lex->tk==LEX_PLUSPLUS ||
      lex->tk==LEX_MINUSMINUS ||
      lex->tk=='!' ||
      lex->tk=='-' ||
      lex->tk=='+' ||
      lex->tk=='~' ||
      lex->tk=='[' ||
      lex->tk=='(') {
    /* Execute a simple statement that only contains basic arithmetic... */
    return jspeExpression();
  } else if (lex->tk=='{') {
    /* A block of code */
    jspeBlock();
    return 0;
  } else if (lex->tk==';') {
    /* Empty statement - to allow things like ;;; */
    JSP_ASSERT_MATCH(';');
    return 0;
  } else if (lex->tk==LEX_R_VAR ||
            lex->tk==LEX_R_LET ||
            lex->tk==LEX_R_CONST) {
    return jspeStatementVar();
  } else if (lex->tk==LEX_R_IF) {
    return jspeStatementIf();
  } else if (lex->tk==LEX_R_DO) {
    return jspeStatementDoOrWhile(false);
  } else if (lex->tk==LEX_R_WHILE) {
    return jspeStatementDoOrWhile(true);
  } else if (lex->tk==LEX_R_FOR) {
    return jspeStatementFor();
  } else if (lex->tk==LEX_R_TRY) {
    return jspeStatementTry();
  } else if (lex->tk==LEX_R_RETURN) {
    return jspeStatementReturn();
  } else if (lex->tk==LEX_R_THROW) {
    return jspeStatementThrow();
  } else if (lex->tk==LEX_R_FUNCTION) {
    return jspeStatementFunctionDecl(false/* function */);
#ifndef SAVE_ON_FLASH
  } else if (lex->tk==LEX_R_CLASS) {
      return jspeStatementFunctionDecl(true/* class */);
#endif
  } else if (lex->tk==LEX_R_CONTINUE) {
    JSP_ASSERT_MATCH(LEX_R_CONTINUE);
    if (JSP_SHOULD_EXECUTE) {
      if (!(execInfo.execute & EXEC_IN_LOOP))
        jsExceptionHere(JSET_SYNTAXERROR, "CONTINUE statement outside of FOR or WHILE loop");
      else
        execInfo.execute = (execInfo.execute & (JsExecFlags)~EXEC_RUN_MASK) | EXEC_CONTINUE;
    }
  } else if (lex->tk==LEX_R_BREAK) {
    JSP_ASSERT_MATCH(LEX_R_BREAK);
    if (JSP_SHOULD_EXECUTE) {
      if (!(execInfo.execute & (EXEC_IN_LOOP|EXEC_IN_SWITCH)))
        jsExceptionHere(JSET_SYNTAXERROR, "BREAK statement outside of SWITCH, FOR or WHILE loop");
      else
        execInfo.execute = (execInfo.execute & (JsExecFlags)~EXEC_RUN_MASK) | EXEC_BREAK;
    }
  } else if (lex->tk==LEX_R_SWITCH) {
    return jspeStatementSwitch();
  } else if (lex->tk==LEX_R_DEBUGGER) {
    JSP_ASSERT_MATCH(LEX_R_DEBUGGER);
#ifdef USE_DEBUGGER
    if (JSP_SHOULD_EXECUTE)
      jsiDebuggerLoop();
#endif
  } else JSP_MATCH(LEX_EOF);
  return 0;
}