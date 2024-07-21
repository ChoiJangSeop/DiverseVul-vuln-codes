bool Item_subselect::exec()
{
  subselect_engine *org_engine= engine;

  DBUG_ENTER("Item_subselect::exec");
  DBUG_ASSERT(fixed);

  /*
    Do not execute subselect in case of a fatal error
    or if the query has been killed.
  */
  if (thd->is_error() || thd->killed)
    DBUG_RETURN(true);

  DBUG_ASSERT(!thd->lex->context_analysis_only);
  /*
    Simulate a failure in sub-query execution. Used to test e.g.
    out of memory or query being killed conditions.
  */
  DBUG_EXECUTE_IF("subselect_exec_fail", DBUG_RETURN(true););

  bool res= engine->exec();

#ifndef DBUG_OFF
  ++exec_counter;
#endif
  if (engine != org_engine)
  {
    /*
      If the subquery engine changed during execution due to lazy subquery
      optimization, or because the original engine found a more efficient other
      engine, re-execute the subquery with the new engine.
    */
    DBUG_RETURN(exec());
  }
  DBUG_RETURN(res);
}