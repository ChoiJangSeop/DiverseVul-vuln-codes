void ImmutableConstantOp::Compute(OpKernelContext* ctx) {
  std::unique_ptr<MemmappedTensorAllocator> allocator(
      new MemmappedTensorAllocator());

  OP_REQUIRES_OK(ctx,
                 allocator->InitializeFromRegion(region_name_, ctx->env()));
  ctx->set_output(0, Tensor(allocator.get(), dtype_, shape_));
  OP_REQUIRES_OK(ctx, allocator->allocation_status());
  // Allocator is owned by the tensor from this point.
  allocator.release()->set_delete_on_deallocate();
}