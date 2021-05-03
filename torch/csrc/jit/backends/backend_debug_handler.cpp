#include <torch/csrc/jit/backends/backend_debug_handler.h>

namespace torch {
namespace jit {

namespace {
thread_local BackendDebugHandleManager* debug_handle_manager_ptr{nullptr};
} // namespace

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::atomic<DebugHandleType> BackendDebugHandleManager::unique_debug_handle_{0};

int64_t BackendDebugHandleManager::getNextDebugHandleForInlinedCallStackPtr(
    const Node* node) {
  InlinedCallStackPtr cs_ptr;
  if (node->callstack().has_value()) {
    cs_ptr = node->callstack().value();
  } else {
    cs_ptr = c10::intrusive_ptr<InlinedCallStack>();
  }
  DebugHandleType debug_handle = unique_debug_handle_;
  const SourceRange& range = node->sourceRange();
  handles_to_inlined_callstack_ptrs_[debug_handle] =
      std::make_tuple(range, node->kind().toQualString(), cs_ptr);
  // This increment is with seq memory order.
  // Not trying to perf optimizing this for now.
  unique_debug_handle_++;
  return debug_handle;
}

std::unordered_map<DebugHandleType, DebugInfoTuple> BackendDebugHandleManager::
    getCallStackPtrMap() {
  // Note that this is return by copy and since
  // InlinedCallStackPtrs are intrusive ptr it will result in
  // bump of refcount. Not performant, but this is not intented
  // to be used in perf critical path.
  // Alternate might be do move but that will be destructive
  return handles_to_inlined_callstack_ptrs_;
}

BackendModuleDebugInfoRecorder::BackendModuleDebugInfoRecorder(
    ObjectPtr module_ptr) {
  TORCH_CHECK(
      debug_handle_manager_ptr == nullptr,
      "Module debug recording already in progress.");
  debug_handle_manager_ptr = &debug_handle_manager;
  module_ptr_ = module_ptr;
}

BackendModuleDebugInfoRecorder::~BackendModuleDebugInfoRecorder() {
  // If due to some exception within preprocess, such as compilation failure
  // we throw, then we want to make sure the exit is clean
  debug_handle_manager_ptr = nullptr;
}

void BackendModuleDebugInfoRecorder::stopRecording() {
  getStaticBackendModuleDebugInfoMapPtr()->addDebugInfoMap(
      module_ptr_, std::move(debug_handle_manager_ptr->getCallStackPtrMap()));
  debug_handle_manager_ptr = nullptr;
}

BackendDebugHandleManager* getBackendDebugHandleManager() {
  return debug_handle_manager_ptr;
}

BackendModuleDebugInfoMap* getStaticBackendModuleDebugInfoMapPtr() {
  static BackendModuleDebugInfoMap module_debug_info_map;
  return &module_debug_info_map;
}

void BackendModuleDebugInfoMap::addDebugInfoMap(
    const ObjectPtr& ptr,
    DelegateDebugInfoMapType&& debug_map) {
  std::unique_lock<std::mutex> lock(debug_info_mutex_);
  TORCH_CHECK(
      debug_info_map_.count(ptr) == 0,
      "Debug info map already exists for the said module.");
  debug_info_map_.emplace(ptr, std::move(debug_map));
}

void BackendModuleDebugInfoMap::removeDebugInfoMap(const ObjectPtr& ptr) {
  std::unique_lock<std::mutex> lock(debug_info_mutex_);
  debug_info_map_.erase(ptr);
}

c10::optional<DelegateDebugInfoMapType> BackendModuleDebugInfoMap::
    getDebugInfoMap(const ObjectPtr& ptr) {
  std::unique_lock<std::mutex> lock(debug_info_mutex_);
  const auto& it = debug_info_map_.find(ptr);
  if (it == debug_info_map_.end()) {
    return c10::nullopt;
  }
  return it->second;
}
} // namespace jit
} // namespace torch