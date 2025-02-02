// Copyright (C) 2018-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

// #include "openvino/core/node.hpp"
#include "openvino/pass/pattern/op/pattern.hpp"

namespace ov {
namespace pass {
namespace pattern {
namespace op {
/// A submatch on the graph value which contains optional op types defined in constructor.
/// The match is succeed in case of full graphs matching or extended by one of optional type graph or pattern.
/// Otherwise fails.
/// Important note: graph can include only one optional op in the end of graph vs pattern.
/// Optional op can contain only 1 in and 1 out
class OPENVINO_API Optional : public Pattern {
public:
    OPENVINO_RTTI("patternOptional");
    /// \brief creates an optional node matching one pattern. Add nodes to match list.
    /// \param type_infos Optional operation types to exclude them from the matching
    /// in case the following op types do not exist in a pattern to match.
    /// \param patterns The pattern to match a graph.
    Optional(
        const std::vector<DiscreteTypeInfo>& type_infos,
        const Output<Node>& pattern,
        const pattern::op::ValuePredicate& pred =
            [](const Output<Node>& output) {
                return true;
            })
        : Pattern({pattern}, pred),
          optional_types(type_infos){};

    bool match_value(pattern::Matcher* matcher,
                     const Output<Node>& pattern_value,
                     const Output<Node>& graph_value) override;

    std::vector<DiscreteTypeInfo> get_optional_types() const;

protected:
    std::vector<DiscreteTypeInfo> optional_types;
};
}  // namespace op

template <class NodeType>
void collect_type_info(std::vector<DiscreteTypeInfo>& type_info_vec) {
    type_info_vec.push_back(NodeType::get_type_info_static());
}

template <class NodeType,
          class... NodeTypeArgs,
          typename std::enable_if<sizeof...(NodeTypeArgs) != 0, bool>::type = true>
void collect_type_info(std::vector<DiscreteTypeInfo>& type_info_vec) {
    collect_type_info<NodeType>(type_info_vec);
    collect_type_info<NodeTypeArgs...>(type_info_vec);
}

template <class... NodeTypes>
std::shared_ptr<Node> optional(const Output<Node>& input, const pattern::op::ValuePredicate& pred) {
    std::vector<DiscreteTypeInfo> optional_type_info_vec;
    collect_type_info<NodeTypes...>(optional_type_info_vec);
    return std::make_shared<op::Optional>(optional_type_info_vec, input, pred);
}

template <class... NodeTypes>
std::shared_ptr<Node> optional(const Output<Node>& input) {
    return optional<NodeTypes...>(input, [](const Output<Node>& output) {
        return true;
    });
}

}  // namespace pattern
}  // namespace pass
}  // namespace ov
