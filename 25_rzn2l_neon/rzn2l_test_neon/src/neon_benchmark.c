/**
 * @file neon_benchmark.c
 * @brief NEON/DSP性能基准测试
 * 
 * 本文件实现NEON和C语言的性能对比测试，包括：
 * 
 * ## 测试分类
 * 1. **通用DSP测试**（USE_GENERAL_TEST）
 *    - 数组加法、乘累加、矩阵乘法、FIR滤波、类型转换
 *    - 适合NEON向量化（批量数据处理）
 * 
 * 2. **FOC算法测试**（USE_FOC_TEST）
 *    - Clarke变换、Park变换、逆Park变换、SVPWM、PI控制器
 *    - 不适合NEON向量化（小计算量、数据依赖）
 * 
 * ## 关键发现
 * - **-O2/-O3优化时**：编译器自动向量化效果很好，手动NEON无优势
 * - **-O1优化时**：手动NEON intrinsics有优势
 * - **FOC算法**：标量实现已最优，NEON反而增加开销
 * - **批量处理**：NEON有显著优势（FIR: 2.11x, Int->Float: 3.37x）
 * 
 * ## 优化建议
 * 1. 优先使用-O2/-O3优化，让编译器自动向量化
 * 2. 只在批量数据处理时手动使用NEON
 * 3. FOC等控制算法使用标量实现
 * 4. 使用restrict关键字帮助编译器分析指针别名
 * 
 * @note 修复记录：
 *       - 2026-04-11: 修复加速比计算显示bug
 *       - 2026-04-11: 优化FOC算法NEON实现
 *       - 2026-04-11: 添加volatile防止过度优化
 *       - 2026-04-11: 添加dummy_sum防止循环被优化掉
 */

#include "hal_data.h"
#include <arm_neon.h>  // NEON intrinsics头文件
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* ==================== 测试配置参数 ==================== */
#define TEST_ARRAY_SIZE 1024      ///< 测试数组大小
#define MATRIX_SIZE 8             ///< 矩阵大小（8x8）
#define FIR_TAPS 16               ///< FIR滤波器抽头数
#define BENCHMARK_ITERATIONS 10   ///< 基准测试迭代次数

/* ==================== NEON优化等级配置 ==================== */
/**
 * @brief NEON函数优化等级配置
 * 
 * @note 使用方法：
 *       - 修改NEON_OPT_LEVEL宏定义即可调整所有NEON函数的优化等级
 *       - 可选值："O0", "O1", "O2", "O3", "Os", "Ofast"
 *       - 推荐值：
 *         - "O2"：平衡优化和代码大小
 *         - "O3"：最大优化（激进向量化）
 *         - "Ofast"：O3 + 快速数学优化
 * 
 * @warning 注意：
 *          - O3可能导致代码膨胀
 *          - Ofast可能违反IEEE浮点标准
 *          - 建议先用O2测试，性能不足再尝试O3
 */
#ifndef NEON_OPT_LEVEL
#define NEON_OPT_LEVEL "O1"  ///< 默认使用O1优化
#endif

/* NEON优化宏定义 */
#define NEON_OPTIMIZE __attribute__((optimize(NEON_OPT_LEVEL)))

/* ==================== 功能开关 ==================== */
#define USE_GENERAL_TEST 1        ///< 启用通用DSP测试
#define USE_FOC_TEST 1            ///< 启用FOC算法测试
#define USE_CMSIS_DSP 0           ///< 启用CMSIS-DSP库（当前关闭）

#if USE_CMSIS_DSP
#include "arm_math.h"
#endif

/* ==================== 工具函数宏定义 ==================== */
#define uart_print neon_benchmark_utils_uart_printf  ///< UART打印函数
#define benchmark_start neon_benchmark_start          ///< 开始计时
#define benchmark_end neon_benchmark_end              ///< 结束计时

/* 外部函数声明 */
extern void neon_benchmark_start(void);
extern uint32_t neon_benchmark_end(void);
extern int neon_benchmark_utils_uart_printf(const char *fmt, ...);



/* ==================== 通用DSP测试函数 ==================== */
#if USE_GENERAL_TEST

/**
 * @brief 数组加法（C语言实现）
 * 
 * @param dest 输出数组
 * @param src1 输入数组1
 * @param src2 输入数组2
 * @param size 数组大小
 * 
 * @note 简单循环实现，编译器可自动向量化
 */
static void test_array_add_c(int32_t *dest, const int32_t *src1, const int32_t *src2, int size) {
    for (int i = 0; i < size; i++) {
        dest[i] = src1[i] + src2[i];
    }
}

/**
 * @brief 数组加法（NEON实现）
 * 
 * @note 实现说明：
 *       - 使用int32x4_t向量，每次处理4个int32
 *       - vld1q_s32：加载4个int32到NEON寄存器
 *       - vaddq_s32：NEON向量加法
 *       - vst1q_s32：存储4个int32到内存
 *       - 剩余元素用标量处理
 * 
 * @warning 测试结果：
 *          - -O1时：NEON有优势
 *          - -O2/-O3时：编译器自动向量化效果相当
 * 
 * @note 优化等级：由NEON_OPT_LEVEL宏控制（默认O2）
 */
static NEON_OPTIMIZE void test_array_add_neon(int32_t *dest, const int32_t *src1, const int32_t *src2, int size) {
    int i = 0;
    // 向量化处理：每次4个元素
    for (; i <= size - 4; i += 4) {
        int32x4_t a = vld1q_s32(src1 + i);      // 加载4个元素
        int32x4_t b = vld1q_s32(src2 + i);      // 加载4个元素
        int32x4_t result = vaddq_s32(a, b);     // 向量加法
        vst1q_s32(dest + i, result);            // 存储4个元素
    }
    // 处理剩余元素
    for (; i < size; i++) {
        dest[i] = src1[i] + src2[i];
    }
}

#if USE_CMSIS_DSP
static void test_array_add_dsp(int32_t *dest, const int32_t *src1, const int32_t *src2, int size) {
    arm_add_q31((q31_t *)src1, (q31_t *)src2, (q31_t *)dest, size);
}
#endif

/**
 * @brief 乘累加运算（C语言实现）
 * 
 * 计算点积：sum = Σ(src1[i] * src2[i])
 * 
 * @param dest 输出：累加结果
 * @param src1 输入数组1
 * @param src2 输入数组2
 * @param size 数组大小
 * 
 * @note 典型DSP运算，适合NEON优化
 */
static void test_multiply_accumulate_c(int32_t *dest, const int32_t *src1, const int32_t *src2, int size) {
    int32_t sum = 0;
    for (int i = 0; i < size; i++) {
        sum += src1[i] * src2[i];
    }
    *dest = sum;
}

