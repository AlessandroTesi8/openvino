// Copyright (C) 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//
// @file dnnl_utils.hpp
// Contains utility methods supposed used by oneDNN backend executors
//

#pragma once

#include "cpu_memory.h"
#include "memory_desc/dnnl_memory_desc.h"
#include "nodes/executors/executor.hpp"

namespace ov {
namespace intel_cpu {
namespace utils {
DnnlMemoryDescPtr makeTransposedWeightDescriptor(const DnnlMemoryDescPtr srcDesc, const DnnlMemoryDescPtr dstDesc);
MemoryPtr prepareWeightsMemory(const DnnlMemoryDescPtr srcWeightDesc,
                               const DnnlMemoryDescPtr dstWeightDesc,
                               const MemoryCPtr weightsMem,
                               const ExecutorContext::CPtr context);
}  // namespace utils
}  // namespace intel_cpu
}  // namespace ov
