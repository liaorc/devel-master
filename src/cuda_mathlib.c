const char *pgstrom_cuda_mathlib_code =
  "/*\n"
  " * opencl_math.h\n"
  " *\n"
  " * Collection of math functions for OpenCL devices\n"
  " * --\n"
  " * Copyright 2011-2016 (C) KaiGai Kohei <kaigai@kaigai.gr.jp>\n"
  " * Copyright 2014-2016 (C) The PG-Strom Development Team\n"
  " *\n"
  " * This program is free software; you can redistribute it and/or modify\n"
  " * it under the terms of the GNU General Public License version 2 as\n"
  " * published by the Free Software Foundation.\n"
  " *\n"
  " * This program is distributed in the hope that it will be useful,\n"
  " * but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
  " * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
  " * GNU General Public License for more details.\n"
  " */\n"
  "#ifndef CUDA_MATH_H\n"
  "#define CUDA_MATH_H\n"
  "#ifdef __CUDACC__\n"
  "\n"
  "/*\n"
  " * Utility macros\n"
  " */\n"
  "#define CHECKFLOATVAL(kerror, result, inf_is_valid, zero_is_valid)\t\\\n"
  "\tdo {\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\tif ((isinf((result).value) && !(inf_is_valid)) ||\t\t\t\\\n"
  "\t\t\t((result).value == 0.0 && !(zero_is_valid)))\t\t\t\\\n"
  "\t\t{\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t\t(result).isnull = true;\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t\tSTROM_SET_ERROR((kerror), StromError_CpuReCheck);\t\t\\\n"
  "\t\t}\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t} while(0)\n"
  "\n"
  "#ifndef SAMESIGN\n"
  "#define SAMESIGN(a,b)\t(((a) < 0) == ((b) < 0))\n"
  "#endif\n"
  "\n"
  "/*\n"
  " * Type case functions\n"
  " */\n"
  "STATIC_FUNCTION(pg_bool_t)\n"
  "pgfn_int4_bool(kern_context *kcxt, pg_int4_t arg)\n"
  "{\n"
  "\tpg_bool_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg.isnull;\n"
  "\tresult.value  = (arg.value != 0 ? true : false);\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "/*\n"
  " * Functions for addition operator on basic data types\n"
  " */\n"
  "#define BASIC_INT_ADDFUNC_TEMPLATE(name,r_type,x_type,y_type)\t\t\\\n"
  "\tSTATIC_FUNCTION(pg_##r_type##_t)\t\t\t\t\t\t\t\t\\\n"
  "\tpgfn_##name(kern_context *kcxt,\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t\t\tpg_##x_type##_t arg1, pg_##y_type##_t arg2)\t\t\t\\\n"
  "\t{\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\tpg_##r_type##_t\tresult;\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\tresult.isnull = arg1.isnull | arg2.isnull;\t\t\t\t\t\\\n"
  "\t\tif (!result.isnull)\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t{\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t\tresult.value = arg1.value + arg2.value;\t\t\t\t\t\\\n"
  "\t\t\tif (SAMESIGN(arg1.value, arg2.value) &&\t\t\t\t\t\\\n"
  "\t\t\t\t!SAMESIGN(result.value, arg1.value))\t\t\t\t\\\n"
  "\t\t\t{\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t\t\tresult.isnull = true;\t\t\t\t\t\t\t\t\\\n"
  "\t\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\t\\\n"
  "\t\t\t}\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t}\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\treturn result;\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t}\n"
  "\n"
  "#define BASIC_FLOAT_ADDFUNC_TEMPLATE(name,r_type,x_type,y_type)\t\t\\\n"
  "\tSTATIC_FUNCTION(pg_##r_type##_t)\t\t\t\t\t\t\t\t\\\n"
  "\tpgfn_##name(kern_context *kcxt,\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t\t\tpg_##x_type##_t arg1, pg_##y_type##_t arg2)         \\\n"
  "    {\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\tpg_##r_type##_t\tresult;\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\tresult.isnull = arg1.isnull | arg2.isnull;\t\t\t\t\t\\\n"
  "\t\tif (!result.isnull)\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t{\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t\tresult.value = arg1.value + arg2.value;\t\t\t\t\t\\\n"
  "\t\t\tCHECKFLOATVAL(&kcxt->e, result,\t\t\t\t\t\t\t\\\n"
  "\t\t\t\t\t\t  isinf(arg1.value) ||\t\t\t\t\t\t\\\n"
  "\t\t\t\t\t\t  isinf(arg2.value), true);\t\t\t\t\t\\\n"
  "\t\t}                                                           \\\n"
  "\t\treturn result;\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t}\n"
  "\n"
  "BASIC_INT_ADDFUNC_TEMPLATE(int2pl, int2,int2,int2)\n"
  "BASIC_INT_ADDFUNC_TEMPLATE(int24pl,int4,int2,int4)\n"
  "BASIC_INT_ADDFUNC_TEMPLATE(int28pl,int8,int2,int8)\n"
  "\n"
  "BASIC_INT_ADDFUNC_TEMPLATE(int42pl,int4,int4,int2)\n"
  "BASIC_INT_ADDFUNC_TEMPLATE(int4pl, int4,int4,int4)\n"
  "BASIC_INT_ADDFUNC_TEMPLATE(int48pl,int4,int4,int8)\n"
  "\n"
  "BASIC_INT_ADDFUNC_TEMPLATE(int82pl,int8,int8,int2)\n"
  "BASIC_INT_ADDFUNC_TEMPLATE(int84pl,int8,int8,int4)\n"
  "BASIC_INT_ADDFUNC_TEMPLATE(int8pl, int8,int8,int8)\n"
  "\n"
  "BASIC_FLOAT_ADDFUNC_TEMPLATE(float4pl, float4, float4, float4)\n"
  "BASIC_FLOAT_ADDFUNC_TEMPLATE(float48pl,float8, float4, float8)\n"
  "BASIC_FLOAT_ADDFUNC_TEMPLATE(float84pl,float8, float8, float4)\n"
  "BASIC_FLOAT_ADDFUNC_TEMPLATE(float8pl, float8, float8, float8)\n"
  "\n"
  "#undef BASIC_INT_ADDFUNC_TEMPLATE\n"
  "#undef BASIC_FLOAT_ADDFUNC_TEMPLATE\n"
  "\n"
  "/*\n"
  " * Functions for addition operator on basic data types\n"
  " */\n"
  "#define BASIC_INT_SUBFUNC_TEMPLATE(name,r_type,x_type,y_type)\t\t\\\n"
  "\tSTATIC_FUNCTION(pg_##r_type##_t)\t\t\t\t\t\t\t\t\\\n"
  "\tpgfn_##name(kern_context *kcxt,\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t\t\tpg_##x_type##_t arg1, pg_##y_type##_t arg2)\t\t\t\\\n"
  "\t{\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\tpg_##r_type##_t\tresult;\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\tresult.isnull = arg1.isnull | arg2.isnull;\t\t\t\t\t\\\n"
  "\t\tif (!result.isnull)\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t{\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t\tresult.value = arg1.value - arg2.value;\t\t\t\t\t\\\n"
  "\t\t\tif (!SAMESIGN(arg1.value, arg2.value) &&\t\t\t\t\\\n"
  "\t\t\t\t!SAMESIGN(result.value, arg1.value))\t\t\t\t\\\n"
  "\t\t\t{\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t\t\tresult.isnull = true;\t\t\t\t\t\t\t\t\\\n"
  "\t\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\t\\\n"
  "\t\t\t}\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t}\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\treturn result;\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t}\n"
  "\n"
  "#define BASIC_FLOAT_SUBFUNC_TEMPLATE(name,r_type,x_type,y_type)\t\t\\\n"
  "\tSTATIC_FUNCTION(pg_##r_type##_t)\t\t\t\t\t\t\t\t\\\n"
  "\tpgfn_##name(kern_context *kcxt,\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t\t\tpg_##x_type##_t arg1, pg_##y_type##_t arg2)         \\\n"
  "    {\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\tpg_##r_type##_t\tresult;\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\tresult.isnull = arg1.isnull | arg2.isnull;\t\t\t\t\t\\\n"
  "\t\tif (!result.isnull)\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t{\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t\tresult.value = arg1.value - arg2.value;\t\t\t\t\t\\\n"
  "\t\t\tCHECKFLOATVAL(&kcxt->e, result,\t\t\t\t\t\t\t\\\n"
  "\t\t\t\t\t\t  isinf(arg1.value) ||\t\t\t\t\t\t\\\n"
  "\t\t\t\t\t\t  isinf(arg2.value), true);\t\t\t\t\t\\\n"
  "\t\t}                                                           \\\n"
  "\t\treturn result;\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t}\n"
  "\n"
  "BASIC_INT_SUBFUNC_TEMPLATE(int2mi,  int2, int2, int2)\n"
  "BASIC_INT_SUBFUNC_TEMPLATE(int24mi, int4, int2, int4)\n"
  "BASIC_INT_SUBFUNC_TEMPLATE(int28mi, int8, int2, int8)\n"
  "\n"
  "BASIC_INT_SUBFUNC_TEMPLATE(int42mi, int4, int4, int2)\n"
  "BASIC_INT_SUBFUNC_TEMPLATE(int4mi,  int4, int4, int4)\n"
  "BASIC_INT_SUBFUNC_TEMPLATE(int48mi, int8, int4, int8)\n"
  "\n"
  "BASIC_INT_SUBFUNC_TEMPLATE(int82mi, int8, int8, int2)\n"
  "BASIC_INT_SUBFUNC_TEMPLATE(int84mi, int8, int8, int4)\n"
  "BASIC_INT_SUBFUNC_TEMPLATE(int8mi,  int8, int8, int8)\n"
  "\n"
  "BASIC_FLOAT_SUBFUNC_TEMPLATE(float4mi,  float4, float4, float4)\n"
  "BASIC_FLOAT_SUBFUNC_TEMPLATE(float48mi, float8, float4, float8)\n"
  "BASIC_FLOAT_SUBFUNC_TEMPLATE(float84mi, float8, float8, float4)\n"
  "BASIC_FLOAT_SUBFUNC_TEMPLATE(float8mi,  float8, float8, float8)\n"
  "\n"
  "#undef BASIC_INT_SUBFUNC_TEMPLATE\n"
  "#undef BASIC_FLOAT_SUBFUNC_TEMPLATE\n"
  "\n"
  "\n"
  "/*\n"
  " * Functions for multiplication operator on basic data types\n"
  " */\n"
  "STATIC_FUNCTION(pg_int2_t)\n"
  "pgfn_int2mul(kern_context *kcxt, pg_int2_t arg1, pg_int2_t arg2)\n"
  "{\n"
  "\tpg_int2_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "\tif (!result.isnull)\n"
  "\t{\n"
  "\t\tcl_int\ttemp = (cl_int)arg1.value * (cl_int)arg2.value;\n"
  "\n"
  "\t\tif (temp < SHRT_MIN || temp > SHRT_MAX)\n"
  "\t\t{\n"
  "\t\t\tresult.isnull = true;\n"
  "\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t}\n"
  "\t\telse\n"
  "\t\t\tresult.value = (cl_short) temp;\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_int4_t)\n"
  "pgfn_int24mul(kern_context *kcxt, pg_int2_t arg1, pg_int4_t arg2)\n"
  "{\n"
  "\tpg_int4_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "\tif (!result.isnull)\n"
  "\t{\n"
  "\t\tresult.value = (cl_int)arg1.value * arg2.value;\n"
  "\t\t/* logic copied from int24mul() */\n"
  "\t\tif (!(arg2.value >= (cl_int) SHRT_MIN &&\n"
  "\t\t\t  arg2.value <= (cl_int) SHRT_MAX) &&\n"
  "\t\t\tresult.value / arg2.value != arg1.value)\n"
  "\t\t{\n"
  "\t\t\tresult.isnull = true;\n"
  "\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t}\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_int8_t)\n"
  "pgfn_int28mul(kern_context *kcxt, pg_int2_t arg1, pg_int8_t arg2)\n"
  "{\n"
  "\tpg_int8_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "\tif (!result.isnull)\n"
  "\t{\n"
  "\t\tresult.value = (cl_long)arg1.value * arg2.value;\n"
  "\t\t/* logic copied from int28mul() */\n"
  "\t\tif (arg2.value != (cl_long)((cl_int) arg2.value) &&\n"
  "\t\t\tresult.value / arg2.value != arg1.value)\n"
  "\t\t{\n"
  "\t\t\tresult.isnull = true;\n"
  "\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t}\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_int4_t)\n"
  "pgfn_int42mul(kern_context *kcxt, pg_int4_t arg1, pg_int2_t arg2)\n"
  "{\n"
  "\tpg_int4_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "\tif (!result.isnull)\n"
  "\t{\n"
  "\t\tresult.value = arg1.value * (cl_int)arg2.value;\n"
  "\t\t/* logic copied from int42mul() */\n"
  "\t\tif (!(arg1.value >= (cl_int)SHRT_MIN &&\n"
  "\t\t\t  arg1.value <= (cl_int) SHRT_MAX) &&\n"
  "\t\t\tresult.value / arg1.value != arg2.value)\n"
  "\t\t{\n"
  "\t\t\tresult.isnull = true;\n"
  "\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t}\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_int4_t)\n"
  "pgfn_int4mul(kern_context *kcxt, pg_int4_t arg1, pg_int4_t arg2)\n"
  "{\n"
  "\tpg_int4_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "\tif (!result.isnull)\n"
  "\t{\n"
  "\t\tresult.value = arg1.value * arg2.value;\n"
  "\t\t/* logic copied from int4mul() */\n"
  "\t\tif (!(arg1.value >= (cl_int) SHRT_MIN &&\n"
  "\t\t\t  arg1.value <= (cl_int) SHRT_MAX &&\n"
  "\t\t\t  arg2.value >= (cl_int) SHRT_MIN &&\n"
  "\t\t\t  arg2.value <= (cl_int) SHRT_MAX) &&\n"
  "\t\t\targ2.value != 0 &&\n"
  "\t\t\t((arg2.value == -1 && arg1.value < 0 && result.value < 0) ||\n"
  "\t\t\t result.value / arg2.value != arg1.value))\n"
  "\t\t{\n"
  "\t\t\tresult.isnull = true;\n"
  "\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t}\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_int8_t)\n"
  "pgfn_int48mul(kern_context *kcxt, pg_int4_t arg1, pg_int8_t arg2)\n"
  "{\n"
  "\tpg_int8_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "\tif (!result.isnull)\n"
  "\t{\n"
  "\t\tresult.value = arg1.value * arg2.value;\n"
  "\t\t/* logic copied from int48mul() */\n"
  "\t\tif (arg2.value != (cl_long) ((cl_int) arg2.value) &&\n"
  "\t\t\tresult.value / arg2.value != arg1.value)\n"
  "\t\t{\n"
  "\t\t\tresult.isnull = true;\n"
  "\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t}\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_int8_t)\n"
  "pgfn_int82mul(kern_context *kcxt, pg_int8_t arg1, pg_int2_t arg2)\n"
  "{\n"
  "\tpg_int8_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "\tif (!result.isnull)\n"
  "\t{\n"
  "\t\tresult.value = arg1.value * (cl_long) arg2.value;\n"
  "\t\t/* logic copied from int82mul() */\n"
  "\t\tif (arg1.value != (cl_long) ((cl_int) arg1.value) &&\n"
  "\t\t\tresult.value / arg1.value != arg2.value)\n"
  "\t\t{\n"
  "\t\t\tresult.isnull = true;\n"
  "\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t}\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_int8_t)\n"
  "pgfn_int84mul(kern_context *kcxt, pg_int8_t arg1, pg_int4_t arg2)\n"
  "{\n"
  "\tpg_int8_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "\tif (!result.isnull)\n"
  "\t{\n"
  "\t\tresult.value = arg1.value * (cl_long) arg2.value;\n"
  "\t\t/* logic copied from int84mul() */\n"
  "\t\tif (arg1.value != (cl_long) ((cl_int) arg1.value) &&\n"
  "\t\t\tresult.value / arg1.value != arg2.value)\n"
  "\t\t{\n"
  "\t\t\tresult.isnull = true;\n"
  "\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t}\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_int8_t)\n"
  "pgfn_int8mul(kern_context *kcxt, pg_int8_t arg1, pg_int8_t arg2)\n"
  "{\n"
  "\tpg_int8_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "\tif (!result.isnull)\n"
  "\t{\n"
  "\t\tresult.value = arg1.value * arg2.value;\n"
  "\t\t/* logic copied from int8mul() */\n"
  "\t\tif ((arg1.value != (cl_long) ((cl_int) arg1.value) ||\n"
  "\t\t\t arg2.value != (cl_long) ((cl_int) arg2.value)) &&\n"
  "\t\t\t(arg2.value != 0 &&\n"
  "\t\t\t ((arg2.value == -1 && arg1.value < 0 && result.value < 0) ||\n"
  "\t\t\t  result.value / arg2.value != arg1.value)))\n"
  "\t\t{\n"
  "\t\t\tresult.isnull = true;\n"
  "\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t}\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_float4_t)\n"
  "pgfn_float4mul(kern_context *kcxt, pg_float4_t arg1, pg_float4_t arg2)\n"
  "{\n"
  "\tpg_float4_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "\tif (!result.isnull)\n"
  "\t{\n"
  "\t\tresult.value = arg1.value * arg2.value;\n"
  "\t\tCHECKFLOATVAL(&kcxt->e, result,\n"
  "\t\t\t\t\t  isinf(arg1.value) || isinf(arg2.value),\n"
  "\t\t\t\t\t  arg1.value == 0.0 || arg2.value == 0.0);\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_float8_t)\n"
  "pgfn_float48mul(kern_context *kcxt, pg_float4_t arg1, pg_float8_t arg2)\n"
  "{\n"
  "\tpg_float8_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "\tif (!result.isnull)\n"
  "\t{\n"
  "\t\tresult.value = arg1.value * arg2.value;\n"
  "\t\tCHECKFLOATVAL(&kcxt->e, result,\n"
  "\t\t\t\t\t  isinf(arg1.value) || isinf(arg2.value),\n"
  "\t\t\t\t\t  arg1.value == 0.0 || arg2.value == 0.0);\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_float8_t)\n"
  "pgfn_float84mul(kern_context *kcxt, pg_float8_t arg1, pg_float4_t arg2)\n"
  "{\n"
  "\tpg_float8_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "\tif (!result.isnull)\n"
  "\t{\n"
  "\t\tresult.value = arg1.value * arg2.value;\n"
  "\t\tCHECKFLOATVAL(&kcxt->e, result,\n"
  "\t\t\t\t\t  isinf(arg1.value) || isinf(arg2.value),\n"
  "\t\t\t\t\t  arg1.value == 0.0 || arg2.value == 0.0);\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_float8_t)\n"
  "pgfn_float8mul(kern_context *kcxt, pg_float8_t arg1, pg_float8_t arg2)\n"
  "{\n"
  "\tpg_float8_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "\tif (!result.isnull)\n"
  "\t{\n"
  "\t\tresult.value = arg1.value * arg2.value;\n"
  "\t\tCHECKFLOATVAL(&kcxt->e, result,\n"
  "\t\t\t\t\t  isinf(arg1.value) || isinf(arg2.value),\n"
  "\t\t\t\t\t  arg1.value == 0.0 || arg2.value == 0.0);\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "/*\n"
  " * Functions for division operator on basic data types\n"
  " */\n"
  "#define SAMESIGN(a,b)\t(((a) < 0) == ((b) < 0))\n"
  "\n"
  "STATIC_FUNCTION(pg_int2_t)\n"
  "pgfn_int2div(kern_context *kcxt, pg_int2_t arg1, pg_int2_t arg2)\n"
  "{\n"
  "\tpg_int2_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "    if (!result.isnull)\n"
  "\t{\n"
  "\t\tif (arg2.value == 0)\n"
  "\t\t{\n"
  "\t\t\tresult.isnull = true;\n"
  "\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t}\n"
  "\t\telse if (arg2.value == -1)\n"
  "\t\t{\n"
  "\t\t\tresult.value = -arg1.value;\n"
  "\t\t\tif (arg1.value != 0 && SAMESIGN(result.value, arg1.value))\n"
  "\t\t\t{\n"
  "\t\t\t\tresult.isnull = true;\n"
  "\t\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t\t}\n"
  "\t\t}\n"
  "\t\telse\n"
  "\t\t\tresult.value = arg1.value / arg2.value;\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_int4_t)\n"
  "pgfn_int24div(kern_context *kcxt, pg_int2_t arg1, pg_int4_t arg2)\n"
  "{\n"
  "\tpg_int4_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "    if (!result.isnull)\n"
  "\t{\n"
  "\t\tif (arg2.value == 0)\n"
  "\t\t{\n"
  "\t\t\tresult.isnull = true;\n"
  "            STROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t}\n"
  "\t\telse\n"
  "\t\t\tresult.value = (cl_int) arg1.value / arg2.value;\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_int8_t)\n"
  "pgfn_int28div(kern_context *kcxt, pg_int2_t arg1, pg_int8_t arg2)\n"
  "{\n"
  "\tpg_int8_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "    if (!result.isnull)\n"
  "\t{\n"
  "\t\tif (arg2.value == 0)\n"
  "\t\t{\n"
  "\t\t\tresult.isnull = true;\n"
  "            STROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t}\n"
  "\t\telse\n"
  "\t\t\tresult.value = (cl_long) arg1.value / arg2.value;\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_int4_t)\n"
  "pgfn_int42div(kern_context *kcxt, pg_int4_t arg1, pg_int2_t arg2)\n"
  "{\n"
  "\tpg_int4_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "    if (!result.isnull)\n"
  "    {\n"
  "\t\tif (arg2.value == 0)\n"
  "\t\t{\n"
  "\t\t\tresult.isnull = true;\n"
  "\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t}\n"
  "\t\telse if (arg2.value == -1)\n"
  "\t\t{\n"
  "\t\t\tresult.value = -arg1.value;\n"
  "\t\t\tif (arg1.value != 0 && SAMESIGN(result.value, arg1.value))\n"
  "\t\t\t{\n"
  "\t\t\t\tresult.isnull = true;\n"
  "\t\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t\t}\n"
  "\t\t}\n"
  "\t\telse\n"
  "\t\t\tresult.value = arg1.value / arg2.value;\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_int4_t)\n"
  "pgfn_int4div(kern_context *kcxt, pg_int4_t arg1, pg_int4_t arg2)\n"
  "{\n"
  "\tpg_int4_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "    if (!result.isnull)\n"
  "\t{\n"
  "\t\tif (arg2.value == 0)\n"
  "\t\t{\n"
  "\t\t\tresult.isnull = true;\n"
  "\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t}\n"
  "\t\telse if (arg2.value == -1)\n"
  "\t\t{\n"
  "\t\t\tresult.value = -arg1.value;\n"
  "\t\t\tif (arg1.value != 0 && SAMESIGN(result.value, arg1.value))\n"
  "\t\t\t{\n"
  "\t\t\t\tresult.isnull = true;\n"
  "\t\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t\t}\n"
  "\t\t}\n"
  "\t\telse\n"
  "\t\t\tresult.value = arg1.value / arg2.value;\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_int8_t)\n"
  "pgfn_int48div(kern_context *kcxt, pg_int4_t arg1, pg_int8_t arg2)\n"
  "{\n"
  "\tpg_int8_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "    if (!result.isnull)\n"
  "\t{\n"
  "\t\tif (arg2.value == 0)\n"
  "\t\t{\n"
  "\t\t\tresult.isnull = true;\n"
  "\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t}\n"
  "\t\telse\n"
  "\t\t\tresult.value = (cl_long) arg1.value / arg2.value;\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_int8_t)\n"
  "pgfn_int82div(kern_context *kcxt, pg_int8_t arg1, pg_int2_t arg2)\n"
  "{\n"
  "\tpg_int8_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "\tif (!result.isnull)\n"
  "\t{\n"
  "\t\tif (arg2.value == 0)\n"
  "\t\t{\n"
  "\t\t\tresult.isnull = true;\n"
  "\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t}\n"
  "\t\telse if (arg2.value == -1)\n"
  "\t\t{\n"
  "\t\t\tresult.value = -arg1.value;\n"
  "\t\t\tif (arg1.value != 0 && SAMESIGN(result.value, arg1.value))\n"
  "\t\t\t{\n"
  "\t\t\t\tresult.isnull = true;\n"
  "\t\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t\t}\n"
  "\t\t}\n"
  "\t\telse\n"
  "\t\t\tresult.value = arg1.value / arg2.value;\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_int8_t)\n"
  "pgfn_int84div(kern_context *kcxt, pg_int8_t arg1, pg_int4_t arg2)\n"
  "{\n"
  "\tpg_int8_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "\tif (!result.isnull)\n"
  "\t{\n"
  "\t\tif (arg2.value == 0)\n"
  "\t\t{\n"
  "\t\t\tresult.isnull = true;\n"
  "\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t}\n"
  "\t\telse if (arg2.value == -1)\n"
  "\t\t{\n"
  "\t\t\tresult.value = -arg1.value;\n"
  "\t\t\tif (arg1.value != 0 && SAMESIGN(result.value, arg1.value))\n"
  "\t\t\t{\n"
  "\t\t\t\tresult.isnull = true;\n"
  "\t\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t\t}\n"
  "\t\t}\n"
  "\t\telse\n"
  "\t\t\tresult.value = arg1.value / arg2.value;\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_int8_t)\n"
  "pgfn_int8div(kern_context *kcxt, pg_int8_t arg1, pg_int8_t arg2)\n"
  "{\n"
  "\tpg_int8_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "\tif (!result.isnull)\n"
  "\t{\n"
  "\t\tif (arg2.value == 0)\n"
  "\t\t{\n"
  "\t\t\tresult.isnull = true;\n"
  "\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t}\n"
  "\t\telse if (arg2.value == -1)\n"
  "\t\t{\n"
  "\t\t\tresult.value = -arg1.value;\n"
  "\t\t\tif (arg1.value != 0 && SAMESIGN(result.value, arg1.value))\n"
  "\t\t\t{\n"
  "\t\t\t\tresult.isnull = true;\n"
  "\t\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t\t}\n"
  "\t\t}\n"
  "\t\telse\n"
  "\t\t\tresult.value = arg1.value / arg2.value;\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_float4_t)\n"
  "pgfn_float4div(kern_context *kcxt, pg_float4_t arg1, pg_float4_t arg2)\n"
  "{\n"
  "\tpg_float4_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "    if (!result.isnull)\n"
  "    {\n"
  "\t\tif (arg2.value == 0.0)\n"
  "\t\t{\n"
  "\t\t\tresult.isnull = true;\n"
  "\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t}\n"
  "\t\telse\n"
  "\t\t{\n"
  "\t\t\tresult.value = arg1.value / arg2.value;\n"
  "\t\t\tCHECKFLOATVAL(&kcxt->e, result,\n"
  "\t\t\t\t\t\t  isinf(arg1.value) || isinf(arg2.value),\n"
  "\t\t\t\t\t\t  arg1.value == 0.0);\n"
  "\t\t}\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_float8_t)\n"
  "pgfn_float48div(kern_context *kcxt, pg_float4_t arg1, pg_float8_t arg2)\n"
  "{\n"
  "\tpg_float8_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "    if (!result.isnull)\n"
  "    {\n"
  "\t\tif (arg2.value == 0.0)\n"
  "\t\t{\n"
  "\t\t\tresult.isnull = true;\n"
  "\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t}\n"
  "\t\telse\n"
  "\t\t{\n"
  "\t\t\tresult.value = arg1.value / arg2.value;\n"
  "\t\t\tCHECKFLOATVAL(&kcxt->e, result,\n"
  "\t\t\t\t\t\t  isinf(arg1.value) || isinf(arg2.value),\n"
  "\t\t\t\t\t\t  arg1.value == 0.0);\n"
  "\t\t}\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_float8_t)\n"
  "pgfn_float84div(kern_context *kcxt, pg_float8_t arg1, pg_float4_t arg2)\n"
  "{\n"
  "\tpg_float8_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "    if (!result.isnull)\n"
  "    {\n"
  "\t\tif (arg2.value == 0.0)\n"
  "\t\t{\n"
  "\t\t\tresult.isnull = true;\n"
  "\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t}\n"
  "\t\telse\n"
  "\t\t{\n"
  "\t\t\tresult.value = arg1.value / arg2.value;\n"
  "\t\t\tCHECKFLOATVAL(&kcxt->e, result,\n"
  "\t\t\t\t\t\t  isinf(arg1.value) || isinf(arg2.value),\n"
  "\t\t\t\t\t\t  arg1.value == 0.0);\n"
  "\t\t}\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_float8_t)\n"
  "pgfn_float8div(kern_context *kcxt, pg_float8_t arg1, pg_float8_t arg2)\n"
  "{\n"
  "\tpg_float8_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull | arg2.isnull;\n"
  "    if (!result.isnull)\n"
  "    {\n"
  "\t\tif (arg2.value == 0.0)\n"
  "\t\t{\n"
  "\t\t\tresult.isnull = true;\n"
  "\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t}\n"
  "\t\telse\n"
  "\t\t{\n"
  "\t\t\tresult.value = arg1.value / arg2.value;\n"
  "\t\t\tCHECKFLOATVAL(&kcxt->e, result,\n"
  "\t\t\t\t\t\t  isinf(arg1.value) || isinf(arg2.value),\n"
  "\t\t\t\t\t\t  arg1.value == 0.0);\n"
  "\t\t}\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "/*\n"
  " * Functions for modulo operator on basic data types\n"
  " */\n"
  "#define BASIC_INT_MODFUNC_TEMPLATE(name,d_type)\t\t\t\t\t\t\\\n"
  "\tSTATIC_FUNCTION(pg_##d_type##_t)\t\t\t\t\t\t\t\t\\\n"
  "\tpgfn_##name(kern_context *kcxt,\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t\t\tpg_##d_type##_t arg1, pg_##d_type##_t arg2)\t\t\t\\\n"
  "\t{\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\tpg_##d_type##_t\tresult;\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\tresult.isnull = arg1.isnull | arg2.isnull;\t\t\t\t\t\\\n"
  "\t\tif (!result.isnull)\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t{\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t\tif (arg2.value == 0)\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t\t{\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t\t\tresult.isnull = true;\t\t\t\t\t\t\t\t\\\n"
  "\t\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\t\\\n"
  "\t\t\t}\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t\telse if (arg2.value == -1)\t\t\t\t\t\t\t\t\\\n"
  "\t\t\t\tresult.value = 0;\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t\telse\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\t\t\tresult.value = arg1.value % arg2.value;\t\t\t\t\\\n"
  "\t\t}\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t\treturn result;\t\t\t\t\t\t\t\t\t\t\t\t\\\n"
  "\t}\n"
  "\n"
  "BASIC_INT_MODFUNC_TEMPLATE(int2mod, int2)\n"
  "BASIC_INT_MODFUNC_TEMPLATE(int4mod, int4)\n"
  "BASIC_INT_MODFUNC_TEMPLATE(int8mod, int8)\n"
  "\n"
  "#undef BASIC_INT_MODFUNC_TEMPLATE\n"
  "\n"
  "/*\n"
  " * Misc mathematic functions\n"
  " */\n"
  "STATIC_FUNCTION(pg_float8_t)\n"
  "pgfn_dsqrt(kern_context *kcxt, pg_float8_t arg1)\n"
  "{\n"
  "\tpg_float8_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull;\n"
  "\tif (!arg1.isnull)\n"
  "\t{\n"
  "\t\tif (arg1.value < 0.0)\n"
  "\t\t{\n"
  "\t\t\tresult.isnull = true;\n"
  "\t\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t\t}\n"
  "\t\telse\n"
  "\t\t{\n"
  "\t\t\tresult.value = sqrt(arg1.value);\n"
  "\t\t\tCHECKFLOATVAL(&kcxt->e, result,\n"
  "\t\t\t\t\t\t  isinf(arg1.value), arg1.value == 0);\n"
  "\t\t}\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_float8_t)\n"
  "pgfn_dpow(kern_context *kcxt, pg_float8_t arg1, pg_float8_t arg2)\n"
  "{\n"
  "\tpg_float8_t\tresult;\n"
  "\n"
  "\tif ((arg1.value == 0.0 && arg2.value < 0.0) ||\n"
  "\t\t(arg1.value < 0.0 && floor(arg2.value) != arg2.value))\n"
  "\t{\n"
  "\t\tresult.isnull = true;\n"
  "\t\tSTROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);\n"
  "\t}\n"
  "\telse\n"
  "\t{\n"
  "\t\tresult.isnull = false;\n"
  "\t\tresult.value = pow(arg1.value, arg2.value);\n"
  "\t}\n"
  "\t// TODO: needs to investigate which value shall be returned when\n"
  "\t// NVIDIA platform accept very large number\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_float8_t)\n"
  "pgfn_dpi(kern_context *kcxt)\n"
  "{\n"
  "\tpg_float8_t\tresult;\n"
  "\n"
  "\tresult.isnull = false;\n"
  "\tresult.value = 3.141592653589793115998;\n"
  "\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "STATIC_FUNCTION(pg_float8_t)\n"
  "pgfn_dcot(kern_context *kcxt, pg_float8_t arg1)\n"
  "{\n"
  "\tpg_float8_t\tresult;\n"
  "\n"
  "\tresult.isnull = arg1.isnull;\n"
  "\tif (!arg1.isnull)\n"
  "\t{\n"
  "\t\t/* tan(x) cause error, EDOM, if input value is infinity */\n"
  "\t\tCHECKFLOATVAL(&kcxt->e, arg1, false, true);\n"
  "\n"
  "\t\tresult.value = 1.0 / tan(arg1.value);\n"
  "\n"
  "\t\tCHECKFLOATVAL(&kcxt->e, result, true /* cot(pi/2) == inf */, true);\n"
  "\t}\n"
  "\treturn result;\n"
  "}\n"
  "\n"
  "#endif\t/* __CUDACC__ */\n"
  "#endif\t/* CUDA_MATH_H */\n"
;