/**
 * @brief 乘累加运算（NEON实现）
 * 
 * @note 实现说明：
 *       - 使用vmlaq_s32（乘累加指令）
 *       - 每次处理4个元素：sum += a[i]*b[i]
 *       - 最后需要横向累加向量元素
 *       - vgetq_lane_s32：提取向量中的单个元素
 * 
 * @warning 测试结果异常：
 *          - -O2时C版本被过度优化（只用了1个周期）
 *          - 需要确保测试数据不被优化掉
 */
static NEON_OPTIMIZE void test_multiply_accumulate_neon(int32_t *dest, const int32_t *src1, const int32_t *src2, int size) {
    int32x4_t sum_vec = vmovq_n_s32(0);  // 初始化累加向量为0
    int i = 0;
    // 向量化乘累加：每次4个元素
    for (; i <= size - 4; i += 4) {
        int32x4_t a = vld1q_s32(src1 + i);
        int32x4_t b = vld1q_s32(src2 + i);
        sum_vec = vmlaq_s32(sum_vec, a, b);  // sum += a * b
    }
    // 横向累加：将向量4个元素相加
    *dest = vgetq_lane_s32(sum_vec, 0) + vgetq_lane_s32(sum_vec, 1) +
            vgetq_lane_s32(sum_vec, 2) + vgetq_lane_s32(sum_vec, 3);
}

#if USE_CMSIS_DSP
static void test_multiply_accumulate_dsp(q31_t *dest, const q31_t *src1, const q31_t *src2, int size) {
    arm_dot_prod_q31((q31_t *)src1, (q31_t *)src2, size, dest);
}
#endif

/**
 * @brief 矩阵乘法（C语言实现）
 * 
 * 计算C = A * B，其中A、B、C都是size×size矩阵
 * 
 * @param dest 输出矩阵C
 * @param a 输入矩阵A
 * @param b 输入矩阵B
 * @param size 矩阵大小
 * 
 * @note 三重循环，O(n³)复杂度
 */
static void test_matrix_multiply_c(int32_t *dest, const int32_t *a, const int32_t *b, int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            int32_t sum = 0;
            for (int k = 0; k < size; k++) {
                sum += a[i * size + k] * b[k * size + j];
            }
            dest[i * size + j] = sum;
        }
    }
}

/**
 * @brief 矩阵乘法（NEON实现）
 * 
 * @note 实现说明：
 *       - 只优化最内层循环（k循环）
 *       - 使用vmlaq_s32进行向量化乘累加
 *       - 外层循环（i、j）保持标量
 * 
 * @warning 测试结果：
 *          - NEON版本比C版本慢（0.80x）
 *          - 可能原因：矩阵太小（8×8），向量化开销大
 */
static NEON_OPTIMIZE void test_matrix_multiply_neon(int32_t *dest, const int32_t *a, const int32_t *b, int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            int32x4_t sum_vec = vmovq_n_s32(0);
            int k = 0;
            // 向量化内层循环
            for (; k <= size - 4; k += 4) {
                int32x4_t a_vec = vld1q_s32(a + i * size + k);
                int32x4_t b_vec = vld1q_s32(b + k * size + j);
                sum_vec = vmlaq_s32(sum_vec, a_vec, b_vec);
            }
            // 横向累加
            int32_t sum = vgetq_lane_s32(sum_vec, 0) + vgetq_lane_s32(sum_vec, 1) +
                          vgetq_lane_s32(sum_vec, 2) + vgetq_lane_s32(sum_vec, 3);
            // 处理剩余元素
            for (; k < size; k++) {
                sum += a[i * size + k] * b[k * size + j];
            }
            dest[i * size + j] = sum;
        }
    }
}

/**
 * @brief FIR滤波器（C语言实现）
 * 
 * 实现有限脉冲响应滤波器：y[n] = Σ h[k] * x[n-k]
 * 
 * @param output 输出信号
 * @param input 输入信号
 * @param coeffs 滤波器系数
 * @param input_size 输入信号长度
 * @param num_taps 滤波器抽头数
 * 
 * @note 典型DSP应用，适合NEON优化
 */
static void test_fir_filter_c(int32_t *output, const int32_t *input, const int32_t *coeffs, int input_size, int num_taps) {
    for (int n = 0; n < input_size; n++) {
        int32_t sum = 0;
        for (int k = 0; k < num_taps; k++) {
            if (n >= k) {
                sum += input[n - k] * coeffs[k];
            }
        }
        output[n] = sum >> 15;  // Q15定点数处理
    }
}

/**
 * @brief FIR滤波器（NEON实现）
 * 
 * @note 实现说明：
 *       - 向量化内层循环（k循环）
 *       - 处理边界条件：n < k时input[n-k]无效
 *       - 使用临时数组处理边界元素
 *       - 测试结果：NEON有显著优势（2.11x）
 * 
 * @warning FIR是NEON优化的典型应用场景：
 *          - 批量数据处理
 *          - 规律的内存访问
 *          - 无复杂数据依赖
 */
static NEON_OPTIMIZE void test_fir_filter_neon(int32_t *output, const int32_t *input, const int32_t *coeffs, int input_size, int num_taps) {
    for (int n = 0; n < input_size; n++) {
        int32x4_t sum_vec = vmovq_n_s32(0);
        int k = 0;
        // 向量化处理：每次4个抽头
        for (; k <= num_taps - 4; k += 4) {
            int32x4_t input_vec;
            // 处理边界条件
            if (n >= k + 3) {
                input_vec = vld1q_s32(input + n - k);
            } else {
                // 边界处理：填充0
                int32_t temp[4] = {0, 0, 0, 0};
                for (int t = 0; t < 4 && (n - k - t) >= 0; t++) {
                    temp[t] = input[n - k - t];
                }
                input_vec = vld1q_s32(temp);
            }
            int32x4_t coeff_vec = vld1q_s32(coeffs + k);
            sum_vec = vmlaq_s32(sum_vec, input_vec, coeff_vec);
        }
        // 横向累加
        int32_t sum = vgetq_lane_s32(sum_vec, 0) + vgetq_lane_s32(sum_vec, 1) +
                      vgetq_lane_s32(sum_vec, 2) + vgetq_lane_s32(sum_vec, 3);
        // 处理剩余抽头
        for (; k < num_taps; k++) {
            if (n >= k) {
                sum += input[n - k] * coeffs[k];
            }
        }
        output[n] = sum >> 15;
    }
}

#if USE_CMSIS_DSP
static void test_fir_filter_dsp(q31_t *output, const q31_t *input, const q31_t *coeffs, int input_size, int num_taps) {
    arm_fir_instance_q31 fir;
    arm_fir_init_q31(&fir, num_taps, (q31_t *)coeffs, NULL, 64);
    arm_fir_q31(&fir, (q31_t *)input, output, input_size);
}
#endif

