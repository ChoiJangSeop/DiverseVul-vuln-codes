PYBIND11_MODULE(_pywrap_tfe, m) {
  py::class_<TFE_Executor> TFE_Executor_class(m, "TFE_Executor");
  py::class_<TFE_ContextOptions> TFE_ContextOptions_class(m,
                                                          "TFE_ContextOptions");
  py::class_<TFE_MonitoringCounter0> TFE_MonitoringCounter0_class(
      m, "TFE_MonitoringCounter0");
  py::class_<TFE_MonitoringCounter1> TFE_MonitoringCounter1_class(
      m, "TFE_MonitoringCounter1");
  py::class_<TFE_MonitoringCounter2> TFE_MonitoringCounter2_class(
      m, "TFE_MonitoringCounter2");
  py::class_<TFE_MonitoringStringGauge0> TFE_MonitoringStringGauge0_class(
      m, "TFE_MonitoringStringGauge0");
  py::class_<TFE_MonitoringStringGauge1> TFE_MonitoringStringGauge1_class(
      m, "TFE_MonitoringStringGauge1");
  py::class_<TFE_MonitoringStringGauge2> TFE_MonitoringStringGauge2_class(
      m, "TFE_MonitoringStringGauge2");
  py::class_<TFE_MonitoringIntGauge0> TFE_MonitoringIntGauge0_class(
      m, "TFE_MonitoringIntGauge0");
  py::class_<TFE_MonitoringIntGauge1> TFE_MonitoringIntGauge1_class(
      m, "TFE_MonitoringIntGauge1");
  py::class_<TFE_MonitoringIntGauge2> TFE_MonitoringIntGauge2_class(
      m, "TFE_MonitoringIntGauge2");
  py::class_<TFE_MonitoringBoolGauge0> TFE_MonitoringBoolGauge0_class(
      m, "TFE_MonitoringBoolGauge0");
  py::class_<TFE_MonitoringBoolGauge1> TFE_MonitoringBoolGauge1_class(
      m, "TFE_MonitoringBoolGauge1");
  py::class_<TFE_MonitoringBoolGauge2> TFE_MonitoringBoolGauge2_class(
      m, "TFE_MonitoringBoolGauge2");
  py::class_<TFE_MonitoringCounterCell> TFE_MonitoringCounterCell_class(
      m, "TFE_MonitoringCounterCell");
  py::class_<TFE_MonitoringIntGaugeCell> TFE_MonitoringIntGaugeCell_class(
      m, "TFE_MonitoringIntGaugeCell");
  py::class_<TFE_MonitoringStringGaugeCell> TFE_MonitoringStringGaugeCell_class(
      m, "TFE_MonitoringStringGaugeCell");
  py::class_<TFE_MonitoringBoolGaugeCell> TFE_MonitoringBoolGaugeCell_class(
      m, "TFE_MonitoringBoolGaugeCell");
  py::class_<TFE_MonitoringSamplerCell> TFE_MonitoringSamplerCell_class(
      m, "TFE_MonitoringSamplerCell");
  py::class_<TFE_MonitoringBuckets> TFE_MonitoringBuckets_class(
      m, "TFE_MonitoringBuckets");
  py::class_<TFE_MonitoringSampler0> TFE_MonitoringSampler0_class(
      m, "TFE_MonitoringSampler0");
  py::class_<TFE_MonitoringSampler1> TFE_MonitoringSampler1_class(
      m, "TFE_MonitoringSampler1");
  py::class_<TFE_MonitoringSampler2> TFE_MonitoringSampler2_class(
      m, "TFE_MonitoringSampler2");
  py::class_<TFE_CancellationManager> TFE_CancellationManager_class(
      m, "TFE_CancellationManager");

  py::class_<TF_DeviceList> TF_DeviceList_class(m, "TF_DeviceList");
  py::class_<TF_Function> TF_Function_class(m, "TF_Function");

  m.def("TFE_Py_RegisterExceptionClass", [](const py::handle& e) {
    return tensorflow::PyoOrThrow(TFE_Py_RegisterExceptionClass(e.ptr()));
  });
  m.def("TFE_Py_RegisterFallbackExceptionClass", [](const py::handle& e) {
    return tensorflow::PyoOrThrow(
        TFE_Py_RegisterFallbackExceptionClass(e.ptr()));
  });

  m.def(
      "TFE_GetTotalMemoryUsage", [](py::handle& ctx, const char* device_name) {
        tensorflow::EagerContext* context = tensorflow::ContextFromInterface(
            reinterpret_cast<tensorflow::ImmediateExecutionContext*>(
                tensorflow::InputTFE_Context(ctx)));

        tensorflow::DeviceNameUtils::ParsedName input_device_name;
        if (!tensorflow::DeviceNameUtils::ParseFullOrLocalName(
                device_name, &input_device_name)) {
          tensorflow::ThrowValueError(
              absl::StrFormat("Failed parsing device name: '%s'", device_name)
                  .c_str());
        }

        std::vector<tensorflow::Device*> devices =
            context->local_device_mgr()->ListDevices();

        tensorflow::Device* matched_device = nullptr;
        for (int device_idx = 0; device_idx < devices.size(); device_idx++) {
          tensorflow::Device* device = devices[device_idx];

          if (tensorflow::DeviceNameUtils::AreCompatibleDevNames(
                  input_device_name, device->parsed_name())) {
            if (device->device_type() == tensorflow::DEVICE_CPU) {
              tensorflow::ThrowValueError(
                  "CPU does not support getting allocator information");
            }

            if (matched_device != nullptr) {
              tensorflow::ThrowValueError(
                  absl::StrFormat(
                      "Multiple devices matching the provided string "
                      "'%s': '%s' and "
                      "'%s' ",
                      device_name, matched_device->name(), device->name())
                      .c_str());
            }
            matched_device = device;
          }
        }

        if (matched_device == nullptr) {
          tensorflow::ThrowValueError(
              absl::StrFormat("No matching devices found for '%s'", device_name)
                  .c_str());
        }

        tensorflow::AllocatorAttributes attrs;
        tensorflow::Allocator* allocator = matched_device->GetAllocator(attrs);

        if (absl::optional<tensorflow::AllocatorStats> stats =
                allocator->GetStats()) {
          return stats->bytes_in_use;
        }

        tensorflow::ThrowTypeError(
            absl::StrFormat("Allocator stats not available for device '%s'",
                            matched_device->name())
                .c_str());
      });

  // XLA Eager Logic
  m.def("TF_SetXlaEnableLazyCompilation", &TF_SetXlaEnableLazyCompilation);
  m.def("TF_SetTfXlaCpuGlobalJit", &TF_SetTfXlaCpuGlobalJit);
  m.def("TF_SetXlaAutoJitMode", &TF_SetXlaAutoJitMode);
  m.def("TF_SetXlaConstantFoldingDisabled", &TF_SetXlaConstantFoldingDisabled);
  m.def("TF_GetXlaConstantFoldingDisabled", &TF_GetXlaConstantFoldingDisabled);
  m.def("TF_SetXlaMinClusterSize", &TF_SetXlaMinClusterSize);
  m.def("TF_GetCompilerIr", &tensorflow::TFE_GetCompilerIr);

  // MLIR Logic
  m.def("TF_IsMlirBridgeEnabled", [] {
    return tensorflow::GetMlirCommonFlags()->tf_mlir_enable_mlir_bridge;
  });
  m.def("TF_EnableMlirBridge", [](bool enabled) {
    tensorflow::GetMlirCommonFlags()->tf_mlir_enable_mlir_bridge = enabled;
  });
  m.def("TF_EnableXlaDevices", [] {
    tensorflow::GetXlaDeviceFlags()->tf_xla_enable_xla_devices = true;
  });

  // // TFE_Context Logic
  m.def(
      "TFE_NewContext",
      [](const TFE_ContextOptions* opts) {
        tensorflow::Safe_TF_StatusPtr status =
            tensorflow::make_safe(TF_NewStatus());
        TFE_Context* context = TFE_NewContext(opts, status.get());
        tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
        return tensorflow::PyoOrThrow(tensorflow::OutputTFE_Context(context));
      },
      py::return_value_policy::reference);
  m.def("TFE_DeleteContext", [](py::handle& o) {
    TFE_DeleteContext(tensorflow::InputTFE_Context(o));
  });
  m.def(
      "TFE_ContextListDevices",
      [](py::handle& o) {
        tensorflow::Safe_TF_StatusPtr status =
            tensorflow::make_safe(TF_NewStatus());
        auto output = TFE_ContextListDevices(tensorflow::InputTFE_Context(o),
                                             status.get());
        tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
        return output;
      },
      py::return_value_policy::reference);
  m.def("TFE_HostAddressSpace", [](py::handle& o, TF_Buffer& buf) {
    TFE_HostAddressSpace(tensorflow::InputTFE_Context(o), &buf);
  });
  m.def("TFE_ContextAddFunction", [](py::handle& ctx, TF_Function* func) {
    tensorflow::Safe_TF_StatusPtr status =
        tensorflow::make_safe(TF_NewStatus());
    TFE_ContextAddFunction(tensorflow::InputTFE_Context(ctx), func,
                           status.get());
    tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
  });
  m.def("TFE_ContextAddFunctionDef",
        [](py::handle& ctx, const char* serialized_function_def, size_t size) {
          tensorflow::Safe_TF_StatusPtr status =
              tensorflow::make_safe(TF_NewStatus());
          TFE_ContextAddFunctionDef(tensorflow::InputTFE_Context(ctx),
                                    serialized_function_def, size,
                                    status.get());
          tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
        });
  m.def("TFE_ContextGetFunctionDef",
        [](py::handle& ctx, const char* function_name, TF_Buffer& buf) {
          tensorflow::Safe_TF_StatusPtr status =
              tensorflow::make_safe(TF_NewStatus());
          TFE_ContextGetFunctionDef(tensorflow::InputTFE_Context(ctx),
                                    function_name, &buf, status.get());
          tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
        });
  m.def("TFE_ContextRemoveFunction", [](py::handle& ctx, const char* name) {
    tensorflow::Safe_TF_StatusPtr status =
        tensorflow::make_safe(TF_NewStatus());
    TFE_ContextRemoveFunction(tensorflow::InputTFE_Context(ctx), name,
                              status.get());
    tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
  });
  m.def("TFE_ContextHasFunction", [](py::handle& ctx, const char* name) {
    tensorflow::Safe_TF_StatusPtr status =
        tensorflow::make_safe(TF_NewStatus());
    auto output =
        TFE_ContextHasFunction(tensorflow::InputTFE_Context(ctx), name);
    tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
    return output;
  });
  m.def("TFE_ContextEnableRunMetadata", [](py::handle& ctx) {
    TFE_ContextEnableRunMetadata(tensorflow::InputTFE_Context(ctx));
  });
  m.def("TFE_ContextDisableRunMetadata", [](py::handle& ctx) {
    TFE_ContextEnableRunMetadata(tensorflow::InputTFE_Context(ctx));
  });
  m.def("TFE_ContextEnableGraphCollection", [](py::handle& ctx) {
    TFE_ContextEnableGraphCollection(tensorflow::InputTFE_Context(ctx));
  });
  m.def("TFE_ContextDisableGraphCollection", [](py::handle& ctx) {
    TFE_ContextDisableGraphCollection(tensorflow::InputTFE_Context(ctx));
  });
  m.def("TFE_ContextExportRunMetadata", [](py::handle& ctx, TF_Buffer& buf) {
    tensorflow::Safe_TF_StatusPtr status =
        tensorflow::make_safe(TF_NewStatus());
    TFE_ContextExportRunMetadata(tensorflow::InputTFE_Context(ctx), &buf,
                                 status.get());
    tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
  });
  m.def("TFE_ContextClearCaches", [](py::handle& o) {
    TFE_ContextClearCaches(tensorflow::InputTFE_Context(o));
  });
  m.def("TFE_GetContextId", [](py::handle& ctx) {
    return TFE_GetContextId(tensorflow::InputTFE_Context(ctx));
  });
  m.def("TFE_ContextGetDevicePlacementPolicy", [](py::handle& ctx) {
    return TFE_ContextGetDevicePlacementPolicy(
        tensorflow::InputTFE_Context(ctx));
  });
  m.def("TFE_ContextSetThreadLocalDevicePlacementPolicy",
        [](py::handle& ctx, TFE_ContextDevicePlacementPolicy policy) {
          TFE_ContextSetThreadLocalDevicePlacementPolicy(
              tensorflow::InputTFE_Context(ctx), policy);
        });
  m.def("TFE_ContextSetServerDef", [](py::handle& ctx, int keep_alive_secs,
                                      py::bytes proto) {
    tensorflow::Safe_TF_StatusPtr status =
        tensorflow::make_safe(TF_NewStatus());
    tensorflow::Safe_TF_BufferPtr buf =
        tensorflow::make_safe(tensorflow::ProtoStringToTFBuffer(proto.ptr()));
    TFE_ContextSetServerDef(tensorflow::InputTFE_Context(ctx), keep_alive_secs,
                            buf.get()->data, buf.get()->length, status.get());
    tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
  });
  m.def("TFE_ContextUpdateServerDef", [](py::handle& ctx, int keep_alive_secs,
                                         py::bytes proto) {
    tensorflow::Safe_TF_StatusPtr status =
        tensorflow::make_safe(TF_NewStatus());
    tensorflow::Safe_TF_BufferPtr buf =
        tensorflow::make_safe(tensorflow::ProtoStringToTFBuffer(proto.ptr()));
    Py_BEGIN_ALLOW_THREADS;
    TFE_ContextUpdateServerDef(tensorflow::InputTFE_Context(ctx),
                               keep_alive_secs, buf.get()->data,
                               buf.get()->length, status.get());
    Py_END_ALLOW_THREADS;
    tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
  });
  m.def("TFE_ContextCheckAlive", [](py::handle& ctx, const char* worker_name) {
    tensorflow::Safe_TF_StatusPtr status =
        tensorflow::make_safe(TF_NewStatus());
    bool output = TFE_ContextCheckAlive(tensorflow::InputTFE_Context(ctx),
                                        worker_name, status.get());
    tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
    return output;
  });
  m.def("TFE_ContextSyncExecutors", [](py::handle& ctx) {
    tensorflow::Safe_TF_StatusPtr status =
        tensorflow::make_safe(TF_NewStatus());
    TFE_ContextAsyncWait(tensorflow::InputTFE_Context(ctx), status.get());
    tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
  });
  m.def("TFE_ContextClearExecutors", [](py::handle& ctx) {
    tensorflow::Safe_TF_StatusPtr status =
        tensorflow::make_safe(TF_NewStatus());
    TFE_ContextAsyncWait(tensorflow::InputTFE_Context(ctx), status.get());
    // NOTE: different from TFE_ContextSyncExecutors that raises potential
    // errors, deliberately ignore executor statuses in cleanup.
  });
  m.def("TFE_ContextSetSoftDevicePlacement", [](py::handle& ctx, bool enable) {
    tensorflow::Safe_TF_StatusPtr status =
        tensorflow::make_safe(TF_NewStatus());
    TFE_ContextSetSoftDevicePlacement(tensorflow::InputTFE_Context(ctx), enable,
                                      status.get());
  });
  m.def("TFE_ContextSetLogDevicePlacement", [](py::handle& ctx, bool enable) {
    tensorflow::Safe_TF_StatusPtr status =
        tensorflow::make_safe(TF_NewStatus());
    TFE_ContextSetSoftDevicePlacement(tensorflow::InputTFE_Context(ctx), enable,
                                      status.get());
  });

  // TFE_Executor logic
  m.def(
      "TFE_NewExecutor",
      [](const bool is_async) {
        TFE_Executor* exc = TFE_NewExecutor(is_async);
        return exc;
      },
      py::return_value_policy::reference);
  m.def("TFE_DeleteExecutor", &TFE_DeleteExecutor);
  m.def("TFE_ExecutorIsAsync", &TFE_ExecutorIsAsync);
  m.def("TFE_ExecutorWaitForAllPendingNodes", [](TFE_Executor& exc) {
    tensorflow::Safe_TF_StatusPtr status =
        tensorflow::make_safe(TF_NewStatus());
    // NOTE: release Python GIL for pending PyFunc ops to be executed properly.
    Py_BEGIN_ALLOW_THREADS;
    TFE_ExecutorWaitForAllPendingNodes(&exc, status.get());
    Py_END_ALLOW_THREADS;
    tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
  });
  m.def("TFE_ExecutorClearError", &TFE_ExecutorClearError);
  m.def("TFE_ContextSetExecutorForThread", [](py::handle& ctx,
                                              TFE_Executor& exc) {
    TFE_ContextSetExecutorForThread(tensorflow::InputTFE_Context(ctx), &exc);
  });
  m.def(
      "TFE_ContextGetExecutorForThread",
      [](py::handle& o) {
        return TFE_ContextGetExecutorForThread(tensorflow::InputTFE_Context(o));
      },
      py::return_value_policy::reference);

  m.def("TFE_OpNameGetAttrType",
        [](py::handle& ctx, const char* op_or_function_name,
           const char* attr_name) {
          int temp = 0;
          unsigned char* is_list = reinterpret_cast<unsigned char*>(&temp);
          tensorflow::Safe_TF_StatusPtr status =
              tensorflow::make_safe(TF_NewStatus());
          auto output = TFE_OpNameGetAttrType(tensorflow::InputTFE_Context(ctx),
                                              op_or_function_name, attr_name,
                                              is_list, status.get());
          tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
#if PY_MAJOR_VERSION < 3
          PyObject* output_pyo = PyInt_FromLong(output);
#else
          PyObject* output_pyo = PyLong_FromLong(output);
#endif
          if (*is_list == 1) {
            PyObject* list = PyList_New(1);
            PyList_SetItem(list, 0, output_pyo);
            return tensorflow::PyoOrThrow(list);
          }
          return tensorflow::PyoOrThrow(output_pyo);
        });
  m.def("TFE_Py_InitEagerTensor", [](const py::handle& o) {
    return tensorflow::PyoOrThrow(TFE_Py_InitEagerTensor(o.ptr()));
  });
  m.def("TFE_Py_PackEagerTensors",
        [](const py::handle& context, const py::handle& handles) {
          return tensorflow::TFE_Py_PackEagerTensors_wrapper(context, handles);
        });
  m.def("TFE_Py_SetEagerTensorProfiler", &TFE_Py_SetEagerTensorProfiler);
  m.def("TFE_Py_RegisterJVPFunction", [](const py::handle& o) {
    return tensorflow::PyoOrThrow(TFE_Py_RegisterJVPFunction(o.ptr()));
  });
  m.def("TFE_Py_RegisterGradientFunction", [](const py::handle& o) {
    return tensorflow::PyoOrThrow(TFE_Py_RegisterGradientFunction(o.ptr()));
  });
  m.def("TFE_Py_Execute",
        [](const py::handle& context, const char* device_name,
           const char* op_name, const py::handle& inputs,
           const py::handle& attrs, const py::handle& num_outputs) {
          return tensorflow::TFE_Py_ExecuteCancelable_wrapper(
              context, device_name, op_name, inputs, attrs.ptr(), nullptr,
              num_outputs);
        });
  m.def(
      "TFE_Py_ExecuteCancelable",
      [](const py::handle& context, const char* device_name,
         const char* op_name, const py::handle& inputs, const py::handle& attrs,
         TFE_CancellationManager& cancellation_manager,
         const py::handle& num_outputs) {
        return tensorflow::TFE_Py_ExecuteCancelable_wrapper(
            context, device_name, op_name, inputs, attrs.ptr(),
            &cancellation_manager, num_outputs);
      });
  m.def("TFE_Py_FastPathExecute", [](const py::args args) {
    // TFE_Py_FastPathExecute requires error checking prior to returning.
    return tensorflow::PyoOrThrow(TFE_Py_FastPathExecute_C(args.ptr()));
  });
  m.def("TFE_Py_RecordGradient",
        [](const py::handle& op_name, const py::handle& inputs,
           const py::handle& attrs, const py::handle& results,
           const py::handle& forward_pass_name_scope) {
          return tensorflow::PyoOrThrow(TFE_Py_RecordGradient(
              op_name.ptr(), inputs.ptr(), attrs.ptr(), results.ptr(),
              forward_pass_name_scope.ptr()));
        });
  m.def("TFE_Py_UID", []() { return tensorflow::PyoOrThrow(TFE_Py_UID()); });

  // TFE_Py_Tape Logic
  m.def("TFE_Py_TapeSetNew", [](const py::handle& persistent,
                                const py::handle& watch_accessed_variables) {
    return tensorflow::PyoOrThrow(
        TFE_Py_TapeSetNew(persistent.ptr(), watch_accessed_variables.ptr()));
  });
  m.def("TFE_Py_TapeSetAdd",
        [](const py::handle& tape) { TFE_Py_TapeSetAdd(tape.ptr()); });
  m.def("TFE_Py_TapeSetRemove",
        [](const py::handle& tape) { TFE_Py_TapeSetRemove(tape.ptr()); });
  m.def("TFE_Py_TapeSetStopOnThread", &TFE_Py_TapeSetStopOnThread);
  m.def("TFE_Py_TapeSetRestartOnThread", &TFE_Py_TapeSetRestartOnThread);
  m.def("TFE_Py_TapeSetIsStopped",
        []() { return tensorflow::PyoOrThrow(TFE_Py_TapeSetIsStopped()); });
  m.def("TFE_Py_TapeSetIsEmpty",
        []() { return tensorflow::PyoOrThrow(TFE_Py_TapeSetIsEmpty()); });
  m.def("TFE_Py_TapeSetShouldRecordBackprop", [](const py::handle& tensors) {
    return tensorflow::PyoOrThrow(
        TFE_Py_TapeSetShouldRecordBackprop(tensors.ptr()));
  });
  m.def("TFE_Py_TapeSetPossibleGradientTypes", [](const py::handle& tensors) {
    return tensorflow::PyoOrThrow(
        TFE_Py_TapeSetPossibleGradientTypes(tensors.ptr()));
  });
  m.def("TFE_Py_TapeSetDeleteTrace", &TFE_Py_TapeSetDeleteTrace);
  m.def("TFE_Py_TapeSetRecordOperation",
        [](const py::handle& op_type, const py::handle& output_tensors,
           const py::handle& input_tensors, const py::handle& backward_function,
           const py::handle& forward_function) {
          return tensorflow::PyoOrThrow(TFE_Py_TapeSetRecordOperation(
              op_type.ptr(), output_tensors.ptr(), input_tensors.ptr(),
              backward_function.ptr(), forward_function.ptr()));
        });
  m.def(
      "TFE_Py_TapeSetRecordOperationBackprop",
      [](const py::handle& op_type, const py::handle& output_tensors,
         const py::handle& input_tensors, const py::handle& backward_function) {
        return tensorflow::PyoOrThrow(TFE_Py_TapeSetRecordOperationBackprop(
            op_type.ptr(), output_tensors.ptr(), input_tensors.ptr(),
            backward_function.ptr()));
      });
  m.def(
      "TFE_Py_TapeSetRecordOperationForwardprop",
      [](const py::handle& op_type, const py::handle& output_tensors,
         const py::handle& input_tensors, const py::handle& backward_function,
         const py::handle& forwardprop_output_indices) {
        return tensorflow::PyoOrThrow(TFE_Py_TapeSetRecordOperationForwardprop(
            op_type.ptr(), output_tensors.ptr(), input_tensors.ptr(),
            backward_function.ptr(), forwardprop_output_indices.ptr()));
      });
  m.def("TFE_Py_TapeGradient",
        [](const py::handle& tape, const py::handle& target,
           const py::handle& sources, const py::handle& output_gradients,
           const py::handle& sources_raw,
           const py::handle& unconnected_gradients) {
          tensorflow::Safe_TF_StatusPtr status =
              tensorflow::make_safe(TF_NewStatus());
          PyObject* output = TFE_Py_TapeGradient(
              tape.ptr(), target.ptr(), sources.ptr(), output_gradients.ptr(),
              sources_raw.ptr(), unconnected_gradients.ptr(), status.get());
          tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
          return tensorflow::PyoOrThrow(output);
        });

  m.def("TFE_Py_TapeVariableAccessed", [](const py::handle& variable) {
    TFE_Py_TapeVariableAccessed(variable.ptr());
  });
  m.def("TFE_Py_TapeWatch",
        [](const py::handle& tape, const py::handle& tensor) {
          TFE_Py_TapeWatch(tape.ptr(), tensor.ptr());
        });
  m.def("TFE_Py_TapeWatchVariable",
        [](const py::handle& tape, const py::handle& variable) {
          TFE_Py_TapeWatchVariable(tape.ptr(), variable.ptr());
        });
  m.def("TFE_Py_TapeWatchedVariables", [](const py::handle& tape) {
    return tensorflow::PyoOrThrow(TFE_Py_TapeWatchedVariables(tape.ptr()));
  });

  // TFE_Py_VariableWatcher logic.
  m.def("TFE_Py_VariableWatcherNew",
        []() { return tensorflow::PyoOrThrow(TFE_Py_VariableWatcherNew()); });
  m.def("TFE_Py_VariableWatcherRemove", [](const py::handle& variable_watcher) {
    TFE_Py_VariableWatcherRemove(variable_watcher.ptr());
  });
  m.def("TFE_Py_VariableWatcherVariableAccessed",
        [](const py::handle& variable) {
          TFE_Py_VariableWatcherVariableAccessed(variable.ptr());
        });
  m.def("TFE_Py_VariableWatcherWatchedVariables",
        [](const py::handle& variable_watcher) {
          return tensorflow::PyoOrThrow(
              TFE_Py_VariableWatcherWatchedVariables(variable_watcher.ptr()));
        });

  // TFE_Py_ForwardAccumulator logic.
  m.def("TFE_Py_ForwardAccumulatorNew", [](bool use_batch) {
    return tensorflow::PyoOrThrow(TFE_Py_ForwardAccumulatorNew(use_batch));
  });

  m.def("TFE_Py_ForwardAccumulatorSetAdd", [](const py::handle& accumulator) {
    return tensorflow::PyoOrThrow(
        TFE_Py_ForwardAccumulatorSetAdd(accumulator.ptr()));
  });
  m.def("TFE_Py_ForwardAccumulatorSetRemove",
        [](const py::handle& accumulator) {
          TFE_Py_ForwardAccumulatorSetRemove(accumulator.ptr());
        });

  m.def("TFE_Py_ForwardAccumulatorWatch",
        [](const py::handle& accumulator, const py::handle& tensor,
           const py::handle& tangent) {
          TFE_Py_ForwardAccumulatorWatch(accumulator.ptr(), tensor.ptr(),
                                         tangent.ptr());
        });
  m.def("TFE_Py_ForwardAccumulatorJVP",
        [](const py::handle& accumulator, const py::handle& tensor) {
          return tensorflow::PyoOrThrow(
              TFE_Py_ForwardAccumulatorJVP(accumulator.ptr(), tensor.ptr()));
        });
  m.def("TFE_Py_ForwardAccumulatorPushState", []() {
    return tensorflow::PyoOrThrow(TFE_Py_ForwardAccumulatorPushState());
  });
  m.def("TFE_Py_ForwardAccumulatorPopState", []() {
    return tensorflow::PyoOrThrow(TFE_Py_ForwardAccumulatorPopState());
  });
  m.def("TFE_Py_PackJVPs", [](const py::handle& tensors) {
    return tensorflow::PyoOrThrow(TFE_Py_PackJVPs(tensors.ptr()));
  });

  // TFE_ContextOptions Logic
  m.def("TFE_NewContextOptions", &TFE_NewContextOptions,
        py::return_value_policy::reference);
  m.def("TFE_ContextOptionsSetConfig", [](TFE_ContextOptions* options,
                                          py::bytes proto) {
    tensorflow::Safe_TF_StatusPtr status =
        tensorflow::make_safe(TF_NewStatus());
    tensorflow::Safe_TF_BufferPtr buf =
        tensorflow::make_safe(tensorflow::ProtoStringToTFBuffer(proto.ptr()));
    TFE_ContextOptionsSetConfig(options, buf.get()->data, buf.get()->length,
                                status.get());
    tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
  });
  m.def("TFE_ContextOptionsSetDevicePlacementPolicy",
        &TFE_ContextOptionsSetDevicePlacementPolicy);
  m.def("TFE_ContextOptionsSetLazyRemoteInputsCopy",
        &TFE_ContextOptionsSetLazyRemoteInputsCopy);
  m.def("TFE_ContextOptionsSetTfrt", &TFE_ContextOptionsSetTfrt);
  m.def("TFE_ContextOptionsSetAsync", &TFE_ContextOptionsSetAsync);
  m.def("TFE_DeleteContextOptions", &TFE_DeleteContextOptions,
        py::return_value_policy::reference);

  // TFE_Py_TensorShape Logic
  m.def("TFE_Py_TensorShapeSlice",
        [](const py::handle& tensors, int slice_dim) {
          return tensorflow::PyoOrThrow(
              TFE_Py_TensorShapeSlice(tensors.ptr(), slice_dim));
        });
  m.def("TFE_Py_TensorShapeOnDevice", [](const py::handle& tensors,
                                         int slice_dim) {
    return tensorflow::PyoOrThrow(TFE_Py_TensorShapeOnDevice(tensors.ptr()));
  });
  m.def("TFE_Py_EnableInteractivePythonLogging",
        &TFE_Py_EnableInteractivePythonLogging);

  // Additional Context Logic
  m.def("TFE_Py_SetEagerContext", [](const py::handle& o) {
    return tensorflow::PyoOrThrow(TFE_Py_SetEagerContext(o.ptr()));
  });
  m.def("TFE_ContextStartStep", [](py::handle& o) {
    TFE_ContextStartStep(tensorflow::InputTFE_Context(o.ptr()));
  });
  m.def("TFE_ContextEndStep", [](py::handle& o) {
    TFE_ContextEndStep(tensorflow::InputTFE_Context(o.ptr()));
  });
  m.def("TFE_Py_RegisterVSpace", [](const py::handle& o) {
    return tensorflow::PyoOrThrow(TFE_Py_RegisterVSpace(o.ptr()));
  });
  m.def("TFE_Py_EncodeArg",
        [](const py::handle& o, bool include_tensor_ranks_only) {
          return tensorflow::PyoOrThrow(
              TFE_Py_EncodeArg(o.ptr(), include_tensor_ranks_only));
        });
  m.def("TFE_EnableCollectiveOps", [](const py::handle& ctx, py::bytes proto) {
    tensorflow::Safe_TF_StatusPtr status =
        tensorflow::make_safe(TF_NewStatus());
    tensorflow::Safe_TF_BufferPtr buf =
        tensorflow::make_safe(tensorflow::ProtoStringToTFBuffer(proto.ptr()));
    TFE_EnableCollectiveOps(tensorflow::InputTFE_Context(ctx), buf.get()->data,
                            buf.get()->length, status.get());
    tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
  });
  m.def("TFE_AbortCollectiveOps", [](const py::handle& ctx, int code,
                                     const char* message) {
    tensorflow::Safe_TF_StatusPtr status =
        tensorflow::make_safe(TF_NewStatus());
    TF_SetStatus(status.get(), static_cast<TF_Code>(code), message);
    TFE_AbortCollectiveOps(tensorflow::InputTFE_Context(ctx), status.get());
  });
  m.def("TFE_CollectiveOpsCheckPeerHealth",
        [](const py::handle& ctx, const char* task) {
          tensorflow::Safe_TF_StatusPtr status =
              tensorflow::make_safe(TF_NewStatus());
          TFE_CollectiveOpsCheckPeerHealth(tensorflow::InputTFE_Context(ctx),
                                           task, status.get());
          tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
        });
  m.def("TF_ListPhysicalDevices", &tensorflow::TF_ListPhysicalDevices);
  m.def("TF_GetDeviceDetails", &tensorflow::TF_GetDeviceDetails);
  m.def("TF_DeleteDeviceList", &TF_DeleteDeviceList,
        py::return_value_policy::reference);
  m.def("TF_DeviceListCount", &TF_DeviceListCount);
  m.def("TF_DeviceListName", [](const TF_DeviceList* list, int index) {
    tensorflow::Safe_TF_StatusPtr status =
        tensorflow::make_safe(TF_NewStatus());
    auto output = TF_DeviceListName(list, index, status.get());
    tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
    return output;
  });
  m.def("TF_DeviceListType", [](const TF_DeviceList* list, int index) {
    tensorflow::Safe_TF_StatusPtr status =
        tensorflow::make_safe(TF_NewStatus());
    auto output = TF_DeviceListType(list, index, status.get());
    tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
    return output;
  });

  m.def("TF_PickUnusedPortOrDie", &TF_PickUnusedPortOrDie);

  // TFE_MonitoringCounter Logic
  m.def("TFE_MonitoringCounterCellIncrementBy",
        &TFE_MonitoringCounterCellIncrementBy);
  m.def("TFE_MonitoringCounterCellValue", &TFE_MonitoringCounterCellValue);
  m.def(
      "TFE_MonitoringNewCounter0",
      [](const char* name, const char* description) {
        tensorflow::Safe_TF_StatusPtr status =
            tensorflow::make_safe(TF_NewStatus());
        auto output =
            TFE_MonitoringNewCounter0(name, status.get(), description);
        tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
        return output;
      },
      py::return_value_policy::reference);
  m.def("TFE_MonitoringDeleteCounter0", &TFE_MonitoringDeleteCounter0,
        py::return_value_policy::reference);
  m.def("TFE_MonitoringGetCellCounter0", &TFE_MonitoringGetCellCounter0,
        py::return_value_policy::reference);
  m.def(
      "TFE_MonitoringNewCounter1",
      [](const char* name, const char* description, const char* label1) {
        tensorflow::Safe_TF_StatusPtr status =
            tensorflow::make_safe(TF_NewStatus());
        auto output =
            TFE_MonitoringNewCounter1(name, status.get(), description, label1);
        tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
        return output;
      },
      py::return_value_policy::reference);
  m.def("TFE_MonitoringDeleteCounter1", &TFE_MonitoringDeleteCounter1,
        py::return_value_policy::reference);
  m.def("TFE_MonitoringGetCellCounter1", &TFE_MonitoringGetCellCounter1,
        py::return_value_policy::reference);
  m.def(
      "TFE_MonitoringNewCounter2",
      [](const char* name, const char* description, const char* label1,
         const char* label2) {
        tensorflow::Safe_TF_StatusPtr status =
            tensorflow::make_safe(TF_NewStatus());
        auto output = TFE_MonitoringNewCounter2(name, status.get(), description,
                                                label1, label2);
        tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
        return output;
      },
      py::return_value_policy::reference);
  m.def("TFE_MonitoringDeleteCounter2", &TFE_MonitoringDeleteCounter2,
        py::return_value_policy::reference);
  m.def("TFE_MonitoringGetCellCounter2", &TFE_MonitoringGetCellCounter2,
        py::return_value_policy::reference);

  // TFE_MonitoringIntGauge Logic
  m.def("TFE_MonitoringIntGaugeCellSet", &TFE_MonitoringIntGaugeCellSet);
  m.def("TFE_MonitoringIntGaugeCellValue", &TFE_MonitoringIntGaugeCellValue);
  m.def(
      "TFE_MonitoringNewIntGauge0",
      [](const char* name, const char* description) {
        tensorflow::Safe_TF_StatusPtr status =
            tensorflow::make_safe(TF_NewStatus());
        auto output =
            TFE_MonitoringNewIntGauge0(name, status.get(), description);
        tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
        return output;
      },
      py::return_value_policy::reference);
  m.def("TFE_MonitoringDeleteIntGauge0", &TFE_MonitoringDeleteIntGauge0,
        py::return_value_policy::reference);
  m.def("TFE_MonitoringGetCellIntGauge0", &TFE_MonitoringGetCellIntGauge0,
        py::return_value_policy::reference);
  m.def(
      "TFE_MonitoringNewIntGauge1",
      [](const char* name, const char* description, const char* label1) {
        tensorflow::Safe_TF_StatusPtr status =
            tensorflow::make_safe(TF_NewStatus());
        auto output =
            TFE_MonitoringNewIntGauge1(name, status.get(), description, label1);
        tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
        return output;
      },
      py::return_value_policy::reference);
  m.def("TFE_MonitoringDeleteIntGauge1", &TFE_MonitoringDeleteIntGauge1,
        py::return_value_policy::reference);
  m.def("TFE_MonitoringGetCellIntGauge1", &TFE_MonitoringGetCellIntGauge1,
        py::return_value_policy::reference);
  m.def(
      "TFE_MonitoringNewIntGauge2",
      [](const char* name, const char* description, const char* label1,
         const char* label2) {
        tensorflow::Safe_TF_StatusPtr status =
            tensorflow::make_safe(TF_NewStatus());
        auto output = TFE_MonitoringNewIntGauge2(name, status.get(),
                                                 description, label1, label2);
        tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
        return output;
      },
      py::return_value_policy::reference);
  m.def("TFE_MonitoringDeleteIntGauge2", &TFE_MonitoringDeleteIntGauge2,
        py::return_value_policy::reference);
  m.def("TFE_MonitoringGetCellIntGauge2", &TFE_MonitoringGetCellIntGauge2,
        py::return_value_policy::reference);
  m.def("TFE_MonitoringStringGaugeCellSet", &TFE_MonitoringStringGaugeCellSet);
  m.def("TFE_MonitoringStringGaugeCellValue",
        &TFE_MonitoringStringGaugeCellValue);
  m.def(
      "TFE_MonitoringNewStringGauge0",
      [](const char* name, const char* description) {
        tensorflow::Safe_TF_StatusPtr status =
            tensorflow::make_safe(TF_NewStatus());
        auto output =
            TFE_MonitoringNewStringGauge0(name, status.get(), description);
        tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
        return output;
      },
      py::return_value_policy::reference);

  // TFE_MonitoringStringGauge Logic
  m.def("TFE_MonitoringDeleteStringGauge0", &TFE_MonitoringDeleteStringGauge0);
  m.def("TFE_MonitoringGetCellStringGauge0", &TFE_MonitoringGetCellStringGauge0,
        py::return_value_policy::reference);
  m.def(
      "TFE_MonitoringNewStringGauge1",
      [](const char* name, const char* description, const char* label1) {
        tensorflow::Safe_TF_StatusPtr status =
            tensorflow::make_safe(TF_NewStatus());
        auto output = TFE_MonitoringNewStringGauge1(name, status.get(),
                                                    description, label1);
        tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
        return output;
      },
      py::return_value_policy::reference);
  m.def("TFE_MonitoringDeleteStringGauge1", &TFE_MonitoringDeleteStringGauge1);
  m.def("TFE_MonitoringGetCellStringGauge1", &TFE_MonitoringGetCellStringGauge1,
        py::return_value_policy::reference);
  m.def(
      "TFE_MonitoringNewStringGauge2",
      [](const char* name, const char* description, const char* label1,
         const char* label2) {
        tensorflow::Safe_TF_StatusPtr status =
            tensorflow::make_safe(TF_NewStatus());
        auto output = TFE_MonitoringNewStringGauge2(
            name, status.get(), description, label1, label2);
        tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
        return output;
      },
      py::return_value_policy::reference);
  m.def("TFE_MonitoringDeleteStringGauge2", &TFE_MonitoringDeleteStringGauge2);
  m.def("TFE_MonitoringGetCellStringGauge2", &TFE_MonitoringGetCellStringGauge2,
        py::return_value_policy::reference);

  // TFE_MonitoringBoolGauge Logic
  m.def("TFE_MonitoringBoolGaugeCellSet", &TFE_MonitoringBoolGaugeCellSet);
  m.def("TFE_MonitoringBoolGaugeCellValue", &TFE_MonitoringBoolGaugeCellValue);
  m.def(
      "TFE_MonitoringNewBoolGauge0",
      [](const char* name, const char* description) {
        tensorflow::Safe_TF_StatusPtr status =
            tensorflow::make_safe(TF_NewStatus());
        auto output =
            TFE_MonitoringNewBoolGauge0(name, status.get(), description);
        tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
        return output;
      },
      py::return_value_policy::reference);
  m.def("TFE_MonitoringDeleteBoolGauge0", &TFE_MonitoringDeleteBoolGauge0,
        py::return_value_policy::reference);
  m.def("TFE_MonitoringGetCellBoolGauge0", &TFE_MonitoringGetCellBoolGauge0,
        py::return_value_policy::reference);
  m.def(
      "TFE_MonitoringNewBoolGauge1",
      [](const char* name, const char* description, const char* label1) {
        tensorflow::Safe_TF_StatusPtr status =
            tensorflow::make_safe(TF_NewStatus());
        auto output = TFE_MonitoringNewBoolGauge1(name, status.get(),
                                                  description, label1);
        tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
        return output;
      },
      py::return_value_policy::reference);
  m.def("TFE_MonitoringDeleteBoolGauge1", &TFE_MonitoringDeleteBoolGauge1,
        py::return_value_policy::reference);
  m.def("TFE_MonitoringGetCellBoolGauge1", &TFE_MonitoringGetCellBoolGauge1,
        py::return_value_policy::reference);
  m.def(
      "TFE_MonitoringNewBoolGauge2",
      [](const char* name, const char* description, const char* label1,
         const char* label2) {
        tensorflow::Safe_TF_StatusPtr status =
            tensorflow::make_safe(TF_NewStatus());
        auto output = TFE_MonitoringNewBoolGauge2(name, status.get(),
                                                  description, label1, label2);
        tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
        return output;
      },
      py::return_value_policy::reference);
  m.def("TFE_MonitoringDeleteBoolGauge2", &TFE_MonitoringDeleteBoolGauge2,
        py::return_value_policy::reference);
  m.def("TFE_MonitoringGetCellBoolGauge2", &TFE_MonitoringGetCellBoolGauge2,
        py::return_value_policy::reference);

  // TFE_MonitoringSampler Logic
  m.def("TFE_MonitoringSamplerCellAdd", &TFE_MonitoringSamplerCellAdd);
  m.def("TFE_MonitoringSamplerCellValue", &TFE_MonitoringSamplerCellValue);
  m.def("TFE_MonitoringNewExponentialBuckets",
        &TFE_MonitoringNewExponentialBuckets,
        py::return_value_policy::reference);
  m.def("TFE_MonitoringDeleteBuckets", &TFE_MonitoringDeleteBuckets,
        py::return_value_policy::reference);
  m.def(
      "TFE_MonitoringNewSampler0",
      [](const char* name, TFE_MonitoringBuckets* buckets,
         const char* description) {
        tensorflow::Safe_TF_StatusPtr status =
            tensorflow::make_safe(TF_NewStatus());
        auto output =
            TFE_MonitoringNewSampler0(name, buckets, status.get(), description);
        tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
        return output;
      },
      py::return_value_policy::reference);
  m.def("TFE_MonitoringDeleteSampler0", &TFE_MonitoringDeleteSampler0,
        py::return_value_policy::reference);
  m.def("TFE_MonitoringGetCellSampler0", &TFE_MonitoringGetCellSampler0,
        py::return_value_policy::reference);
  m.def(
      "TFE_MonitoringNewSampler1",
      [](const char* name, TFE_MonitoringBuckets* buckets,
         const char* description, const char* label1) {
        tensorflow::Safe_TF_StatusPtr status =
            tensorflow::make_safe(TF_NewStatus());
        auto output = TFE_MonitoringNewSampler1(name, buckets, status.get(),
                                                description, label1);
        tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
        return output;
      },
      py::return_value_policy::reference);
  m.def("TFE_MonitoringDeleteSampler1", &TFE_MonitoringDeleteSampler1,
        py::return_value_policy::reference);
  m.def("TFE_MonitoringGetCellSampler1", &TFE_MonitoringGetCellSampler1,
        py::return_value_policy::reference);
  m.def(
      "TFE_MonitoringNewSampler2",
      [](const char* name, TFE_MonitoringBuckets* buckets,
         const char* description, const char* label1, const char* label2) {
        tensorflow::Safe_TF_StatusPtr status =
            tensorflow::make_safe(TF_NewStatus());
        auto output = TFE_MonitoringNewSampler2(name, buckets, status.get(),
                                                description, label1, label2);
        tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
        return output;
      },
      py::return_value_policy::reference);
  m.def("TFE_MonitoringDeleteSampler2", &TFE_MonitoringDeleteSampler2,
        py::return_value_policy::reference);
  m.def("TFE_MonitoringGetCellSampler2", &TFE_MonitoringGetCellSampler2,
        py::return_value_policy::reference);

  // TFE_CancellationManager Logic
  m.def("TFE_NewCancellationManager", &TFE_NewCancellationManager,
        py::return_value_policy::reference);
  m.def("TFE_CancellationManagerIsCancelled",
        &TFE_CancellationManagerIsCancelled);
  m.def("TFE_CancellationManagerStartCancel",
        &TFE_CancellationManagerStartCancel);
  m.def("TFE_DeleteCancellationManager", &TFE_DeleteCancellationManager,
        py::return_value_policy::reference);

  m.def("TFE_ClearScalarCache", &tensorflow::TFE_ClearScalarCache);

  // Util buffer helper functions
  m.def("TF_NewBufferFromString", &TF_NewBufferFromString,
        py::return_value_policy::reference);

  // DLPack functions
  m.def("TFE_ToDlpackCapsule", [](py::handle& o) {
    PyObject* eager_tensor_pyobject_ptr = o.ptr();
    TFE_TensorHandle* thandle = EagerTensor_Handle(eager_tensor_pyobject_ptr);
    tensorflow::Safe_TF_StatusPtr status =
        tensorflow::make_safe(TF_NewStatus());
    void* dlm_ptr = tensorflow::TFE_HandleToDLPack(thandle, status.get());
    tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());

    py::capsule capsule(
        dlm_ptr, tensorflow::kDlTensorCapsuleName, [](PyObject* capsule) {
          if (PyCapsule_IsValid(capsule, tensorflow::kDlTensorCapsuleName)) {
            void* dlm_rptr =
                PyCapsule_GetPointer(capsule, tensorflow::kDlTensorCapsuleName);
            if (dlm_rptr) {
              tensorflow::TFE_CallDLManagedTensorDeleter(dlm_rptr);
              PyCapsule_SetDestructor(capsule, nullptr);
            }
          }
        });
    return capsule;
  });

  m.def("TFE_FromDlpackCapsule", [](const py::capsule& pycapsule,
                                    const py::handle& context) {
    tensorflow::Safe_TF_StatusPtr status =
        tensorflow::make_safe(TF_NewStatus());
    if (absl::string_view(pycapsule.name()) !=
        tensorflow::kDlTensorCapsuleName) {
      status->status = tensorflow::errors::InvalidArgument(
          "DLPack tensor must be a capsule with name \"dltensor\", got \"%s\". "
          "Note that a DLPack tensor may be consumed at most once.",
          absl::string_view(pycapsule.name()));
      tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
    }

    TFE_TensorHandle* thandle = tensorflow::TFE_HandleFromDLPack(
        pycapsule, status.get(), tensorflow::InputTFE_Context(context));

    tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());

    PyCapsule_SetName(pycapsule.ptr(), "used_dltensor");
    PyCapsule_SetDestructor(pycapsule.ptr(), nullptr);

    PyObject* pyhandle = EagerTensorFromHandle(thandle);
    return tensorflow::PyoOrThrow(pyhandle);
  });

  m.def("TFE_Py_RegisterCustomDevice", [](const py::handle& context,
                                          const py::capsule& device,
                                          const char* device_name,
                                          const py::capsule& device_info) {
    tensorflow::Safe_TF_StatusPtr status =
        tensorflow::make_safe(TF_NewStatus());
    if (absl::string_view(device.name()) != "TFE_CustomDevice") {
      status->status = tensorflow::errors::InvalidArgument(
          "Expected a capsule named 'TFE_CustomDevice' for the `device` "
          "argument, got ",
          absl::string_view(device.name()));
      tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
    }
    if (absl::string_view(device_info.name()) !=
        "TFE_CustomDevice_DeviceInfo") {
      status->status = tensorflow::errors::InvalidArgument(
          "Expected a capsule named 'TFE_CustomDevice_DeviceInfo' for "
          "the `device_info` argument, got ",
          absl::string_view(device_info.name()));
      tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
    }
    // TFE_RegisterCustomDevice takes ownership
    PyCapsule_SetDestructor(device_info.ptr(), nullptr);
    TFE_RegisterCustomDevice(
        tensorflow::InputTFE_Context(context),
        *reinterpret_cast<TFE_CustomDevice*>(
            PyCapsule_GetPointer(device.ptr(), "TFE_CustomDevice")),
        device_name,
        PyCapsule_GetPointer(device_info.ptr(), "TFE_CustomDevice_DeviceInfo"),
        status.get());
    tensorflow::MaybeRaiseRegisteredFromTFStatus(status.get());
  });

  py::class_<EagerContextThreadLocalDataWrapper>(m,
                                                 "EagerContextThreadLocalData")
      .def(py::init<py::handle, py::handle, py::handle>(),
           py::arg("py_eager_context"), py::arg("is_eager"),
           py::arg("device_spec"))
      .def_property("is_eager",
                    &EagerContextThreadLocalDataWrapper::get_is_eager,
                    &EagerContextThreadLocalDataWrapper::set_is_eager)
      .def_property(
          "invoking_op_callbacks",
          &EagerContextThreadLocalDataWrapper::get_invoking_op_callbacks,
          &EagerContextThreadLocalDataWrapper::set_invoking_op_callbacks)
      .def_property("device_name",
                    &EagerContextThreadLocalDataWrapper::get_device_name,
                    &EagerContextThreadLocalDataWrapper::set_device_name)
      .def_property("scope_name",
                    &EagerContextThreadLocalDataWrapper::get_scope_name,
                    &EagerContextThreadLocalDataWrapper::set_scope_name)
      .def_property("device_spec",
                    &EagerContextThreadLocalDataWrapper::get_device_spec,
                    &EagerContextThreadLocalDataWrapper::set_device_spec)
      .def_property(
          "function_call_options",
          &EagerContextThreadLocalDataWrapper::get_function_call_options,
          &EagerContextThreadLocalDataWrapper::set_function_call_options)
      .def_property("executor",
                    &EagerContextThreadLocalDataWrapper::get_executor,
                    &EagerContextThreadLocalDataWrapper::set_executor)
      .def_property("op_callbacks",
                    &EagerContextThreadLocalDataWrapper::get_op_callbacks,
                    &EagerContextThreadLocalDataWrapper::set_op_callbacks);

  // C API Enum

  py::enum_<TFE_ContextDevicePlacementPolicy>(
      m, "TFE_ContextDevicePlacementPolicy")
      .value("TFE_DEVICE_PLACEMENT_EXPLICIT", TFE_DEVICE_PLACEMENT_EXPLICIT)
      .value("TFE_DEVICE_PLACEMENT_WARN", TFE_DEVICE_PLACEMENT_WARN)
      .value("TFE_DEVICE_PLACEMENT_SILENT", TFE_DEVICE_PLACEMENT_SILENT)
      .value("TFE_DEVICE_PLACEMENT_SILENT_FOR_INT32",
             TFE_DEVICE_PLACEMENT_SILENT_FOR_INT32)
      .export_values();

  py::enum_<TF_AttrType>(m, "TF_AttrType")
      .value("TF_ATTR_STRING", TF_ATTR_STRING)
      .value("TF_ATTR_INT", TF_ATTR_INT)
      .value("TF_ATTR_FLOAT", TF_ATTR_FLOAT)
      .value("TF_ATTR_BOOL", TF_ATTR_BOOL)
      .value("TF_ATTR_TYPE", TF_ATTR_TYPE)
      .value("TF_ATTR_SHAPE", TF_ATTR_SHAPE)
      .value("TF_ATTR_TENSOR", TF_ATTR_TENSOR)
      .value("TF_ATTR_PLACEHOLDER", TF_ATTR_PLACEHOLDER)
      .value("TF_ATTR_FUNC", TF_ATTR_FUNC)
      .export_values();
};