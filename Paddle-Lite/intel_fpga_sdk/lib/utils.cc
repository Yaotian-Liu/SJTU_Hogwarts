/* Copyright (c) 2020 AWCloud. All Rights Reserved.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compldoutnce with the License.
   You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#include "utils.h"
#include "intelfpga.h"
#include <iostream>
#include <fstream>
#include <sstream>
//---------------------------------------------------------------------------

int64_t intelfpga_gettime( void )
{
    struct timeval time;

    gettimeofday(&time, NULL);

    return 1000000LL * (int64_t)time.tv_sec + (int64_t)time.tv_usec;
}

float intelfpga_absmax(const float* din, int size)
{
    float max_value = 0.f;
    int cnt = size / 16;
    int remain = size & 15;
    float32x4_t vmax_val = vdupq_n_f32(0.f);
    const float* ptr_in = din;
    if (cnt > 0) {
        int loop_cnt = cnt;
        asm volatile(
                "vld1.32   {d0-d3}, [%[in]]!                @ load 8 float\n"
                "vld1.32   {d4-d7}, [%[in]]!                @ load 8 float\n"
                "0:                                         @ main loop\n"
                "vabs.f32 q4, q0                            @ abs \n"
                "vabs.f32 q5, q1                            @ abs \n"
                "vabs.f32 q6, q2                            @ abs \n"
                "vabs.f32 q7, q3                            @ abs \n"
                "vld1.32   {d0-d3}, [%[in]]!                @ load 8 float\n"
                "vmax.f32 q2, q4, q5                        @ max \n"
                "vmax.f32 q3, q6, q7                        @ max \n"
                "vmax.f32 q4, q2, q3                        @ max \n"
                "vld1.32   {d4-d7}, [%[in]]!                @ load 8 float\n"
                "vmax.f32 %q[max_val], q4, %q[max_val]      @ max \n"
                "subs %[cnt], #1                            @ loop count -1\n"
                "bne    0b                                  @ jump to main loop\n"
                : [in] "+r"(ptr_in), [cnt] "+r"(loop_cnt), [max_val] "+w"(vmax_val)
                :
                : "cc", "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7");
        float32x2_t vmax_p =
            vpmax_f32(vget_high_f32(vmax_val), vget_low_f32(vmax_val));
        float max0 = vget_lane_f32(vmax_p, 0);
        float max1 = vget_lane_f32(vmax_p, 1);
        float max2 = max0 > max1 ? max0 : max1;
        max_value = max_value > max2 ? max_value : max2;
    }
    ptr_in = din + 16 * cnt;
    for (int i = 0; i < remain; ++i) {
        float data = fabsf(*(ptr_in++));
        max_value = fmaxf(max_value, data);
    }
    return max_value;
}

void intelfpga_fp32_to_int8(const float* din, int8_t* dout, const float scale, int size)
{
    int cnt = size / 16;
    int remain = size & 15;
    float inv_scale = scale;
    float32x4_t vzero = vdupq_n_f32(0.f);
    float32x4_t vscale = vdupq_n_f32(inv_scale);
    float32x4_t vpoff = vdupq_n_f32(0.5f);
    float32x4_t vnoff = vdupq_n_f32(-0.5f);
    const float* din_c = din;
    signed char* dout_c = dout;
    if (cnt > 0) {
        int cnt_loop = cnt;
        const float* din_ptr = din_c;
        signed char* dout_ptr = dout_c;
        float vmax[4] = {-127.0, -127.0, -127.0, -127.0};
        asm volatile(
                "vld1.32    {d0-d3}, [%[din]]!          @ load in0~in7\n"
                "vld1.32    {d4-d7}, [%[din]]!          @ load in8~in16\n"
                "0:                                     @ main loop\n"
                "vand.i32   q4, %q[vpoff], %q[vpoff]    @ set offset, 0.5\n"
                "vand.i32   q5, q4, q4                  @ set offset, 0.5\n"
                "vand.i32   q6, q4, q4                  @ set offset, 0.5\n"
                "vand.i32   q7, q4, q4                  @ set offset, 0.5\n"
                "vcgt.f32   q8, q0, %q[vzero]           @ get mask > 0, in0\n"
                "vcgt.f32   q9, q1, %q[vzero]           @ get mask > 0, in1\n"
                "vcgt.f32   q10, q2, %q[vzero]          @ get mask > 0, in2\n"
                "vcgt.f32   q11, q3, %q[vzero]          @ get mask > 0, in3\n"
                "vbif.f32   q4, %q[vnoff], q8           @ get right offset\n"
                "vbif.f32   q5, %q[vnoff], q9           @ get right offset\n"
                "vbif.f32   q6, %q[vnoff], q10          @ get right offset\n"
                "vbif.f32   q7, %q[vnoff], q11          @ get right offset\n"
                "vmla.f32   q4, q0, %q[vscale]          @ mul scale\n"
                "vld1.32    {d0-d1}, [%[vmax]]          @ set q0 = -127 \n"
                "vmla.f32   q5, q1, %q[vscale]          @ mul scale\n"
                "vmla.f32   q6, q2, %q[vscale]          @ mul scale\n"
                "vmla.f32   q7, q3, %q[vscale]          @ mul scale\n"
                /* data >= -127 */
                "vcge.f32 q8, q4, q0                    @ q4 >= -127 \n"
                "vcge.f32 q9, q5, q0                    @ q4 >= -127 \n"
                "vcge.f32 q10, q6, q0                   @ q4 >= -127 \n"
                "vcge.f32 q11, q7, q0                   @ q4 >= -127 \n"
                /* choose data */
                "vbif q4, q0, q8                        @ choose \n"
                "vbif q5, q0, q9                        @ choose \n"
                "vbif q6, q0, q10                       @ choose \n"
                "vbif q7, q0, q11                       @ choose \n"
                /* fp32 - int32 */
                "vcvt.s32.f32  q0, q4                   @ cvt to int32\n"
                "vcvt.s32.f32  q1, q5                   @ cvt to int32\n"
                "vcvt.s32.f32  q2, q6                   @ cvt to int32\n"
                "vcvt.s32.f32  q3, q7                   @ cvt to int32\n"
                "vqmovn.s32 d8, q0                      @ cnt to int16\n"
                "vqmovn.s32 d9, q1                      @ cnt to int16\n"
                "vqmovn.s32 d10, q2                     @ cnt to int16\n"
                "vqmovn.s32 d11, q3                     @ cnt to int16\n"
                "vld1.32 {d0-d3}, [%[din]]!             @ load in0~in7\n"
                "vqmovn.s16 d12, q4                     @ cnt to int8\n"
                "vqmovn.s16 d13, q5                     @ cnt to int8\n"
                "vld1.32 {d4-d7}, [%[din]]!             @ load in8~in16\n"
                "vst1.32 {d12-d13},[%[dout]]!           @ write to output\n"
                "subs   %[cnt], #1                      @ loop count -1\n"
                "bne    0b                              @ to main loop\n"
                : [dout] "+r"(dout_ptr), [din] "+r"(din_ptr), [cnt] "+r"(cnt_loop)
                : [vscale] "w"(vscale),
                [vpoff] "w"(vpoff),
                [vnoff] "w"(vnoff),
                [vmax] "r"(vmax),
                [vzero] "w"(vzero)
                   : "cc",
                   "memory",
                   "q0",
                   "q1",
                   "q2",
                   "q3",
                   "q4",
                   "q5",
                   "q6",
                   "q7",
                   "q8",
                   "q9",
                   "q10",
                   "q11");
    }
    const float* din_r = din_c + 16 * cnt;
    signed char* dout_r = dout_c + 16 * cnt;
    for (int i = 0; i < remain; ++i) {
        dout_r[i] = saturate_cast<int8_t>(roundf(inv_scale * din_r[i]));
        dout_r[i] = dout_r[i] < -127 ? -127 : dout_r[i];
    }
}