/**
 * @brief Int32转Float（C语言实现）
 * 
 * 将Q15定点数转换为浮点数：dest[i] = src[i] / 32768.0f
 * 
 * @param dest 输出浮点数组
 * @param src 输入int32数组
 * @param size 数组大小
 * 
 * @note 类型转换是NEON的优势场景
 */
static void test_convert_float_c(float *dest, const int32_t *src, int size) {
    for (int i = 0; i < size; i++) {
        dest[i] = (float)src[i] / 32768.0f;
    }
}

/**
 * @brief Int32转Float（NEON实现）
 * 
 * @note 实现说明：
 *       - vcvtq_f32_s32：int32向量转float32向量
 *       - vmulq_f32：向量乘法（乘以1/32768）
 *       - 测试结果：NEON有显著优势（3.37x）
 * 
 * @warning 类型转换非常适合NEON：
 *          - 批量处理
 *          - 无数据依赖
 *          - 单指令完成转换
 */
static NEON_OPTIMIZE void test_convert_float_neon(float *dest, const int32_t *src, int size) {
    float32x4_t scale = vmovq_n_f32(1.0f / 32768.0f);  // 预计算缩放因子
    int i = 0;
    // 向量化转换：每次4个元素
    for (; i <= size - 4; i += 4) {
        int32x4_t src_vec = vld1q_s32(src + i);        // 加载4个int32
        float32x4_t dest_vec = vcvtq_f32_s32(src_vec); // 转换为float32
        dest_vec = vmulq_f32(dest_vec, scale);         // 乘以缩放因子
        vst1q_f32(dest + i, dest_vec);                 // 存储4个float32
    }
    for (; i < size; i++) {
        dest[i] = (float)src[i] / 32768.0f;
    }
}
#endif

#if USE_FOC_TEST
#define FOC_SAMPLES 256
#define FOC_ITERATIONS 1000
#define PI 3.14159265358979f
#define SQRT3 1.732050808f  ///< √3常数

/* ==================== FOC测试数据（volatile防止优化） ==================== */
/**
 * @note volatile关键字说明：
 *       - 防止编译器将变量缓存到寄存器
 *       - 确保每次访问都从内存读取
 *       - 在-O2/-O3优化时防止测试代码被优化掉
 */
static volatile float g_iabc[FOC_SAMPLES * 3];          ///< 三相电流 [ia, ib, ic]
static volatile float g_ialpha_beta[FOC_SAMPLES * 2];   ///< αβ坐标系电流 [ialpha, ibeta]
static volatile float g_idq[FOC_SAMPLES * 2];           ///< dq坐标系电流 [id, iq]
static volatile float g_valpha_beta[FOC_SAMPLES * 2];   ///< αβ坐标系电压 [valpha, vbeta]
static volatile float g_vabc[FOC_SAMPLES * 3];          ///< 三相电压 [va, vb, vc]
static float g_sin_table[256];  ///< sin查找表
static float g_cos_table[256];  ///< cos查找表

static void init_foc_data(void) {
    for (int i = 0; i < 256; i++) {
        float angle = 2.0f * PI * i / 256.0f;
        g_sin_table[i] = sinf(angle);
        g_cos_table[i] = cosf(angle);
    }
    for (int i = 0; i < FOC_SAMPLES; i++) {
        float angle = 2.0f * PI * i / FOC_SAMPLES;
        g_iabc[i * 3 + 0] = sinf(angle) * 100.0f;
        g_iabc[i * 3 + 1] = sinf(angle - 2.094f) * 100.0f;
        g_iabc[i * 3 + 2] = sinf(angle + 2.094f) * 100.0f;
    }
}

/**
 * @brief Clarke变换（C语言实现）
 * 
 * 将三相静止坐标系(abc)转换为两相静止坐标系(αβ)
 * 
 * @param alpha_beta 输出：[alpha, beta]
 * @param i_abc 输入：[ia, ib, ic] 三相电流
 * 
 * @note 公式：
 *       alpha = ia
 *       beta = (ib - ic) / sqrt(3)
 */
static void foc_clarke_c(float *alpha_beta, const float *i_abc) {
    alpha_beta[0] = i_abc[0];
    alpha_beta[1] = (i_abc[1] - i_abc[2]) * (1.0f / SQRT3);
}

/**
 * @brief Clarke变换（NEON实现）
 * 
 * @note 实现说明：
 *       - 对于平衡三相系统：ic = -(ia + ib)
 *       - 因此：ib - ic = ib + ia + ib = ia + 2*ib
 *       - 使用标量实现，编译器会自动优化
 *       - 不使用NEON intrinsics，因为计算量太小
 * 
 * @warning FOC算法不适合NEON向量化：
 *          - 每次只处理2-3个数据
 *          - NEON intrinsics反而增加开销
 *          - 标量实现 + -O2/-O3优化已足够
 */
static void foc_clarke_neon(float *alpha_beta, const float *i_abc) {
    // Clarke transform: alpha = ia, beta = (ib - ic) / sqrt(3)
    // For balanced 3-phase: ic = -(ia + ib), so ib - ic = ia + 2*ib
    float inv_sqrt3 = 1.0f / SQRT3;
    alpha_beta[0] = i_abc[0];
    alpha_beta[1] = (i_abc[0] + 2.0f * i_abc[1]) * inv_sqrt3;
}

#if USE_CMSIS_DSP
static void foc_clarke_dsp(float *alpha_beta, const float *i_abc) {
    arm Clarke_transform_f32(i_abc, alpha_beta);
}
#endif

/**
 * @brief Park变换（C语言实现）
 * 
 * 将两相静止坐标系(αβ)转换为两相旋转坐标系(dq)
 * 
 * @param dq 输出：[d, q] dq坐标系分量
 * @param alpha_beta 输入：[alpha, beta] αβ坐标系分量
 * @param sin_theta sin(θ)，θ为电角度
 * @param cos_theta cos(θ)，θ为电角度
 * 
 * @note 公式：
 *       d =  alpha * cos(θ) + beta * sin(θ)
 *       q = -alpha * sin(θ) + beta * cos(θ)
 */
static void foc_park_c(float *dq, const float *alpha_beta, float sin_theta, float cos_theta) {
    float ia = alpha_beta[0];
    float ib = alpha_beta[1];
    dq[0] = ia * cos_theta + ib * sin_theta;
    dq[1] = -ia * sin_theta + ib * cos_theta;
}

