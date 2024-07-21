void preprocessNodes(std::vector<Proxy> &nodes, extra_settings &ext)
{
    std::for_each(nodes.begin(), nodes.end(), [&ext](Proxy &x)
    {
        if(ext.remove_emoji)
            x.Remark = trim(removeEmoji(x.Remark));

        nodeRename(x, ext.rename_array, ext);

        if(ext.add_emoji)
            x.Remark = addEmoji(x, ext.emoji_array, ext);
    });

    if(ext.sort_flag)
    {
        bool failed = true;
        if(ext.sort_script.size())
        {
            std::string script = ext.sort_script;
            if(startsWith(script, "path:"))
                script = fileGet(script.substr(5), false);
            script_safe_runner(ext.js_runtime, ext.js_context, [&](qjs::Context &ctx)
            {
                try
                {
                    ctx.eval(script);
                    auto compare = (std::function<int(const Proxy&, const Proxy&)>) ctx.eval("compare");
                    auto comparer = [&](const Proxy &a, const Proxy &b)
                    {
                        if(a.Type == ProxyType::Unknow)
                            return 1;
                        if(b.Type == ProxyType::Unknow)
                            return 0;
                        return compare(a, b);
                    };
                    std::stable_sort(nodes.begin(), nodes.end(), comparer);
                    failed = false;
                }
                catch(qjs::exception)
                {
                    script_print_stack(ctx);
                }
            }, global.scriptCleanContext);
        }
        if(failed) std::stable_sort(nodes.begin(), nodes.end(), [](const Proxy &a, const Proxy &b)
        {
            return a.Remark < b.Remark;
        });
    }
}