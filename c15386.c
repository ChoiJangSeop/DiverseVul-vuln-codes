std::string addEmoji(const Proxy &node, const RegexMatchConfigs &emoji_array, extra_settings &ext)
{
    std::string real_rule, ret;

    for(const RegexMatchConfig &x : emoji_array)
    {
        if(!x.Script.empty())
        {
            std::string result;
            script_safe_runner(ext.js_runtime, ext.js_context, [&](qjs::Context &ctx)
            {
                std::string script = x.Script;
                if(startsWith(script, "path:"))
                    script = fileGet(script.substr(5), true);
                try
                {
                    ctx.eval(script);
                    auto getEmoji = (std::function<std::string(const Proxy&)>) ctx.eval("getEmoji");
                    ret = getEmoji(node);
                    if(!ret.empty())
                        result = ret + " " + node.Remark;
                }
                catch (qjs::exception)
                {
                    script_print_stack(ctx);
                }
            }, global.scriptCleanContext);
            if(!result.empty())
                return result;
            continue;
        }
        if(x.Replace.empty())
            continue;
        if(applyMatcher(x.Match, real_rule, node) && real_rule.size() && regFind(node.Remark, real_rule))
            return x.Replace + " " + node.Remark;
    }
    return node.Remark;
}