void intelfpga_fp32_to_int32(const float* din, int32_t* dout, const float scale, int size)
{
    int i;
    float val;
    float inv_scale = scale;

    for (i=0; i<size; i++) {
        val = din[i] * inv_scale;
        if (val<0.0)
            val -= 0.5;
        else
            val += 0.5;

        dout[i] = (int32_t)val;
    }
}

void intelfpga_int32_to_fp32(const int* din, float* dout, float scale, int size)
{
    int cnt = size / 16;
    int remain = size & 15;
    float in_scale = scale;
    const int* din_c = din;
    float* dout_c = dout;
    float32x4_t vscale = vdupq_n_f32(in_scale);
    if (cnt > 0) {
        int loop = cnt;
        const int* din_ptr = din_c;
        float* dout_ptr = dout_c;
        asm volatile(
                "vld1.s32       {d0-d3}, [%[in]]!       \n"
                "vld1.s32       {d4-d7}, [%[in]]!       \n"
                "0:                                     \n"
                "vcvt.f32.s32   q4, q0                  \n"
                "vcvt.f32.s32   q5, q1                  \n"
                "vcvt.f32.s32   q6, q2                  \n"
                "vcvt.f32.s32   q7, q3                  \n"
                "vld1.s32       {d0-d3}, [%[in]]!       \n"
                "vmul.f32       q8, q4, %q[scale]       \n"
                "vmul.f32       q9, q5, %q[scale]       \n"
                "vmul.f32       q10, q6, %q[scale]      \n"
                "vmul.f32       q11, q7, %q[scale]      \n"
                "vld1.s32       {d4-d7}, [%[in]]!       \n"
                "subs           %[loop], #1             \n"
                "vst1.f32       {d16-d19}, [%[out]]!    \n"
                "vst1.f32       {d20-d23}, [%[out]]!    \n"
                "bne            0b                      \n"
                : [loop] "+r"(loop), [in] "+r"(din_ptr), [out] "+r"(dout_ptr)
                : [scale] "w"(vscale)
                : "cc",
                "memory",
                "q0",
                "q1",
                "q2",
                "q3",
                "q4",
                "q5",
                "q6",
                "q7",
                "q8",
                "q9",
                "q10",
                "q11");
    }
    const int* din_r = din_c + 16 * cnt;
    float* dout_r = dout_c + 16 * cnt;
    for (int i = 0; i < remain; ++i) {
        dout_r[i] = in_scale * din_r[i];
    }
}

