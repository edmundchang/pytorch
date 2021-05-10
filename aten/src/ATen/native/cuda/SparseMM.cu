#include <ATen/ATen.h>
#include <ATen/NativeFunctions.h>
#include <c10/util/Exception.h>

namespace at {
namespace native {
// sparse, sparse, sparse, dense, real, real -> sparse
Tensor& _sspaddmm_out_only_sparse_cuda(
    const Tensor& self,
    const Tensor& mat1,
    const Tensor& mat2,
    const Scalar& beta,
    const Scalar& alpha,
    Tensor& result) {
  AT_ERROR("tensor.sspaddmm(...) can only be called on sparse tensors");
}
Tensor& _sspaddmm_out_cuda(
    const Tensor& self,
    const Tensor& mat1,
    const Tensor& mat2,
    const Scalar& beta,
    const Scalar& alpha,
    Tensor& result) {
  AT_ERROR("NYI: CUDA sspaddmm is not implemented");
}
} // namespace native
} // namespace at