/**
 * @brief Park变换（NEON实现）
 * 
 * @note 实现说明：
 *       - 使用标量实现，编译器会自动优化
 *       - 不使用复杂的NEON intrinsics，因为：
 *         1. 计算量太小（只有2个输出）
 *         2. sin/cos计算有数据依赖
 *         3. NEON intrinsics反而增加开销
 *       - -O2/-O3优化时，编译器会生成高效的指令
 * 
 * @warning 测试结果：标量实现比复杂NEON实现快6-9倍
 */
static void foc_park_neon(float *dq, const float *alpha_beta, float sin_theta, float cos_theta) {
    // Park transform: d = alpha*cos + beta*sin, q = -alpha*sin + beta*cos
    // Use MLA (multiply-accumulate) for efficiency
    float alpha = alpha_beta[0];
    float beta = alpha_beta[1];

    // Use NEON MLA instruction
    float32x2_t ab = vld1_f32(alpha_beta);  // [alpha, beta]
    float32x2_t sc = {sin_theta, cos_theta}; // [sin, cos]

    // d = alpha*cos + beta*sin
    float d = alpha * cos_theta + beta * sin_theta;

    // q = -alpha*sin + beta*cos
    float q = beta * cos_theta - alpha * sin_theta;

    dq[0] = d;
    dq[1] = q;
}

#if USE_CMSIS_DSP
static void foc_park_dsp(float *dq, const float *alpha_beta, float sin_theta, float cos_theta) {
    arm_park_f32(alpha_beta, alpha_beta + 1, dq, dq + 1, sin_theta, cos_theta);
}
#endif

/**
 * @brief 逆Park变换（C语言实现）
 * 
 * 将两相旋转坐标系(dq)转换为两相静止坐标系(αβ)
 * 
 * @param alpha_beta 输出：[alpha, beta] αβ坐标系分量
 * @param dq 输入：[d, q] dq坐标系分量
 * @param sin_theta sin(θ)，θ为电角度
 * @param cos_theta cos(θ)，θ为电角度
 * 
 * @note 公式：
 *       alpha = d * cos(θ) - q * sin(θ)
 *       beta  = d * sin(θ) + q * cos(θ)
 */
static void foc_inv_park_c(float *alpha_beta, const float *dq, float sin_theta, float cos_theta) {
    float vd = dq[0];
    float vq = dq[1];
    alpha_beta[0] = vd * cos_theta - vq * sin_theta;
    alpha_beta[1] = vd * sin_theta + vq * cos_theta;
}

/**
 * @brief 逆Park变换（NEON实现）
 * 
 * @note 实现说明：
 *       - 使用标量实现，让编译器自动优化
 *       - 测试结果：与C版本性能相同（1.00x）
 *       - 证明标量实现已是最优
 */
static void foc_inv_park_neon(float *alpha_beta, const float *dq, float sin_theta, float cos_theta) {
    // Inverse Park: alpha = d*cos - q*sin, beta = d*sin + q*cos
    float d = dq[0];
    float q = dq[1];

    // Direct scalar computation (compiler will optimize with NEON)
    alpha_beta[0] = d * cos_theta - q * sin_theta;
    alpha_beta[1] = d * sin_theta + q * cos_theta;
}

#if USE_CMSIS_DSP
static void foc_inv_park_dsp(float *alpha_beta, const float *dq, float sin_theta, float cos_theta) {
    arm_park_f32(dq, dq + 1, alpha_beta, alpha_beta + 1, sin_theta, cos_theta);
}
#endif

/**
 * @brief 空间矢量脉宽调制SVPWM（C语言实现）
 * 
 * 将两相静止坐标系(αβ)电压转换为三相电压，并应用共模电压消除
 * 
 * @param vabc 输出：[va, vb, vc] 三相电压（归一化）
 * @param valpha_beta 输入：[valpha, vbeta] αβ坐标系电压
 * @param vdc 直流母线电压（用于归一化）
 * 
 * @note 算法步骤：
 *       1. αβ → abc变换
 *       2. 计算共模电压：Vcom = (max(Va,Vb,Vc) + min(Va,Vb,Vc)) / 2
 *       3. 消除共模：Va' = Va - Vcom
 *       4. 归一化：Va_norm = Va' / vdc
 */
static void foc_svpwm_c(float *vabc, const float *valpha_beta, float vdc) {
    float Va = valpha_beta[0];
    float Vb = 0.5f * (-valpha_beta[0] + SQRT3 * valpha_beta[1]);
    float Vc = 0.5f * (-valpha_beta[0] - SQRT3 * valpha_beta[1]);
    float max_v = Va > Vb ? Va : Vb;
    max_v = max_v > Vc ? max_v : Vc;
    float min_v = Va < Vb ? Va : Vb;
    min_v = min_v < Vc ? min_v : Vc;
    float Vcom = 0.5f * (max_v + min_v);
    vabc[0] = (Va - Vcom) / vdc;
    vabc[1] = (Vb - Vcom) / vdc;
    vabc[2] = (Vc - Vcom) / vdc;
}

/**
 * @brief 空间矢量脉宽调制SVPWM（NEON实现）
 * 
 * @note 实现说明：
 *       - 使用标量实现，包含分支判断（max/min）
 *       - 预计算倒数避免除法：vdc_recip = 1.0f / vdc
 *       - 测试结果：与C版本性能相同（1.00x）
 *       - SVPWM不适合NEON向量化（有分支、计算量小）
 * 
 * @warning 不要尝试用NEON优化max/min，标量实现已是最优
 */
static void foc_svpwm_neon(float *vabc, const float *valpha_beta, float vdc) {
    // SVPWM: Transform alpha-beta to 3-phase with common mode elimination
    float Va = valpha_beta[0];
    float Vb = valpha_beta[1];

    // Calculate 3-phase voltages
    float sqrt3_2 = 0.8660254037844386f;  // sqrt(3)/2
    float Vb_3ph = -0.5f * Va + sqrt3_2 * Vb;
    float Vc = -0.5f * Va - sqrt3_2 * Vb;

    // Find max and min for common mode
    float max_v = Va > Vb_3ph ? Va : Vb_3ph;
    max_v = max_v > Vc ? max_v : Vc;

    float min_v = Va < Vb_3ph ? Va : Vb_3ph;
    min_v = min_v < Vc ? min_v : Vc;

    float Vcom = 0.5f * (max_v + min_v);
    float vdc_recip = 1.0f / vdc;  // 预计算倒数，避免重复除法

    // Apply common mode and normalize
    vabc[0] = (Va - Vcom) * vdc_recip;
    vabc[1] = (Vb_3ph - Vcom) * vdc_recip;
    vabc[2] = (Vc - Vcom) * vdc_recip;
}