#define FILL_BIAS                                            \
    "1:                               \n"                      \
    "vld1.32 {d6-d7}, [%[din_ptr]]!   @ vld1q_f32(din_ptr) \n" \
    "vld1.32 {d8-d9}, [%[din_ptr]]!   @ vld1q_f32(din_ptr) \n" \
    "vld1.32 {d10-d11}, [%[din_ptr]]! @ vld1q_f32(din_ptr) \n" \
    "vld1.32 {d12-d13}, [%[din_ptr]]! @ vld1q_f32(din_ptr) \n" \
    "vadd.f32 q3, q3, %q[vbdouts] @ add \n"                      \
    "vadd.f32 q4, q4, %q[vbdouts] @ add \n"                      \
    "vadd.f32 q5, q5, %q[vbdouts] @ add \n"                      \
    "vadd.f32 q6, q6, %q[vbdouts] @ add \n"
#define FILL_RELU                               \
    "vmax.f32 q3, q3, %q[vzero] @ vmaxq_f32() \n" \
    "vmax.f32 q4, q4, %q[vzero] @ vmaxq_f32() \n" \
    "vmax.f32 q5, q5, %q[vzero] @ vmaxq_f32() \n" \
    "vmax.f32 q6, q6, %q[vzero] @ vmaxq_f32() \n"
#define FILL_RELU6                             \
    "vmin.f32 q3, q3, %q[vsix] @ vminq_f32() \n" \
    "vmin.f32 q4, q4, %q[vsix] @ vmaxq_f32() \n" \
    "vmin.f32 q5, q5, %q[vsix] @ vmaxq_f32() \n" \
    "vmin.f32 q6, q6, %q[vsix] @ vmaxq_f32() \n"
#define FILL_LEAKY_RELU                          \
    "vcge.f32 q7, q3, %q[vzero]   @ vcgeq_u32 \n"  \
    "vmul.f32 q8, q3, %q[vscale]  @ vmulq_f32 \n"  \
    "vcge.f32 q9, q4, %q[vzero]   @ vcgeq_u32 \n"  \
    "vmul.f32 q10, q4, %q[vscale]  @ vmulq_f32 \n" \
    "vcge.f32 q11, q5, %q[vzero]   @ vcgeq_u32 \n" \
    "vmul.f32 q12, q5, %q[vscale]  @ vmulq_f32 \n" \
    "vcge.f32 q13, q6, %q[vzero]   @ vcgeq_u32 \n" \
    "vmul.f32 q14, q6, %q[vscale]  @ vmulq_f32 \n" \
    "vbif q3, q8, q7               @ choose \n"    \
    "vbif q4, q10, q9              @ choose \n"    \
    "vbif q5, q12, q11             @ choose \n"    \
    "vbif q6, q14, q13             @ choose \n"
#define FILL_STORE                                          \
    "subs %[cnt], #1                                \n"       \
    "vst1.32 {d6-d7}, [%[dout_ptr]]!       @ vst1q_f32()  \n" \
    "vst1.32 {d8-d9}, [%[dout_ptr]]!       @ vst1q_f32()  \n" \
    "vst1.32 {d10-d11}, [%[dout_ptr]]!     @ vst1q_f32()  \n" \
    "vst1.32 {d12-d13}, [%[dout_ptr]]!     @ vst1q_f32()  \n" \
    "bne  1b                                    \n"

