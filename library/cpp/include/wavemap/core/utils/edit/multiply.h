#ifndef WAVEMAP_CORE_UTILS_EDIT_MULTIPLY_H_
#define WAVEMAP_CORE_UTILS_EDIT_MULTIPLY_H_

#include <memory>

#include "wavemap/core/common.h"
#include "wavemap/core/utils/thread_pool.h"

namespace wavemap::edit {
namespace detail {
template <typename MapT>
void multiplyNodeRecursive(typename MapT::Block::OctreeType::NodeRefType node,
                           FloatingPoint multiplier) {
  // Multiply
  node.data() *= multiplier;

  // Recursively handle all children
  for (int child_idx = 0; child_idx < OctreeIndex::kNumChildren; ++child_idx) {
    if (auto child_node = node.getChild(child_idx); child_node) {
      multiplyNodeRecursive<MapT>(*child_node, multiplier);
    }
  }
}
}  // namespace detail

template <typename MapT>
void multiply(MapT& map, FloatingPoint multiplier,
              const std::shared_ptr<ThreadPool>& thread_pool = nullptr) {
  using NodePtrType = typename MapT::Block::OctreeType::NodePtrType;

  // Process all blocks
  for (auto& [block_index, block] : map.getHashMap()) {
    // Indicate that the block has changed
    block.setLastUpdatedStamp();
    // Multiply the block's average value (wavelet scale coefficient)
    FloatingPoint& root_value = block.getRootScale();
    root_value *= multiplier;
    // Recursively multiply all node values (wavelet detail coefficients)
    NodePtrType root_node_ptr = &block.getRootNode();
    if (thread_pool) {
      thread_pool->add_task([root_node_ptr, multiplier]() {
        detail::multiplyNodeRecursive<MapT>(*root_node_ptr, multiplier);
      });
    } else {
      detail::multiplyNodeRecursive<MapT>(*root_node_ptr, multiplier);
    }
  }
  // Wait for all parallel jobs to finish
  if (thread_pool) {
    thread_pool->wait_all();
  }
}
}  // namespace wavemap::edit

#endif  // WAVEMAP_CORE_UTILS_EDIT_MULTIPLY_H_
