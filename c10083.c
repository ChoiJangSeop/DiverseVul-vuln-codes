static int intel_engine_setup(struct intel_gt *gt, enum intel_engine_id id)
{
	const struct engine_info *info = &intel_engines[id];
	struct drm_i915_private *i915 = gt->i915;
	struct intel_engine_cs *engine;

	BUILD_BUG_ON(MAX_ENGINE_CLASS >= BIT(GEN11_ENGINE_CLASS_WIDTH));
	BUILD_BUG_ON(MAX_ENGINE_INSTANCE >= BIT(GEN11_ENGINE_INSTANCE_WIDTH));

	if (GEM_DEBUG_WARN_ON(id >= ARRAY_SIZE(gt->engine)))
		return -EINVAL;

	if (GEM_DEBUG_WARN_ON(info->class > MAX_ENGINE_CLASS))
		return -EINVAL;

	if (GEM_DEBUG_WARN_ON(info->instance > MAX_ENGINE_INSTANCE))
		return -EINVAL;

	if (GEM_DEBUG_WARN_ON(gt->engine_class[info->class][info->instance]))
		return -EINVAL;

	engine = kzalloc(sizeof(*engine), GFP_KERNEL);
	if (!engine)
		return -ENOMEM;

	BUILD_BUG_ON(BITS_PER_TYPE(engine->mask) < I915_NUM_ENGINES);

	engine->id = id;
	engine->legacy_idx = INVALID_ENGINE;
	engine->mask = BIT(id);
	engine->i915 = i915;
	engine->gt = gt;
	engine->uncore = gt->uncore;
	engine->hw_id = engine->guc_id = info->hw_id;
	engine->mmio_base = __engine_mmio_base(i915, info->mmio_bases);

	engine->class = info->class;
	engine->instance = info->instance;
	__sprint_engine_name(engine);

	engine->props.heartbeat_interval_ms =
		CONFIG_DRM_I915_HEARTBEAT_INTERVAL;
	engine->props.max_busywait_duration_ns =
		CONFIG_DRM_I915_MAX_REQUEST_BUSYWAIT;
	engine->props.preempt_timeout_ms =
		CONFIG_DRM_I915_PREEMPT_TIMEOUT;
	engine->props.stop_timeout_ms =
		CONFIG_DRM_I915_STOP_TIMEOUT;
	engine->props.timeslice_duration_ms =
		CONFIG_DRM_I915_TIMESLICE_DURATION;

	/* Override to uninterruptible for OpenCL workloads. */
	if (INTEL_GEN(i915) == 12 && engine->class == RENDER_CLASS)
		engine->props.preempt_timeout_ms = 0;

	engine->defaults = engine->props; /* never to change again */

	engine->context_size = intel_engine_context_size(gt, engine->class);
	if (WARN_ON(engine->context_size > BIT(20)))
		engine->context_size = 0;
	if (engine->context_size)
		DRIVER_CAPS(i915)->has_logical_contexts = true;

	/* Nothing to do here, execute in order of dependencies */
	engine->schedule = NULL;

	ewma__engine_latency_init(&engine->latency);
	seqlock_init(&engine->stats.lock);

	ATOMIC_INIT_NOTIFIER_HEAD(&engine->context_status_notifier);

	/* Scrub mmio state on takeover */
	intel_engine_sanitize_mmio(engine);

	gt->engine_class[info->class][info->instance] = engine;
	gt->engine[id] = engine;

	return 0;
}