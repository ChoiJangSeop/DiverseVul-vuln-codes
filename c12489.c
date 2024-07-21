ipcp_store_vr_results (void)
{
  cgraph_node *node;

  FOR_EACH_FUNCTION_WITH_GIMPLE_BODY (node)
    {
      ipa_node_params *info = IPA_NODE_REF (node);
      bool found_useful_result = false;

      if (!opt_for_fn (node->decl, flag_ipa_vrp))
	{
	  if (dump_file)
	    fprintf (dump_file, "Not considering %s for VR discovery "
		     "and propagate; -fipa-ipa-vrp: disabled.\n",
		     node->name ());
	  continue;
	}

      if (info->ipcp_orig_node)
	info = IPA_NODE_REF (info->ipcp_orig_node);
      if (!info->lattices)
	/* Newly expanded artificial thunks do not have lattices.  */
	continue;

      unsigned count = ipa_get_param_count (info);
      for (unsigned i = 0; i < count; i++)
	{
	  ipcp_param_lattices *plats = ipa_get_parm_lattices (info, i);
	  if (!plats->m_value_range.bottom_p ()
	      && !plats->m_value_range.top_p ())
	    {
	      found_useful_result = true;
	      break;
	    }
	}
      if (!found_useful_result)
	continue;

      ipcp_transformation_initialize ();
      ipcp_transformation *ts = ipcp_transformation_sum->get_create (node);
      vec_safe_reserve_exact (ts->m_vr, count);

      for (unsigned i = 0; i < count; i++)
	{
	  ipcp_param_lattices *plats = ipa_get_parm_lattices (info, i);
	  ipa_vr vr;

	  if (!plats->m_value_range.bottom_p ()
	      && !plats->m_value_range.top_p ())
	    {
	      vr.known = true;
	      vr.type = plats->m_value_range.m_vr.kind ();
	      vr.min = wi::to_wide (plats->m_value_range.m_vr.min ());
	      vr.max = wi::to_wide (plats->m_value_range.m_vr.max ());
	    }
	  else
	    {
	      vr.known = false;
	      vr.type = VR_VARYING;
	      vr.min = vr.max = wi::zero (INT_TYPE_SIZE);
	    }
	  ts->m_vr->quick_push (vr);
	}
    }
}