// Copyright (c) 2019 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// new file by hc 20220416

#include "lite/kernels/intel_fpga/conv_intelfpga.h"
#include "lite/backends/arm/math/conv_block_utils.h"
#include "lite/backends/arm/math/conv_impl.h"

namespace paddle {
namespace lite {
namespace kernels {
namespace intel_fpga {

template <>
IntelFpgaConv<PRECISION(kInt8), PRECISION(kInt8)>::~IntelFpgaConv() {}
template <>
void IntelFpgaConv<PRECISION(kInt8), PRECISION(kInt8)>::ReInitWhenNeeded() {
}

template <>
void IntelFpgaConv<PRECISION(kInt8), PRECISION(kInt8)>::PrepareForRun() {
 //PrepareForRun();
   auto& param = this->Param<param_t>();
}

template <>
void IntelFpgaConv<PRECISION(kInt8), PRECISION(kInt8)>::Run() {
}

}  // namespace intel_fpga
}  // namespace kernels
}  // namespace lite
}  // namespace paddle
