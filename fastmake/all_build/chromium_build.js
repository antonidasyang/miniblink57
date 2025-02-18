import { constVal, buildCommonSetting } from "./const_val.js";

var json = [{
    "var":[
        {"sdkPath":constVal.sdkPath},
        {"clangPath":constVal.clangPath},
        {"srcPath":constVal.srcPath},
        {"ndkIncludePath":constVal.ndkIncludePath},
        {"ndkBinPath":constVal.ndkBinPath},
        {"v8dir": constVal.v8dir},
        {"targetDir": constVal.targetDir},
        {"sysroot": constVal.sysroot},
    ],
    "compile":{
        "ccompiler":"${clangPath}/clang.exe",
        "cppcompiler":"${clangPath}/clang++.exe",
            
        "include":[
//            "${sdkPath}/include/c++/7.2.0",
//            "${sdkPath}/include/c++/7.2.0/include",
//            "${ndkIncludePath}",
//            "${sysroot}/usr/include/glib-2.0",
//            "${sysroot}/usr/lib/x86_64-linux-gnu/glib-2.0/include/",
//            "${sdkPath}/include/c++/7.2.0/x86_64-unknown-linux-gnu/",
//            "${sdkPath}/sysroot/usr/include",
//            "${sdkPath}/sysroot/usr/",
//            "${sdkPath}/sysroot/usr/include/linux",
            ...constVal.includePaths,
            "${srcPath}",
            "${srcPath}/linux",
            "${srcPath}/gen/${v8dir}/include",
            "${srcPath}/${v8dir}",
            "${srcPath}/${v8dir}/include",
            "${srcPath}/gen",
            "${srcPath}/third_party/",
            "${srcPath}/third_party/npapi",
            "${srcPath}/third_party/WebKit",
            "${srcPath}/third_party/WebKit/Source",
            "${srcPath}/cef",
            "${srcPath}/third_party/icu/source/common",
            "${srcPath}/audio_manager_base/blink",
            "${srcPath}/gen/blink/core",
            "${srcPath}/gen/blink/bindings/modules/v8",
            "${srcPath}/gen/blink/bindings/core/v8",
            "${srcPath}/gen/third_party/WebKit",
            "${srcPath}/gpu",
            "${srcPath}/third_party/khronos",
            "${srcPath}/third_party/libxml/src/include",
            "${srcPath}/third_party/libxml/win32/include",
            "${srcPath}/third_party/skia/include/core",
            "${srcPath}/third_party/skia/include/config",
            "${srcPath}/third_party/skia/include/codec",
            "${srcPath}/third_party/skia/include/device",
            "${srcPath}/third_party/skia/include/effects",
            "${srcPath}/third_party/skia/include/images",
            "${srcPath}/third_party/skia/include/pathops",
            "${srcPath}/third_party/skia/include/ports",
            "${srcPath}/third_party/skia/include/private",
            "${srcPath}/third_party/skia/include/utils",
            "${srcPath}/third_party/skia/include/gpu",
            "${srcPath}/third_party/angle/include",
            "${srcPath}/third_party/angle/src"
        ],
        "prebuildSrc":[
            //"${srcPath}/base/strings/string16.cc",
            //"${srcPath}/base/message_loop/message_loop.cc",
            "${srcPath}/cc/output/software_output_device.cc",
            "${srcPath}/cc/raster/raster_buffer_provider.cc",
            //"${srcPath}/base/base_paths_posix.cc",
            //"${srcPath}/base/message_loop/message_pump_glib.cc"
        ],
        "src":[
            "${srcPath}/base/posix/safe_strerror.cc",
            "${srcPath}/base/at_exit.cc",
            "${srcPath}/base/barrier_closure.cc",
            "${srcPath}/base/base64url.cc",
            "${srcPath}/base/base64_base.cc",
            "${srcPath}/base/base_paths.cc",
            "${srcPath}/base/base_paths_posix.cc",
            "${srcPath}/base/base_switches.cc",
            "${srcPath}/base/big_endian.cc",
            "${srcPath}/base/bind_helpers.cc",
            "${srcPath}/base/build_time.cc",
            "${srcPath}/base/callback_helpers.cc",
            "${srcPath}/base/callback_internal.cc",
            //"${srcPath}/base/check_example.cc",
            "${srcPath}/base/command_line.cc",
            "${srcPath}/base/cpu.cc",
            "${srcPath}/base/deferred_sequenced_task_runner.cc",
            "${srcPath}/base/environment.cc",
            "${srcPath}/base/feature_list.cc",
            "${srcPath}/base/guid.cc",
            "${srcPath}/base/guid_posix.cc",
            "${srcPath}/base/hash.cc",
            "${srcPath}/base/lazy_instance.cc",
            "${srcPath}/base/location_base.cc",
            "${srcPath}/base/logging_base.cc",
            "${srcPath}/base/md5_base.cc",
            "${srcPath}/base/path_service.cc",
            "${srcPath}/base/pending_task.cc",
            "${srcPath}/base/pickle.cc",
            "${srcPath}/base/rand_util.cc",
            "${srcPath}/base/rand_util_posix.cc",
            "${srcPath}/base/run_loop.cc",
            "${srcPath}/base/scoped_native_library.cc",
            "${srcPath}/base/sequenced_task_runner.cc",
            "${srcPath}/base/sequence_checker_impl.cc",
            "${srcPath}/base/sha1_portable.cc",
            "${srcPath}/base/supports_user_data.cc",
            //"${srcPath}/base/sync_socket_win.cc",
            "${srcPath}/base/sys_info.cc",
            //"${srcPath}/base/sys_info_win.cc",
            "${srcPath}/base/task_runner.cc",
            "${srcPath}/base/thread_task_runner_handle.cc",
            "${srcPath}/base/tracked_objects.cc",
            "${srcPath}/base/tracking_info.cc",
            "${srcPath}/base/values.cc",
            "${srcPath}/base/value_conversions.cc",
            "${srcPath}/base/version.cc",
            "${srcPath}/base/vlog.cc",
            "${srcPath}/base/sys_info_linux.cc",
            "${srcPath}/base/sys_info_posix.cc",
            "${srcPath}/base/memory/aligned_memory.cc",
            "${srcPath}/base/memory/discardable_memory.cc",
            "${srcPath}/base/memory/discardable_memory_allocator.cc",
            "${srcPath}/base/memory/discardable_shared_memory.cc",
            "${srcPath}/base/memory/memory_pressure_listener.cc",
            "${srcPath}/base/memory/memory_pressure_monitor.cc",
            "${srcPath}/base/memory/ref_counted.cc",
            "${srcPath}/base/memory/ref_counted_memory.cc",
            "${srcPath}/base/memory/shared_memory_posix.cc",
            "${srcPath}/base/memory/singleton.cc",
            "${srcPath}/base/memory/weak_ptr.cc",
            "${srcPath}/base/debug/debugger_posix.cc",
            "${srcPath}/base/debug/stack_trace_posix.cc",
            "${srcPath}/base/debug/alias.cc",
            "${srcPath}/base/debug/asan_invalid_access.cc",
            "${srcPath}/base/debug/crash_logging.cc",
            "${srcPath}/base/debug/debugger.cc",
            "${srcPath}/base/debug/dump_without_crashing.cc",
            "${srcPath}/base/debug/profiler.cc",
            "${srcPath}/base/debug/stack_trace.cc",
            "${srcPath}/base/debug/task_annotator.cc",
            "${srcPath}/base/process/internal_linux.cc",
            "${srcPath}/base/process/kill.cc",
            "${srcPath}/base/process/kill_posix.cc",
            "${srcPath}/base/process/launch.cc",
            "${srcPath}/base/process/memory.cc",
            "${srcPath}/base/process/memory_linux.cc",
            "${srcPath}/base/process/process_linux.cc",
            "${srcPath}/base/process/process_handle.cc",
            "${srcPath}/base/process/process_handle_posix.cc",
            "${srcPath}/base/process/process_info_linux.cc",
            "${srcPath}/base/process/process_iterator.cc",
            "${srcPath}/base/process/process_iterator_linux.cc",
            "${srcPath}/base/process/process_metrics.cc",
            "${srcPath}/base/process/process_metrics_posix.cc",
            "${srcPath}/base/process/process_posix.cc",
            "${srcPath}/base/files/file_util_posix.cc",
            "${srcPath}/base/files/file_base.cc",
            "${srcPath}/base/files/file_enumerator.cc",
            "${srcPath}/base/files/file_enumerator_posix.cc",
            "${srcPath}/base/files/file_path.cc",
            "${srcPath}/base/files/file_path_constants.cc",
            "${srcPath}/base/files/file_path_watcher.cc",
            "${srcPath}/base/files/file_path_watcher_linux.cc",
            "${srcPath}/base/files/file_proxy.cc",
            "${srcPath}/base/files/file_tracing.cc",
            "${srcPath}/base/files/file_util.cc",
            "${srcPath}/base/files/file_util_linux.cc",
            "${srcPath}/base/files/file_posix.cc",
            "${srcPath}/base/files/important_file_writer.cc",
            "${srcPath}/base/files/memory_mapped_file.cc",
            "${srcPath}/base/files/memory_mapped_file_posix.cc",
            "${srcPath}/base/files/scoped_file.cc",
            "${srcPath}/base/files/scoped_temp_dir.cc",
            "${srcPath}/base/json/json_file_value_serializer.cc",
            "${srcPath}/base/json/json_parser.cc",
            "${srcPath}/base/json/json_reader.cc",
            "${srcPath}/base/json/json_string_value_serializer.cc",
            "${srcPath}/base/json/json_value_converter.cc",
            "${srcPath}/base/json/json_writer.cc",
            "${srcPath}/base/json/string_escape.cc",
            "${srcPath}/base/message_loop/incoming_task_queue.cc",
            "${srcPath}/base/message_loop/message_loop.cc",
            "${srcPath}/base/message_loop/message_loop_task_runner.cc",
            "${srcPath}/base/message_loop/message_pump.cc",
            "${srcPath}/base/message_loop/message_pump_default.cc",
            "${srcPath}/base/message_loop/message_pump_glib.cc",
            "${srcPath}/base/profiler/alternate_timer.cc",
            "${srcPath}/base/profiler/scoped_profile.cc",
            "${srcPath}/base/profiler/scoped_tracker.cc",
            "${srcPath}/base/profiler/tracked_time.cc",
            "${srcPath}/base/strings/latin1_string_conversions.cc",
            "${srcPath}/base/strings/nullable_string16.cc",
            "${srcPath}/base/strings/string16.cc",
            "${srcPath}/base/strings/pattern_base.cc",
            "${srcPath}/base/strings/safe_sprintf.cc",
            "${srcPath}/base/strings/stringprintf.cc",
            "${srcPath}/base/strings/string_number_conversions.cc",
            "${srcPath}/base/strings/string_piece.cc",
            "${srcPath}/base/strings/string_split.cc",
            "${srcPath}/base/strings/string_util.cc",
            "${srcPath}/base/strings/string_util_constants.cc",
            "${srcPath}/base/strings/sys_string_conversions_posix.cc",
            "${srcPath}/base/strings/utf_offset_string_conversions.cc",
            "${srcPath}/base/strings/utf_string_conversions.cc",
            "${srcPath}/base/strings/utf_string_conversion_utils.cc",
            "${srcPath}/base/synchronization/cancellation_flag.cc",
            "${srcPath}/base/synchronization/condition_variable_posix.cc",
            "${srcPath}/base/synchronization/lock.cc",
            "${srcPath}/base/synchronization/lock_impl_posix.cc",
            "${srcPath}/base/synchronization/waitable_event_watcher_posix.cc",
            "${srcPath}/base/synchronization/waitable_event_posix.cc",
            "${srcPath}/base/threading/non_thread_safe_impl.cc",
            "${srcPath}/base/threading/platform_thread_internal_posix.cc",
            "${srcPath}/base/threading/platform_thread_linux.cc",
            "${srcPath}/base/threading/platform_thread_posix.cc",
            "${srcPath}/base/threading/post_task_and_reply_impl.cc",
            "${srcPath}/base/threading/sequenced_task_runner_handle.cc",
            "${srcPath}/base/threading/sequenced_worker_pool.cc",
            "${srcPath}/base/threading/simple_thread.cc",
            "${srcPath}/base/threading/thread.cc",
            "${srcPath}/base/threading/thread_checker_impl.cc",
            "${srcPath}/base/threading/thread_collision_warner.cc",
            "${srcPath}/base/threading/thread_id_name_manager.cc",
            "${srcPath}/base/threading/thread_local_storage.cc",
            "${srcPath}/base/threading/thread_local_storage_posix.cc",
            "${srcPath}/base/threading/thread_local_posix.cc",
            "${srcPath}/base/threading/thread_restrictions.cc",
            "${srcPath}/base/threading/watchdog.cc",
            "${srcPath}/base/threading/worker_pool.cc",
            "${srcPath}/base/threading/worker_pool_posix.cc",
            "${srcPath}/base/time/clock_base.cc",
            "${srcPath}/base/time/default_clock.cc",
            "${srcPath}/base/time/default_tick_clock.cc",
            "${srcPath}/base/time/tick_clock.cc",
            "${srcPath}/base/time/time.cc",
            "${srcPath}/base/time/time_posix.cc",
            "${srcPath}/base/timer/elapsed_timer.cc",
            "${srcPath}/base/timer/hi_res_timer_manager_posix.cc",
            "${srcPath}/base/timer/mock_timer.cc",
            "${srcPath}/base/timer/timer_base.cc",
            "${srcPath}/base/trace_event/heap_profiler_allocation_context.cc",
            "${srcPath}/base/trace_event/heap_profiler_allocation_context_tracker.cc",
            "${srcPath}/base/trace_event/heap_profiler_allocation_register.cc",
            "${srcPath}/base/trace_event/heap_profiler_allocation_register_posix.cc",
            "${srcPath}/base/trace_event/heap_profiler_heap_dump_writer.cc",
            "${srcPath}/base/trace_event/heap_profiler_stack_frame_deduplicator.cc",
            "${srcPath}/base/trace_event/heap_profiler_type_name_deduplicator.cc",
            "${srcPath}/base/trace_event/malloc_dump_provider.cc",
            "${srcPath}/base/trace_event/memory_allocator_dump.cc",
            "${srcPath}/base/trace_event/memory_allocator_dump_guid.cc",
            "${srcPath}/base/trace_event/memory_dump_manager.cc",
            "${srcPath}/base/trace_event/memory_dump_request_args.cc",
            "${srcPath}/base/trace_event/memory_dump_session_state.cc",
            "${srcPath}/base/trace_event/memory_infra_background_whitelist.cc",
            "${srcPath}/base/trace_event/process_memory_dump.cc",
            "${srcPath}/base/trace_event/process_memory_maps.cc",
            "${srcPath}/base/trace_event/process_memory_totals.cc",
            "${srcPath}/base/trace_event/trace_buffer.cc",
            "${srcPath}/base/trace_event/trace_config.cc",
            "${srcPath}/base/trace_event/trace_event_argument.cc",
            "${srcPath}/base/trace_event/trace_event_impl.cc",
            "${srcPath}/base/trace_event/trace_event_memory_overhead.cc",
            "${srcPath}/base/trace_event/trace_event_synthetic_delay.cc",
            "${srcPath}/base/trace_event/trace_event_system_stats_monitor.cc",
            "${srcPath}/base/trace_event/trace_log.cc",
            "${srcPath}/base/trace_event/trace_log_constants.cc",
            "${srcPath}/base/trace_event/trace_sampling_thread.cc",
            "${srcPath}/base/trace_event/tracing_agent.cc",
            "${srcPath}/base/trace_event/blame_context.cc",
            "${srcPath}/base/third_party/icu/icu_utf.cc",
            "${srcPath}/base/third_party/dmg_fp/dtoa_wrapper.cc",
            "${srcPath}/base/third_party/dmg_fp/g_fmt.cc",
            "${srcPath}/base/third_party/dynamic_annotations/dynamic_annotations.cpp",
            "${srcPath}/base/third_party/nspr/prtime.cc",
            "${srcPath}/cc/blimp/picture_data.cc",
            "${srcPath}/base/optional.cc",
            "${srcPath}/base/logging_empty.cc",
            "${srcPath}/base/metrics/histogram_base.cc",
            "${srcPath}/base/metrics/histogram.cc",
            "${srcPath}/base/metrics/histogram_samples.cc",
            "${srcPath}/base/metrics/histogram_snapshot_manager.cc",
            "${srcPath}/base/metrics/metrics_hashes.cc",
            "${srcPath}/base/metrics/persistent_histogram_allocator.cc",
            "${srcPath}/base/metrics/persistent_memory_allocator.cc",
            "${srcPath}/base/metrics/persistent_sample_map.cc",
            "${srcPath}/base/metrics/sample_map.cc",
            "${srcPath}/base/metrics/sample_vector.cc",
            "${srcPath}/base/metrics/sparse_histogram.cc",
            "${srcPath}/base/metrics/statistics_recorder.cc",
            "${srcPath}/base/metrics/user_metrics.cc",
            "${srcPath}/base/metrics/bucket_ranges.cc",
            "${srcPath}/base/metrics/field_trial.cc",
            "${srcPath}/base/metrics/histogram_delta_serialization.cc",
            "${srcPath}/base/trace_event/trace_event_etw_export_win_empty.cc",
            "${srcPath}/base/synchronization/read_write_lock_posix.cc",
            "${srcPath}/base/unguessable_token.cc",
            "${srcPath}/cc/animation/scroll_offset_animations_impl.cc",
            "${srcPath}/cc/animation/timing_function.cc",
            "${srcPath}/cc/animation/transform_operation.cc",
            "${srcPath}/cc/animation/transform_operations.cc",
            "${srcPath}/cc/animation/animation_cc.cc",
            "${srcPath}/cc/animation/animation_curve.cc",
            "${srcPath}/cc/animation/animation_events.cc",
            "${srcPath}/cc/animation/animation_host.cc",
            "${srcPath}/cc/animation/animation_id_provider.cc",
            "${srcPath}/cc/animation/animation_player.cc",
            "${srcPath}/cc/animation/animation_timeline.cc",
            "${srcPath}/cc/animation/element_animations.cc",
            "${srcPath}/cc/animation/keyframed_animation_curve.cc",
            "${srcPath}/cc/animation/scroll_offset_animation_curve.cc",
            "${srcPath}/cc/animation/scroll_offset_animations.cc",
            "${srcPath}/cc/base/contiguous_container.cc",
            "${srcPath}/cc/base/delayed_unique_notifier.cc",
            "${srcPath}/cc/base/invalidation_region.cc",
            "${srcPath}/cc/base/histograms.cc",
            "${srcPath}/cc/base/index_rect.cc",
            "${srcPath}/cc/base/math_util.cc",
            "${srcPath}/cc/base/list_container_helper.cc",
            "${srcPath}/cc/base/reverse_spiral_iterator.cc",
            "${srcPath}/cc/base/rolling_time_delta_history.cc",
            "${srcPath}/cc/base/region.cc",
            "${srcPath}/cc/base/simple_enclosed_region.cc",
            "${srcPath}/cc/base/rtree.cc",
            "${srcPath}/cc/base/switches.cc",
            "${srcPath}/cc/base/spiral_iterator.cc",
            "${srcPath}/cc/base/tiling_data.cc",
            "${srcPath}/cc/base/unique_notifier.cc",
            "${srcPath}/cc/blink/web_image_layer_impl.cc",
            "${srcPath}/cc/blink/web_layer_impl.cc",
            "${srcPath}/cc/blink/web_layer_impl_fixed_bounds.cc",
            "${srcPath}/cc/blink/web_scrollbar_layer_impl.cc",
            "${srcPath}/cc/blink/scrollbar_impl.cc",
            "${srcPath}/cc/blink/web_compositor_support_impl.cc",
            "${srcPath}/cc/blink/web_content_layer_impl.cc",
            "${srcPath}/cc/blink/web_display_item_list_impl.cc",
            "${srcPath}/cc/blink/web_external_texture_layer_impl.cc",
            "${srcPath}/cc/debug/frame_viewer_instrumentation.cc",
            "${srcPath}/cc/debug/invalidation_benchmark.cc",
            "${srcPath}/cc/debug/lap_timer.cc",
            "${srcPath}/cc/debug/layer_tree_debug_state.cc",
            "${srcPath}/cc/debug/micro_benchmark.cc",
            "${srcPath}/cc/debug/micro_benchmark_controller.cc",
            "${srcPath}/cc/debug/micro_benchmark_controller_impl.cc",
            "${srcPath}/cc/debug/micro_benchmark_impl.cc",
            "${srcPath}/cc/debug/picture_debug_util.cc",
            "${srcPath}/cc/debug/rasterize_and_record_benchmark.cc",
            "${srcPath}/cc/debug/rasterize_and_record_benchmark_impl.cc",
            "${srcPath}/cc/debug/rendering_stats.cc",
            "${srcPath}/cc/debug/rendering_stats_instrumentation.cc",
            "${srcPath}/cc/debug/traced_display_item_list.cc",
            "${srcPath}/cc/debug/traced_value.cc",
            "${srcPath}/cc/debug/unittest_only_benchmark.cc",
            "${srcPath}/cc/debug/unittest_only_benchmark_impl.cc",
            "${srcPath}/cc/debug/benchmark_instrumentation.cc",
            "${srcPath}/cc/debug/debug_colors.cc",
            "${srcPath}/cc/debug/debug_rect_history.cc",
            "${srcPath}/cc/debug/devtools_instrumentation.cc",
            "${srcPath}/cc/debug/frame_rate_counter.cc",
            "${srcPath}/cc/input/input_handler.cc",
            "${srcPath}/cc/input/page_scale_animation.cc",
            "${srcPath}/cc/input/scroll_elasticity_helper.cc",
            "${srcPath}/cc/input/scroll_state.cc",
            "${srcPath}/cc/input/scroll_state_data.cc",
            "${srcPath}/cc/input/scrollbar_animation_controller.cc",
            "${srcPath}/cc/input/scrollbar_animation_controller_linear_fade.cc",
            "${srcPath}/cc/input/scrollbar_animation_controller_thinning.cc",
            "${srcPath}/cc/input/browser_controls_offset_manager.cc",
            "${srcPath}/cc/layers/layer.cc",
            "${srcPath}/cc/layers/layer_impl.cc",
            "${srcPath}/cc/layers/layer_impl_test_properties.cc",
            "${srcPath}/cc/layers/layer_list_iterator.cc",
            "${srcPath}/cc/layers/layer_position_constraint.cc",
            "${srcPath}/cc/layers/layer_sticky_position_constraint.cc",
            "${srcPath}/cc/layers/layer_utils.cc",
            "${srcPath}/cc/layers/nine_patch_layer.cc",
            "${srcPath}/cc/layers/nine_patch_layer_impl.cc",
            "${srcPath}/cc/layers/painted_scrollbar_layer.cc",
            "${srcPath}/cc/layers/painted_scrollbar_layer_impl.cc",
            "${srcPath}/cc/layers/picture_image_layer.cc",
            "${srcPath}/cc/layers/picture_layer.cc",
            "${srcPath}/cc/layers/picture_layer_impl.cc",
            "${srcPath}/cc/layers/render_surface_impl.cc",
            "${srcPath}/cc/layers/scrollbar_layer_impl_base.cc",
            "${srcPath}/cc/layers/solid_color_layer.cc",
            "${srcPath}/cc/layers/solid_color_layer_impl.cc",
            "${srcPath}/cc/layers/solid_color_scrollbar_layer.cc",
            "${srcPath}/cc/layers/solid_color_scrollbar_layer_impl.cc",
            "${srcPath}/cc/layers/surface_layer.cc",
            "${srcPath}/cc/layers/surface_layer_impl.cc",
            "${srcPath}/cc/layers/texture_layer.cc",
            "${srcPath}/cc/layers/texture_layer_impl.cc",
            "${srcPath}/cc/layers/ui_resource_layer.cc",
            "${srcPath}/cc/layers/ui_resource_layer_impl.cc",
            "${srcPath}/cc/layers/viewport.cc",
            "${srcPath}/cc/layers/draw_properties.cc",
            "${srcPath}/cc/layers/empty_content_layer_client.cc",
            "${srcPath}/cc/layers/heads_up_display_layer.cc",
            "${srcPath}/cc/layers/heads_up_display_layer_impl.cc",
            "${srcPath}/cc/output/compositor_frame.cc",
            "${srcPath}/cc/output/compositor_frame_metadata.cc",
            "${srcPath}/cc/output/compositor_frame_sink.cc",
            "${srcPath}/cc/output/context_cache_controller.cc",
            "${srcPath}/cc/output/context_provider.cc",
            "${srcPath}/cc/output/copy_output_request.cc",
            "${srcPath}/cc/output/copy_output_result.cc",
            "${srcPath}/cc/output/direct_renderer.cc",
            "${srcPath}/cc/output/dynamic_geometry_binding.cc",
            "${srcPath}/cc/output/filter_operation.cc",
            "${srcPath}/cc/output/filter_operations.cc",
            "${srcPath}/cc/output/geometry_binding.cc",
            "${srcPath}/cc/output/latency_info_swap_promise.cc",
            "${srcPath}/cc/output/layer_quad.cc",
            "${srcPath}/cc/output/managed_memory_policy.cc",
            "${srcPath}/cc/output/output_surface.cc",
            "${srcPath}/cc/output/output_surface_frame.cc",
            "${srcPath}/cc/output/overlay_candidate.cc",
            "${srcPath}/cc/output/overlay_processor.cc",
            "${srcPath}/cc/output/overlay_strategy_fullscreen.cc",
            "${srcPath}/cc/output/overlay_strategy_single_on_top.cc",
            "${srcPath}/cc/output/overlay_strategy_underlay.cc",
            "${srcPath}/cc/output/program_binding.cc",
            "${srcPath}/cc/output/render_surface_filters.cc",
            "${srcPath}/cc/output/renderer_settings.cc",
            "${srcPath}/cc/output/shader.cc",
            "${srcPath}/cc/output/software_output_device.cc",
            "${srcPath}/cc/output/software_renderer.cc",
            "${srcPath}/cc/output/static_geometry_binding.cc",
            "${srcPath}/cc/output/texture_mailbox_deleter.cc",
            "${srcPath}/cc/output/vulkan_in_process_context_provider.cc",
            "${srcPath}/cc/output/begin_frame_args.cc",
            "${srcPath}/cc/output/bsp_tree.cc",
            "${srcPath}/cc/output/bsp_walk_action.cc",
            "${srcPath}/cc/output/buffer_to_texture_target_map.cc",
            "${srcPath}/cc/output/ca_layer_overlay.cc",
            "${srcPath}/cc/playback/decoded_draw_image.cc",
            "${srcPath}/cc/playback/discardable_image_map.cc",
            "${srcPath}/cc/playback/display_item.cc",
            "${srcPath}/cc/playback/display_item_list.cc",
            "${srcPath}/cc/playback/display_item_list_settings.cc",
            "${srcPath}/cc/playback/display_item_proto_factory.cc",
            "${srcPath}/cc/playback/draw_image.cc",
            "${srcPath}/cc/playback/drawing_display_item.cc",
            "${srcPath}/cc/playback/filter_display_item.cc",
            "${srcPath}/cc/playback/float_clip_display_item.cc",
            "${srcPath}/cc/playback/image_hijack_canvas.cc",
            "${srcPath}/cc/playback/largest_display_item.cc",
            "${srcPath}/cc/playback/raster_source.cc",
            "${srcPath}/cc/playback/recording_source.cc",
            "${srcPath}/cc/playback/skip_image_canvas.cc",
            "${srcPath}/cc/playback/transform_display_item.cc",
            "${srcPath}/cc/playback/clip_display_item.cc",
            "${srcPath}/cc/playback/clip_path_display_item.cc",
            "${srcPath}/cc/playback/compositing_display_item.cc",
            "${srcPath}/cc/quads/surface_draw_quad.cc",
            "${srcPath}/cc/quads/texture_draw_quad.cc",
            "${srcPath}/cc/quads/tile_draw_quad.cc",
            "${srcPath}/cc/quads/yuv_video_draw_quad.cc",
            "${srcPath}/cc/quads/content_draw_quad_base.cc",
            "${srcPath}/cc/quads/debug_border_draw_quad.cc",
            "${srcPath}/cc/quads/draw_polygon.cc",
            "${srcPath}/cc/quads/draw_quad.cc",
            "${srcPath}/cc/quads/largest_draw_quad.cc",
            "${srcPath}/cc/quads/picture_draw_quad.cc",
            "${srcPath}/cc/quads/render_pass.cc",
            "${srcPath}/cc/quads/render_pass_draw_quad.cc",
            "${srcPath}/cc/quads/shared_quad_state.cc",
            "${srcPath}/cc/quads/solid_color_draw_quad.cc",
            "${srcPath}/cc/quads/stream_video_draw_quad.cc",
            "${srcPath}/cc/raster/single_thread_task_graph_runner.cc",
            "${srcPath}/cc/raster/synchronous_task_graph_runner.cc",
            "${srcPath}/cc/raster/task.cc",
            "${srcPath}/cc/raster/task_graph_work_queue.cc",
            "${srcPath}/cc/raster/texture_compressor.cc",
            "${srcPath}/cc/raster/texture_compressor_etc1.cc",
            "${srcPath}/cc/raster/texture_compressor_etc1_sse.cc",
            "${srcPath}/cc/raster/tile_task.cc",
            "${srcPath}/cc/raster/zero_copy_raster_buffer_provider.cc",
            "${srcPath}/cc/raster/bitmap_raster_buffer_provider.cc",
            "${srcPath}/cc/raster/raster_buffer.cc",
            "${srcPath}/cc/raster/raster_buffer_provider.cc",
            "${srcPath}/cc/raster/scoped_gpu_raster.cc",
            "${srcPath}/cc/scheduler/scheduler_settings.cc",
            "${srcPath}/cc/scheduler/scheduler_state_machine.cc",
            "${srcPath}/cc/scheduler/begin_frame_source.cc",
            "${srcPath}/cc/scheduler/begin_frame_tracker.cc",
            "${srcPath}/cc/scheduler/compositor_timing_history.cc",
            "${srcPath}/cc/scheduler/delay_based_time_source.cc",
            "${srcPath}/cc/scheduler/scheduler.cc",
            "${srcPath}/cc/resources/transferable_resource.cc",
            "${srcPath}/cc/resources/ui_resource_bitmap.cc",
            "${srcPath}/cc/resources/ui_resource_manager.cc",
            "${srcPath}/cc/resources/ui_resource_request.cc",
            "${srcPath}/cc/resources/memory_history.cc",
            "${srcPath}/cc/resources/resource_format.cc",
            "${srcPath}/cc/resources/resource_format_utils.cc",
            "${srcPath}/cc/resources/resource_pool.cc",
            "${srcPath}/cc/resources/resource_provider.cc",
            "${srcPath}/cc/resources/scoped_resource.cc",
            "${srcPath}/cc/resources/scoped_ui_resource.cc",
            "${srcPath}/cc/resources/shared_bitmap.cc",
            "${srcPath}/cc/resources/single_release_callback.cc",
            "${srcPath}/cc/resources/single_release_callback_impl.cc",
            "${srcPath}/cc/resources/texture_mailbox.cc",
            "${srcPath}/cc/surfaces/surface_resource_holder.cc",
            "${srcPath}/cc/surfaces/surface_sequence_generator.cc",
            "${srcPath}/cc/surfaces/compositor_frame_sink_support.cc",
            "${srcPath}/cc/surfaces/direct_compositor_frame_sink.cc",
            "${srcPath}/cc/surfaces/direct_surface_reference_factory.cc",
            "${srcPath}/cc/surfaces/display.cc",
            "${srcPath}/cc/surfaces/display_scheduler.cc",
            "${srcPath}/cc/surfaces/frame_sink_id.cc",
            "${srcPath}/cc/surfaces/local_frame_id.cc",
            "${srcPath}/cc/surfaces/referenced_surface_tracker.cc",
            "${srcPath}/cc/surfaces/sequence_surface_reference_factory.cc",
            "${srcPath}/cc/surfaces/surface.cc",
            "${srcPath}/cc/surfaces/surface_aggregator.cc",
            "${srcPath}/cc/surfaces/surface_factory.cc",
            "${srcPath}/cc/surfaces/surface_hittest.cc",
            "${srcPath}/cc/surfaces/surface_id.cc",
            "${srcPath}/cc/surfaces/surface_id_allocator.cc",
            "${srcPath}/cc/surfaces/surface_manager.cc",
            "${srcPath}/cc/surfaces/surface_reference.cc",
            "${srcPath}/cc/tiles/tiling_set_raster_queue_required.cc",
            "${srcPath}/cc/tiles/decoded_image_tracker.cc",
            "${srcPath}/cc/tiles/eviction_tile_priority_queue.cc",
            "${srcPath}/cc/tiles/image_controller.cc",
            "${srcPath}/cc/tiles/mipmap_util.cc",
            "${srcPath}/cc/tiles/picture_layer_tiling.cc",
            "${srcPath}/cc/tiles/picture_layer_tiling_set.cc",
            "${srcPath}/cc/tiles/prioritized_tile.cc",
            "${srcPath}/cc/tiles/raster_tile_priority_queue.cc",
            "${srcPath}/cc/tiles/raster_tile_priority_queue_all.cc",
            "${srcPath}/cc/tiles/raster_tile_priority_queue_required.cc",
            "${srcPath}/cc/tiles/software_image_decode_cache.cc",
            "${srcPath}/cc/tiles/tile.cc",
            "${srcPath}/cc/tiles/tile_draw_info.cc",
            "${srcPath}/cc/tiles/tile_manager.cc",
            "${srcPath}/cc/tiles/tile_priority.cc",
            "${srcPath}/cc/tiles/tile_task_manager.cc",
            "${srcPath}/cc/tiles/tiling_set_eviction_queue.cc",
            "${srcPath}/cc/tiles/tiling_set_raster_queue_all.cc",
            "${srcPath}/cc/trees/transform_node.cc",
            "${srcPath}/cc/trees/tree_synchronizer.cc",
            "${srcPath}/cc/trees/blocking_task_runner.cc",
            "${srcPath}/cc/trees/clip_expander.cc",
            "${srcPath}/cc/trees/clip_node.cc",
            "${srcPath}/cc/trees/damage_tracker.cc",
            "${srcPath}/cc/trees/draw_property_utils.cc",
            "${srcPath}/cc/trees/effect_node.cc",
            "${srcPath}/cc/trees/element_id.cc",
            "${srcPath}/cc/trees/latency_info_swap_promise_monitor.cc",
            "${srcPath}/cc/trees/layer_tree.cc",
            "${srcPath}/cc/trees/layer_tree_host_common.cc",
            "${srcPath}/cc/trees/layer_tree_host_impl.cc",
            "${srcPath}/cc/trees/layer_tree_host_in_process.cc",
            "${srcPath}/cc/trees/layer_tree_impl.cc",
            "${srcPath}/cc/trees/layer_tree_settings.cc",
            "${srcPath}/cc/trees/occlusion.cc",
            "${srcPath}/cc/trees/occlusion_tracker.cc",
            "${srcPath}/cc/trees/property_animation_state.cc",
            "${srcPath}/cc/trees/property_tree.cc",
            "${srcPath}/cc/trees/property_tree_builder.cc",
            "${srcPath}/cc/trees/proxy_common.cc",
            "${srcPath}/cc/trees/proxy_impl.cc",
            "${srcPath}/cc/trees/proxy_main.cc",
            "${srcPath}/cc/trees/scroll_node.cc",
            "${srcPath}/cc/trees/single_thread_proxy.cc",
            "${srcPath}/cc/trees/swap_promise_manager.cc",
            "${srcPath}/cc/trees/swap_promise_monitor.cc",
            "${srcPath}/cc/trees/target_property.cc",
            "${srcPath}/cc/trees/task_runner_provider.cc",
            "${srcPath}/cc/input/layer_selection_bound.cc",
            "${srcPath}/base/sequence_token.cc",
            "${srcPath}/base/device/base/synchronization/one_writer_seqlock.cc",
            "${srcPath}/url/gurl.cc",
            "${srcPath}/url/origin.cc",
            "${srcPath}/url/scheme_host_port.cc",
            "${srcPath}/url/url_canon_etc.cc",
            "${srcPath}/url/url_canon_filesystemurl.cc",
            "${srcPath}/url/url_canon_fileurl.cc",
            "${srcPath}/url/url_canon_host.cc",
            "${srcPath}/url/url_canon_internal.cc",
            "${srcPath}/url/url_canon_ip.cc",
            "${srcPath}/url/url_canon_mailtourl.cc",
            "${srcPath}/url/url_canon_path.cc",
            "${srcPath}/url/url_canon_pathurl.cc",
            "${srcPath}/url/url_canon_query.cc",
            "${srcPath}/url/url_canon_relative.cc",
            "${srcPath}/url/url_canon_stdstring.cc",
            "${srcPath}/url/url_canon_stdurl.cc",
            "${srcPath}/url/url_constants.cc",
            "${srcPath}/url/url_parse_file.cc",
            "${srcPath}/url/url_util.cc",
            "${srcPath}/url/third_party/mozilla/url_parse.cc",
            "${srcPath}/base/WindowsVersion.cpp",
            "${srcPath}/cc/layers/video_frame_provider_client_impl.cc",
            "${srcPath}/cc/layers/video_layer.cc",
            "${srcPath}/cc/layers/video_layer_impl.cc",
            "${srcPath}/base/power_monitor/power_monitor_device_source.cc",
            "${srcPath}/base/power_monitor/power_monitor_device_source_posix.cc",
            "${srcPath}/base/power_monitor/power_monitor_source.cc",
            "${srcPath}/base/power_monitor/power_monitor.cc",
            "${srcPath}/media/filters/source_buffer_state.cc",
            "${srcPath}/media/filters/audio_timestamp_validator.cc",
            "${srcPath}/media/formats/webm/webm_parser.cc",
            "${srcPath}/media/formats/webm/webm_stream_parser.cc",
            "${srcPath}/media/formats/webm/webm_tracks_parser.cc",
            "${srcPath}/media/formats/webm/webm_video_client.cc",
            "${srcPath}/media/formats/webm/webm_webvtt_parser.cc",
            "${srcPath}/media/formats/webm/cluster_builder.cc",
            "${srcPath}/media/formats/webm/opus_packet_builder.cc",
            "${srcPath}/media/formats/webm/tracks_builder.cc",
            "${srcPath}/media/formats/webm/webm_audio_client.cc",
            "${srcPath}/media/formats/webm/webm_cluster_parser.cc",
            "${srcPath}/media/formats/webm/webm_colour_parser.cc",
            "${srcPath}/media/formats/webm/webm_constants.cc",
            "${srcPath}/media/formats/webm/webm_content_encodings.cc",
            "${srcPath}/media/formats/webm/webm_content_encodings_client.cc",
            "${srcPath}/media/formats/webm/webm_crypto_helpers.cc",
            "${srcPath}/media/formats/webm/webm_info_parser.cc",
            "${srcPath}/media/video/picture.cc",
            "${srcPath}/cc/resources/video_resource_updater.cc",
            "${srcPath}/media/audio/audio_device_description.cc",
            "${srcPath}/media/blink/resource_multibuffer_data_provider.cc"
        ],
        // 
        "cmd":[
            //"--target=x86_64-linux-guneabi", 
            "-std=c++14",
            "-fms-extensions",
            "-D_GNU_SOURCE",
            "-DUSE_GLIB=1",
            "-DUSE_AURA=1",
            "-DOS_LINUX_FOR_WIN=1",
            "-D_HAS_CONSTEXPR=0",
            "-DNO_TCMALLOC=1",
            "-DANGLE_ENABLE_GLSL=1",
            "-DBUILDING_CEF_SHARED",
            "-DUSING_CEF_SHARED",
            "-DMEDIA_IMPLEMENTATION=1",
            "-DUSE_PROPRIETARY_CODECS=1",
            "-D_LIB"
        ],
        "objdir":"${srcPath}/out/tmp/chromium/${targetDir}",
        "outdir":"${srcPath}/out/${targetDir}",
                
        "target":"libchromium.a",
        "beginLibs":[
        ],
        "linkerCmd":[],
        "endLibs":[
        ],
        "linker":"${ndkBinPath}/ar.exe"
    }
}];

buildCommonSetting(json);
