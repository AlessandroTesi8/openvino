// Copyright (C) 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "pass_manager.h"
#include "gather_inst.h"
#include "permute_inst.h"
#include "strided_slice_inst.h"
#include "kv_cache_inst.h"
#include "gemm_inst.h"
#include "program_helpers.h"

using namespace cldnn;

void mark_runtime_skippable_nodes::run(program& p) {
    auto itr = p.get_processing_order().begin();
    while (itr != p.get_processing_order().end()) {
        auto& node = *itr++;
        // Skip if the node is impl_types::cpu
        if (node->get_preferred_impl_type() == impl_types::cpu)
            return;
        // Set gathers that might be skipped at runtime as can_be_optimized.
        // If not set, memory dependency will not work for the nodes that are skipped at runtime
        program_helpers::do_for_types<gather>(*node, [](gather_node& node){
            // Check pattern
            auto impl_params = node.get_kernel_impl_params();
            if (node.has_fused_primitives() ||
                (impl_params->get_input_layout(0).data_type != impl_params->get_output_layout().data_type) ||
                node.get_dependency(1).is_constant() || node.get_dependency(1).is_type<data>())
                return;
            auto idx_rank = impl_params->get_input_layout(1).get_partial_shape().size();

            if (idx_rank != 1) {
                return;
            }
            auto axis = impl_params->typed_desc<gather>()->axis;
            if (impl_params->get_input_layout(0).get_partial_shape()[axis] == -1
                || impl_params->get_input_layout(1).get_partial_shape()[0] == -1
                || impl_params->get_input_layout(0).get_partial_shape()[axis] == impl_params->get_input_layout(1).get_partial_shape()[0]) {
                // May be skipepd
                node.can_be_optimized(true);
                GPU_DEBUG_TRACE_DETAIL << "[mark_runtime_skippable_nodes] : " << node.id() << " can_be_optimized" << std::endl;
            }
        });
        program_helpers::do_for_types<permute>(*node, [](permute_node& node){
            auto impl_params = node.get_kernel_impl_params();
            if (node.is_output() ||
                node.has_fused_primitives() ||
                (impl_params->get_input_layout(0).data_type != impl_params->get_output_layout().data_type))
                return;

            // TODO: For now, all permutes with dynamic shape are applied.
            //       A more detailed pattern will need to be applied later
            if (node.is_dynamic()) {
                if (node.get_dependency(0).is_type<kv_cache>())
                    return;
                // If the user is concatenation, priority should be given to in place concat optimization at runtime
                if (node.have_user_with_type<concatenation>() && node.get_users().size() == 1)
                    return;
                node.can_be_optimized(true);
                GPU_DEBUG_TRACE_DETAIL << "[mark_runtime_skippable_nodes] : " << node.id() << " can_be_optimized" << std::endl;
            }
        });
        program_helpers::do_for_types<strided_slice>(*node, [](strided_slice_node& node){
            auto impl_params = node.get_kernel_impl_params();
            if (node.is_output()
                || node.has_fused_primitives()
                || (impl_params->get_input_layout(0).format != impl_params->get_output_layout().format)
                || (impl_params->get_input_layout(0).data_type != impl_params->get_output_layout().data_type))
                return;

            auto prim = impl_params->typed_desc<strided_slice>();
            auto begin = prim->begin;
            auto strides = prim->strides;
            auto begin_mask = prim->begin_mask;
            if (prim->end_mask.empty()
                || !prim->new_axis_mask.empty()
                || !prim->shrink_axis_mask.empty()
                || !prim->ellipsis_mask.empty()
                || !(all_zeroes(begin) || all_ones(begin_mask))
                || !all_ones(strides))
                return;

            auto end = prim->end;
            auto end_mask = prim->end_mask;
            auto in_ps = impl_params->get_input_layout(0).get_partial_shape();
            bool is_valid = false;
            bool is_equal_size = (end.size() == end_mask.size());
            for (size_t i = 0; i < end.size(); i++) {
                if ((is_equal_size && end_mask[i] == 1) || (in_ps[i].is_static() && end[i] == in_ps[i].get_length())) {
                    is_valid = true;
                } else {
                    is_valid = false;
                }
            }
            if (!end.empty() && !is_valid)
                return;
            node.can_be_optimized(true);
            GPU_DEBUG_TRACE_DETAIL << "[mark_runtime_skippable_nodes] : " << node.id() << " can_be_optimized" << std::endl;
        });
    }
}