void intelfpga_fill_bdouts_act(float* tensor, const float* bdouts, int channel, int channel_size, int relu, float alpha)
{
    float* data = tensor;
    int cnt_num = channel_size >> 4;
    int remain = channel_size % 16;
    float32x4_t vzero = vdupq_n_f32(0.f);

    if (relu) {
        float32x4_t vsix = vdupq_n_f32(6.f);
        float32x4_t vscale = vdupq_n_f32(alpha);
        switch (relu) {
            case 1:
                for (int j = 0; j < channel; j++) {
                    float bdouts_data = bdouts ? bdouts[j] : 0.f;
                    float* src = data + j * channel_size;
                    float* dst = data + j * channel_size;
                    float32x4_t vbdouts = vdupq_n_f32(bdouts_data);
                    int cnt = cnt_num;
                    if (cnt_num > 0) {
                        asm volatile(
                                FILL_BIAS FILL_RELU FILL_STORE
                                : [din_ptr] "+r"(src), [dout_ptr] "+r"(dst), [cnt] "+r"(cnt)
                                : [vzero] "w"(vzero), [vbdouts] "w"(vbdouts)
                                : "memory", "cc", "q3", "q4", "q5", "q6");
                    }
                    for (int i = 0; i < remain; i++) {
                        float tmp = (*src + bdouts_data);
                        *dst = tmp >= 0.f ? tmp : 0.f;
                        src++;
                        dst++;
                    }
                }
                break;
            case 2:
                for (int j = 0; j < channel; j++) {
                    float bdouts_data = bdouts ? bdouts[j] : 0.f;
                    float* src = data + j * channel_size;
                    float* dst = data + j * channel_size;
                    float32x4_t vbdouts = vdupq_n_f32(bdouts_data);
                    int cnt = cnt_num;
                    if (cnt_num > 0) {
                        asm volatile(
                                FILL_BIAS FILL_RELU FILL_RELU6 FILL_STORE
                                : [din_ptr] "+r"(src), [dout_ptr] "+r"(dst), [cnt] "+r"(cnt)
                                : [vzero] "w"(vzero), [vsix] "w"(vsix), [vbdouts] "w"(vbdouts)
                                : "memory", "cc", "q3", "q4", "q5", "q6");
                    }
                    for (int i = 0; i < remain; i++) {
                        float tmp = (*src + bdouts_data);
                        tmp  = tmp >= 0.f ? tmp : 0.f;
                        *dst = tmp <= 6.f ? tmp : 6.f;
                        src++;
                        dst++;
                    }
                }
                break;
            case 3:
                for (int j = 0; j < channel; j++) {
                    float bdouts_data = bdouts ? bdouts[j] : 0.f;
                    float* src = data + j * channel_size;
                    float* dst = data + j * channel_size;
                    float32x4_t vbdouts = vdupq_n_f32(bdouts_data);
                    int cnt = cnt_num;
                    if (cnt_num > 0) {
                        asm volatile(
                                FILL_BIAS FILL_LEAKY_RELU FILL_STORE
                                : [din_ptr] "+r"(src), [dout_ptr] "+r"(dst), [cnt] "+r"(cnt)
                                : [vzero] "w"(vzero), [vscale] "w"(vscale), [vbdouts] "w"(vbdouts)
                                : "memory",
                                "cc",
                                "q3",
                                "q4",
                                "q5",
                                "q6",
                                "q7",
                                "q8",
                                "q9",
                                "q10",
                                "q11",
                                "q12",
                                "q13",
                                "q14");
                    }
                    for (int i = 0; i < remain; i++) {
                        float tmp = (*src + bdouts_data);
                        if (tmp >= 0.f) {
                            *dst = tmp;
                        } else {
                            *dst = tmp * alpha;
                        }
                        src++;
                        dst++;
                    }
                }
                break;
        }
    } else {
        for (int j = 0; j < channel; ++j) {
            float bdouts_data = bdouts ? bdouts[j] : 0.f;
            float32x4_t vbdouts = vdupq_n_f32(bdouts_data);
            float* src = data + j * channel_size;
            float* dst = data + j * channel_size;
            int cnt = cnt_num;
            if (cnt > 0) {
                asm volatile(FILL_BIAS FILL_STORE
                        : [din_ptr] "+r"(src), [dout_ptr] "+r"(dst), [cnt] "+r"(cnt)
                        : [vbdouts] "w"(vbdouts)
                        : "memory", "cc", "q3", "q4", "q5", "q6");
            }
            for (int i = 0; i < remain; i++) {
                *dst = *src + bdouts_data;
            }
        }
    }
}

//---------------------------------------------------------------------------