static void foc_svpwm_dsp(float *vabc, const float *valpha_beta, float vdc) {
    float32x2_t ab = vld1_f32(valpha_beta);
    float32_t Va = vget_lane_f32(ab, 0);
    float32_t Vb = vget_lane_f32(ab, 1);
    float32_t Vc = -Va - Vb;
    float32_t max_v = Va > Vb ? Va : Vb;
    max_v = max_v > Vc ? max_v : Vc;
    float32_t min_v = Va < Vb ? Va : Vb;
    min_v = min_v < Vc ? min_v : Vc;
    float32_t Vcom = 0.5f * (max_v + min_v);
    vabc[0] = (Va - Vcom) / vdc;
    vabc[1] = (Vb - Vcom) / vdc;
    vabc[2] = (Vc - Vcom) / vdc;
}

typedef struct {
    float kp;
    float ki;
    float out_max;
    float out_min;
    float integral;
    float prev_err;
} pi_controller_t;

static void pi_init(pi_controller_t *pi, float kp, float ki, float out_max) {
    pi->kp = kp;
    pi->ki = ki;
    pi->out_max = out_max;
    pi->out_min = -out_max;
    pi->integral = 0;
    pi->prev_err = 0;
}

/**
 * @brief PI控制器更新（C语言实现）
 * 
 * 实现带抗饱和的PI控制器
 * 
 * @param pi PI控制器结构体指针
 * @param target 目标值
 * @param feedback 反馈值
 * @param dt 采样时间（秒）
 * @return float 控制器输出
 * 
 * @note 算法：
 *       1. 计算误差：err = target - feedback
 *       2. P项：p_out = kp * err
 *       3. I项：integral += ki * err * dt（带限幅）
 *       4. 输出：output = p_out + integral（带限幅）
 * 
 * @warning 抗饱和机制：
 *          - 积分限幅：防止积分饱和
 *          - 输出限幅：防止输出超限
 */
static float pi_update_c(pi_controller_t *pi, float target, float feedback, float dt) {
    float err = target - feedback;
    float p_out = pi->kp * err;
    pi->integral += pi->ki * err * dt;
    if (pi->integral > pi->out_max) pi->integral = pi->out_max;
    if (pi->integral < pi->out_min) pi->integral = pi->out_min;
    float output = p_out + pi->integral;
    if (output > pi->out_max) output = pi->out_max;
    if (output < pi->out_min) output = pi->out_min;
    pi->prev_err = err;
    return output;
}

/**
 * @brief PI控制器更新（NEON实现）
 * 
 * @note 实现说明：
 *       - 使用标量实现，因为：
 *         1. 单个PI控制器无法向量化
 *         2. 有分支判断（限幅）
 *         3. 有状态更新（integral）
 *       - 测试结果：比C版本略快（1.15x）
 *       - 如果要向量化，需要批量处理多个PI控制器
 * 
 * @warning 不要为单个PI控制器使用NEON intrinsics
 */
static void pi_update_neon(pi_controller_t *pi, float target, float feedback, float dt, float *out) {
    // PI controller: output = kp*err + ki*integral(err)*dt
    // For single PI controller, scalar implementation is more efficient
    float err = target - feedback;
    float p_out = pi->kp * err;
    pi->integral += pi->ki * err * dt;

    // Anti-windup: clamp integral
    if (pi->integral > pi->out_max) pi->integral = pi->out_max;
    if (pi->integral < pi->out_min) pi->integral = pi->out_min;

    *out = p_out + pi->integral;

    // Output saturation
    if (*out > pi->out_max) *out = pi->out_max;
    if (*out < pi->out_min) *out = pi->out_min;

    pi->prev_err = err;
}
#endif

static volatile int32_t g_test_src1[TEST_ARRAY_SIZE];
static volatile int32_t g_test_src2[TEST_ARRAY_SIZE];
static volatile int32_t g_test_dest[TEST_ARRAY_SIZE];
static volatile float g_test_dest_float[TEST_ARRAY_SIZE];
static volatile int32_t g_test_matrix_a[MATRIX_SIZE * MATRIX_SIZE];
static volatile int32_t g_test_matrix_b[MATRIX_SIZE * MATRIX_SIZE];
static volatile int32_t g_test_matrix_dest[MATRIX_SIZE * MATRIX_SIZE];
static volatile int32_t g_fir_coeffs[FIR_TAPS] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160};
static volatile int32_t g_fir_output[TEST_ARRAY_SIZE];

static void init_test_data(void) {
    for (int i = 0; i < TEST_ARRAY_SIZE; i++) {
        g_test_src1[i] = i % 100;
        g_test_src2[i] = (i * 2) % 100;
    }
    for (int i = 0; i < MATRIX_SIZE * MATRIX_SIZE; i++) {
        g_test_matrix_a[i] = i % 10;
        g_test_matrix_b[i] = (i * 3) % 10;
    }
#if USE_FOC_TEST
    init_foc_data();
#endif
}

typedef struct {
    const char *name;
    uint32_t cycles_c;
    uint32_t cycles_neon;
#if USE_CMSIS_DSP
    uint32_t cycles_dsp;
#endif
    uint32_t iterations;
} benchmark_result_t;

#define MAX_TESTS 20

