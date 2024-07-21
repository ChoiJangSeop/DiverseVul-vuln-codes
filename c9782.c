void ring_buffer_reset_cpu(struct trace_buffer *buffer, int cpu)
{
	struct ring_buffer_per_cpu *cpu_buffer = buffer->buffers[cpu];

	if (!cpumask_test_cpu(cpu, buffer->cpumask))
		return;

	atomic_inc(&cpu_buffer->resize_disabled);
	atomic_inc(&cpu_buffer->record_disabled);

	/* Make sure all commits have finished */
	synchronize_rcu();

	reset_disabled_cpu_buffer(cpu_buffer);

	atomic_dec(&cpu_buffer->record_disabled);
	atomic_dec(&cpu_buffer->resize_disabled);
}