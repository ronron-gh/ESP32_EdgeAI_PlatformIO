// Copyright 2018,2019,2020,2021 Sony Corporation.
// Copyright 2021 Sony Group Corporation.
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

#include "../../utilities/shape.h"
#include "arithmetic.h"
#include <nnablart/config.h>
#include <nnablart/functions.h>

#ifdef CONFIG_DIV2

rt_function_error_t exec_div2_generic(rt_function_t *f);
rt_function_error_t exec_div2_fixed8(rt_function_t *f);
rt_function_error_t exec_div2_fixed16(rt_function_t *f);

// Div2
rt_function_error_t allocate_div2_local_context(rt_function_t *f) {
  if (f->num_of_inputs != 2) {
    return RT_FUNCTION_ERROR_INVALID_NUM_OF_INPUTS;
  }
  if (f->num_of_outputs != 1) {
    return RT_FUNCTION_ERROR_INVALID_NUM_OF_OUTPUTS;
  }
  if (f->inputs[0]->shape.size != f->inputs[1]->shape.size) {
    return RT_FUNCTION_ERROR_INVALID_SHAPE;
  }
  if (f->outputs[0]->shape.size != f->inputs[0]->shape.size) {
    return RT_FUNCTION_ERROR_INVALID_SHAPE;
  }
  if (f->inputs[0]->type == NN_DATA_TYPE_FLOAT &&
      f->inputs[1]->type == NN_DATA_TYPE_FLOAT &&
      f->outputs[0]->type == NN_DATA_TYPE_FLOAT) {
#ifdef CONFIG_DIV2_FLOAT32
    f->exec_func = exec_div2;
#endif /* CONFIG_DIV2_FLOAT32 */
  }

  else if (f->inputs[0]->type == NN_DATA_TYPE_INT16 &&
           f->inputs[1]->type == NN_DATA_TYPE_INT16 &&
           f->outputs[0]->type == NN_DATA_TYPE_INT16) {
#ifdef CONFIG_DIV2_FIXED16
    f->exec_func = exec_div2_fixed16;
#endif /* CONFIG_DIV2_FIXED16 */
  }

  else if (f->inputs[0]->type == NN_DATA_TYPE_INT8 &&
           f->inputs[1]->type == NN_DATA_TYPE_INT8 &&
           f->outputs[0]->type == NN_DATA_TYPE_INT8) {
#ifdef CONFIG_DIV2_FIXED8
    f->exec_func = exec_div2_fixed8;
#endif /* CONFIG_DIV2_FIXED8 */
  }

  else {
#ifdef CONFIG_DIV2_GENERIC
    f->exec_func = exec_div2_generic;
#endif /* CONFIG_DIV2_GENERIC */
  }
  return RT_FUNCTION_ERROR_NOERROR;
}

rt_function_error_t free_div2_local_context(rt_function_t *f) {
  return RT_FUNCTION_ERROR_NOERROR;
}

#ifdef CONFIG_DIV2_FLOAT32
rt_function_error_t exec_div2(rt_function_t *f) {
  calc_arithmetic(f, calc_div);
  return RT_FUNCTION_ERROR_NOERROR;
}
#endif /* CONFIG_DIV2_FLOAT32 */

#ifdef CONFIG_DIV2_FIXED8
rt_function_error_t exec_div2_fixed8(rt_function_t *f) {
  const short n_bits_rescale =
      -f->inputs[0]->fp_pos + f->inputs[1]->fp_pos + f->outputs[0]->fp_pos;
  const int rescaling_factor = 1 << n_bits_rescale;
  calc_arithmetic_fixed8_largebuff(f, calc_div_fixed8, rescaling_factor);

  return RT_FUNCTION_ERROR_NOERROR;
}
#endif /* CONFIG_DIV2_FIXED8 */

#ifdef CONFIG_DIV2_FIXED16
rt_function_error_t exec_div2_fixed16(rt_function_t *f) {
  const short n_bits_rescale =
      -f->inputs[0]->fp_pos + f->inputs[1]->fp_pos + f->outputs[0]->fp_pos;
  const int rescaling_factor = 1 << n_bits_rescale;
  calc_arithmetic_fixed16_largebuff(f, calc_div_fixed16, rescaling_factor);

  return RT_FUNCTION_ERROR_NOERROR;
}
#endif /* CONFIG_DIV2_FIXED16 */

#ifdef CONFIG_DIV2_GENERIC
rt_function_error_t exec_div2_generic(rt_function_t *f) {
  calc_arithmetic_generic(f, calc_div);
  return RT_FUNCTION_ERROR_NOERROR;
}
#endif /* CONFIG_DIV2_GENERIC */

#endif /* CONFIG_DIV2 */