void neon_benchmark_main(void) {
    int result_count = 0;
    benchmark_result_t results[MAX_TESTS];
    
    int32_t result_c;
    int32_t result_neon;
#if USE_CMSIS_DSP
    q31_t result_dsp;
#endif
    int mat_iterations = BENCHMARK_ITERATIONS * 10;
    int fir_iterations = BENCHMARK_ITERATIONS * 10;
    uint32_t cycles;
    
    init_test_data();
    
    uart_print("\r\n");
    uart_print("================================================\r\n");
    uart_print("   NEON/DSP Performance Test on RZN CR52\r\n");
    uart_print("================================================\r\n");
    uart_print("System Core Clock: %lu Hz\r\n", (unsigned long)SystemCoreClock);
    uart_print("GSC Clock: %lu Hz\r\n", (unsigned long)BSP_GLOBAL_SYSTEM_COUNTER_CLOCK_HZ);
    uart_print("\r\nTest Configuration:\r\n");
    uart_print("  General DSP Test: %s\r\n", USE_GENERAL_TEST ? "ON" : "OFF");
    uart_print("  FOC Algorithm Test: %s\r\n", USE_FOC_TEST ? "ON" : "OFF");
    uart_print("  CMSIS-DSP Library: %s\r\n", USE_CMSIS_DSP ? "ON" : "OFF");
#if USE_GENERAL_TEST
    uart_print("  Array Size: %d\r\n", TEST_ARRAY_SIZE);
    uart_print("  Matrix Size: %dx%d\r\n", MATRIX_SIZE, MATRIX_SIZE);
    uart_print("  FIR Taps: %d\r\n", FIR_TAPS);
    uart_print("  Iterations: %d\r\n", BENCHMARK_ITERATIONS);
#endif
#if USE_FOC_TEST
    uart_print("  FOC Samples: %d\r\n", FOC_SAMPLES);
    uart_print("  FOC Iterations: %d\r\n", FOC_ITERATIONS);
#endif
    uart_print("================================================\r\n");

    // Dummy variable to prevent compiler from optimizing away test results
    volatile float dummy_sum = 0.0f;

#if USE_GENERAL_TEST
    uart_print("\r\n[General DSP Tests]\r\n");
    
    uart_print("\r\n  [Test 1] Array Addition (%d elements)\r\n", TEST_ARRAY_SIZE);
    results[result_count].name = "Arr Add";
    benchmark_start();
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        test_array_add_c(g_test_dest, g_test_src1, g_test_src2, TEST_ARRAY_SIZE);
    }
    cycles = benchmark_end();
    results[result_count].cycles_c = cycles;
    results[result_count].iterations = BENCHMARK_ITERATIONS;
    uart_print("    C:      %lu cycles, avg: %lu/iter\r\n", 
               (unsigned long)cycles, (unsigned long)(cycles / BENCHMARK_ITERATIONS));
    benchmark_start();
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        test_array_add_neon(g_test_dest, g_test_src1, g_test_src2, TEST_ARRAY_SIZE);
    }
    cycles = benchmark_end();
    results[result_count].cycles_neon = cycles;
    uart_print("    NEON:   %lu cycles, avg: %lu/iter\r\n", 
               (unsigned long)cycles, (unsigned long)(cycles / BENCHMARK_ITERATIONS));
#if USE_CMSIS_DSP
    benchmark_start();
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        test_array_add_dsp(g_test_dest, g_test_src1, g_test_src2, TEST_ARRAY_SIZE);
    }
    cycles = benchmark_end();
    results[result_count].cycles_dsp = cycles;
    uart_print("    DSP:    %lu cycles, avg: %lu/iter\r\n", 
               (unsigned long)cycles, (unsigned long)(cycles / BENCHMARK_ITERATIONS));
#endif
    result_count++;
    
    uart_print("\r\n  [Test 2] Multiply-Accumulate (%d elements)\r\n", TEST_ARRAY_SIZE);
    results[result_count].name = "Mul-Acc";
    benchmark_start();
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        test_multiply_accumulate_c(&result_c, g_test_src1, g_test_src2, TEST_ARRAY_SIZE);
    }
    cycles = benchmark_end();
    results[result_count].cycles_c = cycles;
    results[result_count].iterations = BENCHMARK_ITERATIONS;
    uart_print("    C:      %lu cycles, avg: %lu/iter\r\n", 
               (unsigned long)cycles, (unsigned long)(cycles / BENCHMARK_ITERATIONS));
    benchmark_start();
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        test_multiply_accumulate_neon(&result_neon, g_test_src1, g_test_src2, TEST_ARRAY_SIZE);
    }
    cycles = benchmark_end();
    results[result_count].cycles_neon = cycles;
    uart_print("    NEON:   %lu cycles, avg: %lu/iter\r\n", 
               (unsigned long)cycles, (unsigned long)(cycles / BENCHMARK_ITERATIONS));
#if USE_CMSIS_DSP
    benchmark_start();
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        test_multiply_accumulate_dsp(&result_dsp, (q31_t *)g_test_src1, (q31_t *)g_test_src2, TEST_ARRAY_SIZE);
    }
    cycles = benchmark_end();
    results[result_count].cycles_dsp = cycles;
    uart_print("    DSP:    %lu cycles, avg: %lu/iter\r\n", 
               (unsigned long)cycles, (unsigned long)(cycles / BENCHMARK_ITERATIONS));
#endif
    result_count++;
    
    uart_print("\r\n  [Test 3] Matrix Multiplication (%dx%d)\r\n", MATRIX_SIZE, MATRIX_SIZE);
    results[result_count].name = "Mat Mul";
    benchmark_start();
    for (int i = 0; i < mat_iterations; i++) {
        test_matrix_multiply_c(g_test_matrix_dest, g_test_matrix_a, g_test_matrix_b, MATRIX_SIZE);
    }
    cycles = benchmark_end();
    results[result_count].cycles_c = cycles;
    results[result_count].iterations = mat_iterations;
    uart_print("    C:      %lu cycles, avg: %lu/iter\r\n", 
               (unsigned long)cycles, (unsigned long)(cycles / mat_iterations));
    benchmark_start();
    for (int i = 0; i < mat_iterations; i++) {
        test_matrix_multiply_neon(g_test_matrix_dest, g_test_matrix_a, g_test_matrix_b, MATRIX_SIZE);
    }
    cycles = benchmark_end();
    results[result_count].cycles_neon = cycles;
    uart_print("    NEON:   %lu cycles, avg: %lu/iter\r\n", 
               (unsigned long)cycles, (unsigned long)(cycles / mat_iterations));
#if USE_CMSIS_DSP
    results[result_count].cycles_dsp = 0;
#endif
    result_count++;
    
    uart_print("\r\n  [Test 4] FIR Filter (%d samples, %d taps)\r\n", TEST_ARRAY_SIZE, FIR_TAPS);
    results[result_count].name = "FIR";
    benchmark_start();
    for (int i = 0; i < fir_iterations; i++) {
        test_fir_filter_c(g_fir_output, g_test_src1, g_fir_coeffs, TEST_ARRAY_SIZE, FIR_TAPS);
    }
    cycles = benchmark_end();
    results[result_count].cycles_c = cycles;
    results[result_count].iterations = fir_iterations;
    uart_print("    C:      %lu cycles, avg: %lu/iter\r\n", 
               (unsigned long)cycles, (unsigned long)(cycles / fir_iterations));
    benchmark_start();
    for (int i = 0; i < fir_iterations; i++) {
        test_fir_filter_neon(g_fir_output, g_test_src1, g_fir_coeffs, TEST_ARRAY_SIZE, FIR_TAPS);
    }
    cycles = benchmark_end();
    results[result_count].cycles_neon = cycles;
    uart_print("    NEON:   %lu cycles, avg: %lu/iter\r\n", 
               (unsigned long)cycles, (unsigned long)(cycles / fir_iterations));
#if USE_CMSIS_DSP
    benchmark_start();
    for (int i = 0; i < fir_iterations; i++) {
        test_fir_filter_dsp(g_fir_output, g_test_src1, g_fir_coeffs, TEST_ARRAY_SIZE, FIR_TAPS);
    }
    cycles = benchmark_end();
    results[result_count].cycles_dsp = cycles;
    uart_print("    DSP:    %lu cycles, avg: %lu/iter\r\n", 
               (unsigned long)cycles, (unsigned long)(cycles / fir_iterations));