void intelfpga_int8_to_fp32(const int8_t* in,
        float* out,
        const float* scale,
        int axis_size,
        int64_t outer_size,
        int64_t inner_size) {
    int cnt = inner_size / 16;
    int remain = inner_size & 15;
    int64_t loop_size = axis_size * outer_size;
    for (int64_t n = 0; n < loop_size; ++n) {
        float in_scale = scale[n % axis_size];
        const signed char* din_c = in + n * inner_size;
        float* dout_c = out + n * inner_size;
        float32x4_t vscale = vdupq_n_f32(in_scale);
        if (cnt > 0) {
            int loop = cnt;
            const signed char* din_ptr = din_c;
            float* dout_ptr = dout_c;
            asm volatile(
                    "vld1.32    {d0-d1},    [%[in]]!            @ load 16 int8\n"
                    "0:                                 @ main loop\n"
                    "vmovl.s8      q2, d0               @ trans to int16\n"
                    "vmovl.s8      q3, d1               @ trans to int16\n"
                    "vmovl.s16     q4, d4               @ trans to int32\n"
                    "vmovl.s16     q5, d5               @ trans to int32\n"
                    "vmovl.s16     q6, d6               @ trans to int32\n"
                    "vmovl.s16     q7, d7               @ trans to int32\n"
                    "vcvt.f32.s32  q0, q4               @ trans to fp32\n"
                    "vcvt.f32.s32  q1, q5               @ trans to fp32\n"
                    "vcvt.f32.s32  q2, q6               @ trans to fp32\n"
                    "vcvt.f32.s32  q3, q7               @ trans to fp32\n"
                    "vmul.f32      q4, q0, %q[scale]    @ mul with scale\n"
                    "vmul.f32      q5, q1, %q[scale]    @ mul with scale\n"
                    "vmul.f32      q6, q2, %q[scale]    @ mul with scale\n"
                    "vmul.f32      q7, q3, %q[scale]    @ mul with scale\n"

                    "vld1.32    {d0-d1},    [%[in]]!    @ load 16 int8\n"

                    "subs          %[loop], #1            \n"

                    "vst1.f32      {d8-d11}, [%[out]]!  @ write to memory\n"
                    "vst1.f32      {d12-d15}, [%[out]]! @ write to memory\n"

                    "bne           0b                     \n"
                    : [loop] "+r"(loop), [in] "+r"(din_ptr), [out] "+r"(dout_ptr)
                    : [scale] "w"(vscale)
                       : "cc", "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7");
        }
        const signed char* din_r = din_c + 16 * cnt;
        float* dout_r = dout_c + 16 * cnt;
        for (int i = 0; i < remain; ++i) {
            dout_r[i] = in_scale * din_r[i];
        }
    }
}
int8_t Quantize(float x) {
    int sign, exp, mag;
    int y = 0;
    unsigned int x_int_expr =*(int*)&x;
    sign = x_int_expr >> 31;
    exp = ((x_int_expr & 0x7f800000) >> 23);
    mag = (x_int_expr & 0x7fffff);
    // NON number.
    if(exp == 126) {
        // INF or NaN.
        y = 1;
    } else if(exp < 126) {
        // Subnormal number.
        y = 0;
    } else {
        exp = exp - 127;
        mag = mag >> (23 - exp - 1);
        if((mag & 0x1) == 1) {
            y = y + 1;
        }
        y += (1 << exp) | (mag >> 1);

    }

    if(sign == 1) {
        y = -y;
    }
    if(y > Q_MAX) {
        return Q_MAX;
    }
    if(y < Q_MIN) {
        return Q_MIN;
    }
    return y;
}

// Input:
void conv_chw_pad(int8_t* din, int8_t* dout, int ch, int h,int w, int pad) {
    int w_2pad = w + 2 * pad;
    int w_stride = w_2pad * (h + 2 * pad);
    int8_t* din_int8 = (int8_t*)din;
    int8_t* dout_int8;
    for(int c = 0; c < ch; c++) {
        dout_int8 = dout + w_2pad + c * w_stride;
        for(int r = 0; r < h; r++) {
            for(int k = 0; k < w; k++) {
                *(dout_int8 + pad) = *(din_int8);
                dout_int8++;
                din_int8++;
            }
            dout_int8 += 2 * pad;
        }
    }
}

/*preprocessing weights
 * input weights: [chout, chin/ group, kh, kw] --> outputs weights: [chout / n,
 * chin/ group, kh, kw, n]
 */
bool conv_trans_weights_numc(const int8_t* din,
        int8_t* dout,
        int chout,
        int chin,
        int n,
        int kernel_size) {
    std::cout << "Entering conv_trans_weights_numc" << std::endl;

    if (n <= 0) {
        std::cout << "ch_n and hei_n are more than zero";
        return false;
    }
    int c_loop = chout / n;
    int chout_round = (chout + n - 1) / n;
    int win_stride = chin * kernel_size;
    int wout_stride = n * win_stride;
    int co = 0;
    for (; co < c_loop; ++co) {
        int8_t* dout_c = dout + co * wout_stride;
        const int8_t* dout_array[n];
        dout_array[0] = din + co * wout_stride;
        for (int i = 1; i < n; i++) {
            dout_array[i] = dout_array[i - 1] + win_stride;
        }
        for (int ci = 0; ci < chin; ++ci) {
            for (int k = 0; k < kernel_size; ++k) {
                for (int i = 0; i < n; i++) {
                    *(dout_c++) = *(dout_array[i]++);
                }
            }
        }
    }
    // pad final chout
    if (chout_round > c_loop) {
        int8_t* dout_c = dout + c_loop * wout_stride;
        const int8_t* dout_array[n];
        dout_array[0] = din + c_loop * wout_stride;
        for (int i = 1; i < n; i++) {
            dout_array[i] = dout_array[i - 1] + win_stride;
        }
        // deal remain
        int cremain = chout_round * n - chout;
        for (int i = 1; i <= cremain; i++) {
            dout_array[n - i] = dout_array[0];
        }
        for (int ci = 0; ci < chin; ++ci) {
            for (int k = 0; k < kernel_size; ++k) {
                for (int i = 0; i < n; i++) {
                    *(dout_c++) = *(dout_array[i]++);
                }
            }
        }
    }
    return true;
}
void PrintTensor(std::string filename, void* din, int size) {
    std::ofstream outfile(filename.c_str(), std::ios::trunc);
    if(!outfile.is_open()) {
        std::cout << "Open file: " << filename <<" failed.\n";
        return;
    }
    outfile << "fpga result\n";
    std::stringstream ss;
    int cnt = 1;
    int8_t* din_int8 = (int8_t*)din;
    for(int i = 0; i < size; i++) {
        ss << (int)din_int8[i] << " ";
        if(cnt++ % 16 ==0) {
            ss << "\n";
        }
    }
    outfile << ss.str();
    outfile.close();
} // End

/*void InputRearrange(int8_t* din, int8_t* dout, const int c, const int h,
        const int w, const int pad){
    int8_t* dout_array[INPUT_EXTEND_SCALE];
    int idx_fpga_idata = 0;
    for(int i = 0; i < UpRound(c, INPUT_EXTEND_SCALE); i++) {
        dout_array[0] = din + i * ((h + 2 * pad) * (w + 2 * pad) * INPUT_EXTEND_SCALE);
        for(int n = 1; n < INPUT_EXTEND_SCALE; n++) {
            dout_array[n] = dout_array[n - 1] + (h + 2 * pad) * (w + 2 * pad);
        }
        for(int r = 0; r < (h + 2 * pad); r++) {
            for(int c = 0; c < (w + 2 * pad); c++) {
                for(int k = 0; k < 16; k++) {
                    dout[idx_fpga_idata++] = *(dout_array[k]++);
                }
            }
        }
    }

}*/

void asm_rearrange_16_layers(int8_t* l_0, int8_t* l_1, int8_t* l_2, int8_t* l_3,
    int8_t* l_4, int8_t* l_5, int8_t* l_6, int8_t* l_7, int8_t* out, int iter, int l_len) {

    // l_len is the length of the dout_array[]
    // len means jump 8 dout_array[]
    int len = l_len * 8;

    asm volatile (
        // r2 -> jump back length
        "sub      r2, %[len], #8   \n"

        // main loop
        "loop:                     \n"

        // load l_0 - l_7
        "vld1.8   d0, [%[l_0]]     \n"
        "vld1.8   d1, [%[l_1]]     \n"
        "vld1.8   d2, [%[l_2]]     \n"
        "vld1.8   d3, [%[l_3]]     \n"
        "vld1.8   d4, [%[l_4]]     \n"
        "vld1.8   d5, [%[l_5]]     \n"
        "vld1.8   d6, [%[l_6]]     \n"
        "vld1.8   d7, [%[l_7]]     \n"

        // calculate l_8 - l_15
        // l_{x} -> l_{x+8}
        "add   %[l_0], %[l_0], %[len]   \n"
        "add   %[l_1], %[l_1], %[len]   \n"
        "add   %[l_2], %[l_2], %[len]   \n"
        "add   %[l_3], %[l_3], %[len]   \n"
        "add   %[l_4], %[l_4], %[len]   \n"
        "add   %[l_5], %[l_5], %[len]   \n"
        "add   %[l_6], %[l_6], %[len]   \n"
        "add   %[l_7], %[l_7], %[len]   \n"

        // load l_8 - l_15
        "vld1.8   d20, [%[l_0]]     \n"
        "vld1.8   d21, [%[l_1]]     \n"
        "vld1.8   d22, [%[l_2]]     \n"
        "vld1.8   d23, [%[l_3]]     \n"
        "vld1.8   d24, [%[l_4]]     \n"
        "vld1.8   d25, [%[l_5]]     \n"
        "vld1.8   d26, [%[l_6]]     \n"
        "vld1.8   d27, [%[l_7]]     \n"

        // calculate next l_0 - l_7
        // use r2 to jump back
        "sub   %[l_0], %[l_0], r2   \n"
        "sub   %[l_1], %[l_1], r2   \n"
        "sub   %[l_2], %[l_2], r2   \n"
        "sub   %[l_3], %[l_3], r2   \n"
        "sub   %[l_4], %[l_4], r2   \n"
        "sub   %[l_5], %[l_5], r2   \n"
        "sub   %[l_6], %[l_6], r2   \n"
        "sub   %[l_7], %[l_7], r2   \n"

        // calculate store base addr
        // "add r0, %[out], %[offset] \n"

        // zip single pixel
        "vzip.8   d0, d1           \n"
        "vzip.8   d2, d3           \n"
        "vzip.8   d4, d5           \n"
        "vzip.8   d6, d7           \n"

        "vzip.8   d20, d21           \n"
        "vzip.8   d22, d23           \n"
        "vzip.8   d24, d25           \n"
        "vzip.8   d26, d27           \n"

        // zip two pixels
        "vzip.16  d0, d2           \n"
        "vzip.16  d1, d3           \n"
        "vzip.16  d4, d6           \n"
        "vzip.16  d5, d7           \n"

        "vzip.16  d20, d22           \n"
        "vzip.16  d21, d23           \n"
        "vzip.16  d24, d26           \n"
        "vzip.16  d25, d27           \n"

        // zip four pixels
        "vzip.32  d0, d4           \n"
        "vzip.32  d2, d6           \n"
        "vzip.32  d1, d5           \n"
        "vzip.32  d3, d7           \n"

        "vzip.32  d20, d24           \n"
        "vzip.32  d22, d26           \n"
        "vzip.32  d21, d25           \n"
        "vzip.32  d23, d27           \n"

        // store to memory
        "vst1.8   d0 , [%[out]]!    \n"
        "vst1.8   d20, [%[out]]!    \n"
        "vst1.8   d4 , [%[out]]!    \n"
        "vst1.8   d24, [%[out]]!    \n"
        "vst1.8   d2 , [%[out]]!    \n"
        "vst1.8   d22, [%[out]]!    \n"
        "vst1.8   d6 , [%[out]]!    \n"
        "vst1.8   d26, [%[out]]!    \n"
        "vst1.8   d1 , [%[out]]!    \n"
        "vst1.8   d21, [%[out]]!    \n"
        "vst1.8   d5 , [%[out]]!    \n"
        "vst1.8   d25, [%[out]]!    \n"
        "vst1.8   d3 , [%[out]]!    \n"
        "vst1.8   d23, [%[out]]!    \n"
        "vst1.8   d7 , [%[out]]!    \n"
        "vst1.8   d27, [%[out]]!    \n"

        "subs     %[iter], %[iter], #1  \n"
        "bne      loop                  \n"

        // output
        : [out] "+r"(out),
        [l_0] "+r"(l_0),
        [l_1] "+r"(l_1),
        [l_2] "+r"(l_2),
        [l_3] "+r"(l_3),
        [l_4] "+r"(l_4),
        [l_5] "+r"(l_5),
        [l_6] "+r"(l_6),
        [l_7] "+r"(l_7)
        // input
        : [len] "r"(len),
        [iter] "r"(iter)

        : "cc", "memory", "r2",
        "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7","d20", "d21", "d22", "d23", "d24", "d25", "d26", "d27"
        );
}