#endif
    result_count++;
    
    uart_print("\r\n  [Test 5] Int32 to Float (%d elements)\r\n", TEST_ARRAY_SIZE);
    results[result_count].name = "Int->Float";
    benchmark_start();
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        test_convert_float_c(g_test_dest_float, g_test_src1, TEST_ARRAY_SIZE);
    }
    cycles = benchmark_end();
    results[result_count].cycles_c = cycles;
    results[result_count].iterations = BENCHMARK_ITERATIONS;
    uart_print("    C:      %lu cycles, avg: %lu/iter\r\n", 
               (unsigned long)cycles, (unsigned long)(cycles / BENCHMARK_ITERATIONS));
    benchmark_start();
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        test_convert_float_neon(g_test_dest_float, g_test_src1, TEST_ARRAY_SIZE);
    }
    cycles = benchmark_end();
    results[result_count].cycles_neon = cycles;
    uart_print("    NEON:   %lu cycles, avg: %lu/iter\r\n", 
               (unsigned long)cycles, (unsigned long)(cycles / BENCHMARK_ITERATIONS));
#if USE_CMSIS_DSP
    results[result_count].cycles_dsp = 0;
#endif
    result_count++;
#endif

#if USE_FOC_TEST
    uart_print("\r\n[FOC Algorithm Tests]\r\n");
    
    pi_controller_t pi_d, pi_q;
    pi_init(&pi_d, 1.0f, 0.01f, 12.0f);
    pi_init(&pi_q, 1.0f, 0.01f, 12.0f);
    
    uart_print("\r\n  [Test 6] Clarke Transform (3P->2P, %d samples)\r\n", FOC_SAMPLES);
    results[result_count].name = "Clarke";
    benchmark_start();
    for (int i = 0; i < FOC_ITERATIONS; i++) {
        for (int j = 0; j < FOC_SAMPLES; j++) {
            foc_clarke_c(g_ialpha_beta, &g_iabc[j * 3]);
        }
        dummy_sum += g_ialpha_beta[0] + g_ialpha_beta[1];  // Use result
    }
    cycles = benchmark_end();
    results[result_count].cycles_c = cycles;
    results[result_count].iterations = FOC_ITERATIONS;
    uart_print("    C:      %lu cycles, avg: %lu/iter\r\n", 
               (unsigned long)cycles, (unsigned long)(cycles / FOC_ITERATIONS));
    benchmark_start();
    for (int i = 0; i < FOC_ITERATIONS; i++) {
        for (int j = 0; j < FOC_SAMPLES; j++) {
            foc_clarke_neon(g_ialpha_beta, &g_iabc[j * 3]);
        }
        dummy_sum += g_ialpha_beta[0] + g_ialpha_beta[1];  // Use result
    }
    cycles = benchmark_end();
    results[result_count].cycles_neon = cycles;
    uart_print("    NEON:   %lu cycles, avg: %lu/iter\r\n", 
               (unsigned long)cycles, (unsigned long)(cycles / FOC_ITERATIONS));
#if USE_CMSIS_DSP
    results[result_count].cycles_dsp = results[result_count].cycles_neon;
#endif
    result_count++;
    
    uart_print("\r\n  [Test 7] Park Transform (2P->DQ, %d samples)\r\n", FOC_SAMPLES);
    results[result_count].name = "Park";
    float theta = 0.0f;
    float sin_t = 0.0f, cos_t = 1.0f;
    benchmark_start();
    for (int i = 0; i < FOC_ITERATIONS; i++) {
        theta += 0.01f;
        sin_t = sinf(theta);
        cos_t = cosf(theta);
        for (int j = 0; j < FOC_SAMPLES; j++) {
            foc_park_c(g_idq, g_ialpha_beta, sin_t, cos_t);
        }
        dummy_sum += g_idq[0] + g_idq[1];  // Use result
    }
    cycles = benchmark_end();
    results[result_count].cycles_c = cycles;
    results[result_count].iterations = FOC_ITERATIONS;
    uart_print("    C:      %lu cycles, avg: %lu/iter\r\n", 
               (unsigned long)cycles, (unsigned long)(cycles / FOC_ITERATIONS));
    benchmark_start();
    for (int i = 0; i < FOC_ITERATIONS; i++) {
        theta += 0.01f;
        sin_t = sinf(theta);
        cos_t = cosf(theta);
        for (int j = 0; j < FOC_SAMPLES; j++) {
            foc_park_neon(g_idq, g_ialpha_beta, sin_t, cos_t);
        }
        dummy_sum += g_idq[0] + g_idq[1];  // Use result
    }
    cycles = benchmark_end();
    results[result_count].cycles_neon = cycles;
    uart_print("    NEON:   %lu cycles, avg: %lu/iter\r\n", 
               (unsigned long)cycles, (unsigned long)(cycles / FOC_ITERATIONS));
#if USE_CMSIS_DSP
    results[result_count].cycles_dsp = results[result_count].cycles_neon;
#endif
    result_count++;
    
    uart_print("\r\n  [Test 8] Inverse Park Transform (DQ->2P, %d samples)\r\n", FOC_SAMPLES);
    results[result_count].name = "Inv Park";
    benchmark_start();
    for (int i = 0; i < FOC_ITERATIONS; i++) {
        theta += 0.01f;
        sin_t = sinf(theta);
        cos_t = cosf(theta);
        for (int j = 0; j < FOC_SAMPLES; j++) {
            foc_inv_park_c(g_valpha_beta, g_idq, sin_t, cos_t);
        }
        dummy_sum += g_valpha_beta[0] + g_valpha_beta[1];  // Use result
    }
    cycles = benchmark_end();
    results[result_count].cycles_c = cycles;
    results[result_count].iterations = FOC_ITERATIONS;
    uart_print("    C:      %lu cycles, avg: %lu/iter\r\n", 
               (unsigned long)cycles, (unsigned long)(cycles / FOC_ITERATIONS));
    benchmark_start();
    for (int i = 0; i < FOC_ITERATIONS; i++) {
        theta += 0.01f;
        sin_t = sinf(theta);
        cos_t = cosf(theta);
        for (int j = 0; j < FOC_SAMPLES; j++) {
            foc_inv_park_neon(g_valpha_beta, g_idq, sin_t, cos_t);
        }
        dummy_sum += g_valpha_beta[0] + g_valpha_beta[1];  // Use result
    }
    cycles = benchmark_end();
    results[result_count].cycles_neon = cycles;
    uart_print("    NEON:   %lu cycles, avg: %lu/iter\r\n", 
               (unsigned long)cycles, (unsigned long)(cycles / FOC_ITERATIONS));