void InputRearrange(int8_t* din, int8_t* dout, const int c, const int h,
                    const int w, const int pad) {

    int8_t* dout_array[INPUT_EXTEND_SCALE];
    int dout_offset = 0;
    int dout_array_length = (h + 2 * pad) * (w + 2 * pad);
    for (int i = 0; i < UpRound(c, INPUT_EXTEND_SCALE); i++) {
        // dout_array: (1, h_pad_ w_pad)
        dout_array[0] = din + i * (dout_array_length * INPUT_EXTEND_SCALE);

        for (int n = 1; n < INPUT_EXTEND_SCALE; n++) {
            dout_array[n] = dout_array[n - 1] + dout_array_length;
        }

        int num_iter = dout_array_length / 8;
        int leftover = dout_array_length % 8;

        if (num_iter > 0) {
            asm_rearrange_16_layers(dout_array[0], dout_array[1], dout_array[2], dout_array[3],
                dout_array[4], dout_array[5], dout_array[6], dout_array[7],
                dout, num_iter, dout_array_length);

            dout += num_iter * 8 * 16;

            for (int k = 0; k < INPUT_EXTEND_SCALE; k++)
                dout_array[k] += 8 * num_iter;
        }

        if (leftover > 0)
            for (int i = 0; i < leftover; i++)
                for (int j = 0; j < INPUT_EXTEND_SCALE; j++)
                    *(dout++) = *(dout_array[j]++);
    }
}

void asm_output_rearrange_16_layers(int8_t* din, int8_t* l_0, int8_t* l_1, int8_t* l_2, int8_t* l_3,
    int8_t* l_4, int8_t* l_5, int8_t* l_6, int8_t* l_7, int iter, int l_len) {

    int len = l_len * 8;

    asm volatile (
        // r2 -> jump back length
        "sub    r2, %[len], #8       \n"

        // main loop
        "outloop:                    \n"

        // load din
        "vld1.8   d0 , [%[din]]!     \n"
        "vld1.8   d20, [%[din]]!     \n"
        "vld1.8   d1 , [%[din]]!     \n"
        "vld1.8   d21, [%[din]]!     \n"
        "vld1.8   d2 , [%[din]]!     \n"
        "vld1.8   d22, [%[din]]!     \n"
        "vld1.8   d3 , [%[din]]!     \n"
        "vld1.8   d23, [%[din]]!     \n"
        "vld1.8   d4 , [%[din]]!     \n"
        "vld1.8   d24, [%[din]]!     \n"
        "vld1.8   d5 , [%[din]]!     \n"
        "vld1.8   d25, [%[din]]!     \n"
        "vld1.8   d6 , [%[din]]!     \n"
        "vld1.8   d26, [%[din]]!     \n"
        "vld1.8   d7 , [%[din]]!     \n"
        "vld1.8   d27, [%[din]]!     \n"

        // unzip 4 pixels
        "vuzp.32  d0, d1  \n"
        "vuzp.32  d2, d3  \n"
        "vuzp.32  d4, d5  \n"
        "vuzp.32  d6, d7  \n"

        "vuzp.32  d20, d21  \n"
        "vuzp.32  d22, d23  \n"
        "vuzp.32  d24, d25  \n"
        "vuzp.32  d26, d27  \n"

        // unzip 2 pixels
        "vuzp.16  d0, d2  \n"
        "vuzp.16  d1, d3  \n"
        "vuzp.16  d4, d6  \n"
        "vuzp.16  d5, d7  \n"

        "vuzp.16  d20, d22  \n"
        "vuzp.16  d21, d23  \n"
        "vuzp.16  d24, d26  \n"
        "vuzp.16  d25, d27  \n"

        // unzip single pixel
        "vuzp.8   d0, d4  \n"
        "vuzp.8   d2, d6  \n"
        "vuzp.8   d1, d5  \n"
        "vuzp.8   d3, d7  \n"

        "vuzp.8   d20, d24  \n"
        "vuzp.8   d22, d26  \n"
        "vuzp.8   d21, d25  \n"
        "vuzp.8   d23, d27  \n"

        // store to dout_array[0] - dout_array[7]
        "vst1.8   d0, [%[l_0]]  \n"
        "vst1.8   d4, [%[l_1]]  \n"
        "vst1.8   d2, [%[l_2]]  \n"
        "vst1.8   d6, [%[l_3]]  \n"
        "vst1.8   d1, [%[l_4]]  \n"
        "vst1.8   d5, [%[l_5]]  \n"
        "vst1.8   d3, [%[l_6]]  \n"
        "vst1.8   d7, [%[l_7]]  \n"

        // calculate dout_array[8] - dout_array[15]
        "add   %[l_0], %[l_0], %[len]  \n"
        "add   %[l_1], %[l_1], %[len]  \n"
        "add   %[l_2], %[l_2], %[len]  \n"
        "add   %[l_3], %[l_3], %[len]  \n"
        "add   %[l_4], %[l_4], %[len]  \n"
        "add   %[l_5], %[l_5], %[len]  \n"
        "add   %[l_6], %[l_6], %[len]  \n"
        "add   %[l_7], %[l_7], %[len]  \n"

        // store to dout_array[8] - dout_array[15]
        "vst1.8   d20, [%[l_0]]  \n"
        "vst1.8   d24, [%[l_1]]  \n"
        "vst1.8   d22, [%[l_2]]  \n"
        "vst1.8   d26, [%[l_3]]  \n"
        "vst1.8   d21, [%[l_4]]  \n"
        "vst1.8   d25, [%[l_5]]  \n"
        "vst1.8   d23, [%[l_6]]  \n"
        "vst1.8   d27, [%[l_7]]  \n"

        // calculate next dout_array[0] - dout_array[7]
        "sub   %[l_0], %[l_0], r2   \n"
        "sub   %[l_1], %[l_1], r2   \n"
        "sub   %[l_2], %[l_2], r2   \n"
        "sub   %[l_3], %[l_3], r2   \n"
        "sub   %[l_4], %[l_4], r2   \n"
        "sub   %[l_5], %[l_5], r2   \n"
        "sub   %[l_6], %[l_6], r2   \n"
        "sub   %[l_7], %[l_7], r2   \n"

        "subs   %[iter], %[iter], #1  \n"
        "bne    outloop               \n"


        : [din] "+r"(din),
        [l_0] "+r"(l_0),
        [l_1] "+r"(l_1),
        [l_2] "+r"(l_2),
        [l_3] "+r"(l_3),
        [l_4] "+r"(l_4),
        [l_5] "+r"(l_5),
        [l_6] "+r"(l_6),
        [l_7] "+r"(l_7)

        : [len] "r"(len),
        [iter] "r"(iter)

        : "cc", "memory", "r2",
        "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
        "d20", "d21", "d22", "d23", "d24", "d25", "d26", "d27"
    );
}

void OutputRearrange(int8_t* din, int8_t* dout, const int c, const int h,
        const int w){
    int8_t* dout_array[INPUT_EXTEND_SCALE];
        int idx_fpga_idata = 0;
        int array_length = h * w;
        for (int i = 0; i < UpRound(c, INPUT_EXTEND_SCALE); i++) {
            dout_array[0] = dout + i * array_length * INPUT_EXTEND_SCALE;
            for (int n = 1; n < INPUT_EXTEND_SCALE; n++) {
                dout_array[n] = dout_array[n - 1] + array_length;
            }

            int num_iter = array_length / 8;
            int leftover = array_length % 8;

            if (num_iter > 0) {
                asm_output_rearrange_16_layers(din, dout_array[0], dout_array[1], dout_array[2], dout_array[3],
                    dout_array[4], dout_array[5], dout_array[6], dout_array[7], num_iter, array_length);

                din += num_iter * 8 * 16;

                for (int k = 0; k < INPUT_EXTEND_SCALE; k++)
                    dout_array[k] += 8 * num_iter;
            }
            if (leftover > 0) {
                for (int i = 0; i < leftover; i ++)
                    for (int j = 0; j < INPUT_EXTEND_SCALE; j++)
                        *(dout_array[j]++) = *(din++);
            }
        }
}