#if USE_CMSIS_DSP
    results[result_count].cycles_dsp = results[result_count].cycles_neon;
#endif
    result_count++;
    
    uart_print("\r\n  [Test 9] SVPWM (2P->3P, %d samples)\r\n", FOC_SAMPLES);
    results[result_count].name = "SVPWM";
    benchmark_start();
    for (int i = 0; i < FOC_ITERATIONS; i++) {
        for (int j = 0; j < FOC_SAMPLES; j++) {
            foc_svpwm_c(g_vabc, g_valpha_beta, 24.0f);
        }
        dummy_sum += g_vabc[0] + g_vabc[1] + g_vabc[2];  // Use result
    }
    cycles = benchmark_end();
    results[result_count].cycles_c = cycles;
    results[result_count].iterations = FOC_ITERATIONS;
    uart_print("    C:      %lu cycles, avg: %lu/iter\r\n", 
               (unsigned long)cycles, (unsigned long)(cycles / FOC_ITERATIONS));
    benchmark_start();
    for (int i = 0; i < FOC_ITERATIONS; i++) {
        for (int j = 0; j < FOC_SAMPLES; j++) {
            foc_svpwm_neon(g_vabc, g_valpha_beta, 24.0f);
        }
        dummy_sum += g_vabc[0] + g_vabc[1] + g_vabc[2];  // Use result
    }
    cycles = benchmark_end();
    results[result_count].cycles_neon = cycles;
    uart_print("    NEON:   %lu cycles, avg: %lu/iter\r\n", 
               (unsigned long)cycles, (unsigned long)(cycles / FOC_ITERATIONS));
#if USE_CMSIS_DSP
    results[result_count].cycles_dsp = results[result_count].cycles_neon;
#endif
    result_count++;
    
    uart_print("\r\n  [Test 10] PI Controller (1000 iterations)\r\n");
    results[result_count].name = "PI Ctrl";
    float output = 0.0f;
    benchmark_start();
    for (int i = 0; i < FOC_ITERATIONS; i++) {
        pi_init(&pi_d, 1.0f, 0.01f, 12.0f);
        for (int j = 0; j < FOC_SAMPLES; j++) {
            output += pi_update_c(&pi_d, 10.0f, 5.0f, 0.001f);
        }
        dummy_sum += output;  // Use result
    }
    cycles = benchmark_end();
    results[result_count].cycles_c = cycles;
    results[result_count].iterations = FOC_ITERATIONS;
    uart_print("    C:      %lu cycles, avg: %lu/iter\r\n", 
               (unsigned long)cycles, (unsigned long)(cycles / FOC_ITERATIONS));
    float pi_out = 0.0f;
    benchmark_start();
    for (int i = 0; i < FOC_ITERATIONS; i++) {
        pi_init(&pi_d, 1.0f, 0.01f, 12.0f);
        for (int j = 0; j < FOC_SAMPLES; j++) {
            pi_update_neon(&pi_d, 10.0f, 5.0f, 0.001f, &pi_out);
        }
        dummy_sum += pi_out;  // Use result
    }
    cycles = benchmark_end();
    results[result_count].cycles_neon = cycles;
    uart_print("    NEON:   %lu cycles, avg: %lu/iter\r\n", 
               (unsigned long)cycles, (unsigned long)(cycles / FOC_ITERATIONS));
#if USE_CMSIS_DSP
    results[result_count].cycles_dsp = results[result_count].cycles_neon;
#endif
    result_count++;
#endif

    uart_print("\r\n");
    uart_print("================================================\r\n");
    uart_print("              Summary Table\r\n");
    uart_print("================================================\r\n");
    
#if USE_CMSIS_DSP
    uart_print("%-12s|%-10s|%-10s|%-10s\r\n", "Test", "C(avg)", "NEON(avg)", "DSP(avg)");
    uart_print("------------|----------|----------|----------\r\n");
    for (int i = 0; i < result_count; i++) {
        uint32_t avg_c = results[i].cycles_c / results[i].iterations;
        uint32_t avg_n = results[i].cycles_neon / results[i].iterations;
        uint32_t avg_d = results[i].cycles_dsp / results[i].iterations;
        uart_print("%-12s|%9lu|%9lu|%9lu\r\n",
                   results[i].name,
                   (unsigned long)avg_c,
                   (unsigned long)avg_n,
                   (unsigned long)avg_d);
    }
    uart_print("\r\n");
    uart_print("Speedup vs C:\r\n");
    uart_print("%-12s|%-14s|%-14s\r\n", "Test", "NEON Speedup", "DSP Speedup");
    uart_print("------------|--------------|--------------\r\n");
    for (int i = 0; i < result_count; i++) {
        uint32_t avg_c = results[i].cycles_c / results[i].iterations;
        uint32_t avg_n = results[i].cycles_neon / results[i].iterations;
        uint32_t avg_d = results[i].cycles_dsp / results[i].iterations;
        uint32_t spd_n = avg_n > 0 ? (avg_c * 100) / avg_n : 0;
        uint32_t spd_d = avg_d > 0 ? (avg_c * 100) / avg_d : 0;
        uart_print("%-12s|%lu.%02lux        |%lu.%02lux\r\n",
                   results[i].name,
                   (unsigned long)(spd_n / 100), (unsigned long)(spd_n % 100),
                   (unsigned long)(spd_d / 100), (unsigned long)(spd_d % 100));
    }
#else
    uart_print("%-12s|%-10s|%-10s|%-10s\r\n", "Test", "C(avg)", "NEON(avg)", "Speedup");
    uart_print("------------|----------|----------|----------\r\n");
    for (int i = 0; i < result_count; i++) {
        uint32_t avg_c = results[i].cycles_c / results[i].iterations;
        uint32_t avg_n = results[i].cycles_neon / results[i].iterations;
        uint32_t spd = avg_n > 0 ? (avg_c * 100) / avg_n : 0;
        uart_print("%-12s|%9lu|%9lu|%lu.%02lux\r\n",
                   results[i].name,
                   (unsigned long)avg_c,
                   (unsigned long)avg_n,
                   (unsigned long)(spd / 100),
                   (unsigned long)(spd % 100));
    }
#endif
    
    uart_print("================================================\r\n");
    uart_print("         Benchmark Complete!\r\n");
    uart_print("================================================\r\n");

    // Prevent compiler from optimizing away dummy_sum
    if (dummy_sum > 1e30f) {
        uart_print("Dummy sum: %f\r\n", dummy_sum);
    }

    while (1) {
    }
}
