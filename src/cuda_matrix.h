/*
 * cuda_matrix.h
 *
 * collection of matrix/vector/array support routines for CUDA devices
 * --
 * Copyright 2011-2016 (C) KaiGai Kohei <kaigai@kaigai.gr.jp>
 * Copyright 2014-2016 (C) The PG-Strom Development Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef CUDA_MATRIX_H
#define CUDA_MATRIX_H
#ifdef __CUDACC__

/* ------------------------------------------------------------------
 *
 * Support macros for PostgreSQL's array structure
 *
 * ------------------------------------------------------------------ */

typedef struct
{
	/*
	 * NOTE: We assume 4bytes varlena header for array type. It allows
	 * aligned references to the array elements. Unlike CPU side, we
	 * cannot have extra malloc to ensure 4bytes varlena header. It is
	 * the reason why our ScalarArrayOp implementation does not support
	 * array data type referenced by Var node; which is potentially has
	 * short format.
	 */
	cl_uint		vl_len_;		/* don't touch this field */
	cl_int		ndim;			/* # of dimensions */
	cl_int		dataoffset;		/* offset to data, or 0 if no bitmap */
	cl_uint		elemtype;		/* element type OID */
} ArrayType;

#define MAXDIM			6

#define ARR_SIZE(a)		VARSIZE_ANY(a)
#define ARR_NDIM(a)		(((ArrayType *)(a))->ndim)
#define ARR_HASNULL(a)	(((ArrayType *)(a))->dataoffset != 0)
#define ARR_ELEMTYPE(a)	(((ArrayType *)(a))->elemtype)
#define ARR_DIMS(a)									\
	((int *) (((char *) (a)) + sizeof(ArrayType)))
#define ARR_LBOUND(a)								\
	((int *) (((char *) (a)) + sizeof(ArrayType) +	\
			  sizeof(int) * ARR_NDIM(a)))
#define ARR_NULLBITMAP(a)							\
	(ARR_HASNULL(a)									\
	 ? (((char *) (a)) + sizeof(ArrayType) +		\
		2 * sizeof(int) * ARR_NDIM(a))				\
	 : (char *) NULL)
/*
 * The total array header size (in bytes) for an array with the specified
 * number of dimensions and total number of items.
 */
#define ARR_OVERHEAD_NONULLS(ndims)					\
	MAXALIGN(sizeof(ArrayType) + 2 * sizeof(int) * (ndims))
#define ARR_OVERHEAD_WITHNULLS(ndims, nitems)		\
	MAXALIGN(sizeof(ArrayType) + 2 * sizeof(int) * (ndims) +	\
			 ((nitems) + 7) / 8)
/*
 * Returns a pointer to the actual array data.
 */
#define ARR_DATA_OFFSET(a)					\
	(ARR_HASNULL(a)							\
	 ? ((ArrayType *)(a))->dataoffset		\
	 : ARR_OVERHEAD_NONULLS(ARR_NDIM(a)))

#define ARR_DATA_PTR(a)		(((char *) (a)) + ARR_DATA_OFFSET(a))

#ifndef PG_ARRAY_TYPE_DEFINED
#define PG_ARRAY_TYPE_DEFINED
STROMCL_VARLENA_TYPE_TEMPLATE(array)
#endif

STATIC_INLINE(cl_int)
ArrayGetNItems(kern_context *kcxt, cl_int ndim, const cl_int *dims)
{
	cl_int		i, ret;
	cl_long		prod;

	if (ndim <= 0)
		return 0;

	ret = 1;
	for (i=0; i < ndim; i++)
	{

		if (dims[i] < 0)
		{
			/* negative dimension implies an error... */
			STROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);
			return 0;
		}
		prod = (cl_long) ret * (cl_long) dims[i];
		ret = (cl_int) prod;
		if ((cl_long) ret != prod)
		{
			/* array size exceeds the maximum allowed... */
			STROM_SET_ERROR(&kcxt->e, StromError_CpuReCheck);
			return 0;
		}
	}
	assert(ret >= 0);

	return ret;
}

/* ----------------------------------------------------------------
 *
 * MATRIX data type support
 *
 * ---------------------------------------------------------------- */

#ifndef PG_MATRIX_TYPE_DEFINED
#define PG_MATRIX_TYPE_DEFINED
STROMCL_VARLENA_TYPE_TEMPLATE(matrix)
#endif

#endif	/* __CUDACC__ */

typedef struct
{
	cl_uint		__vl_len;	/* varlena header (only 4B header) */
	cl_int		ndim;		/* always 2 for matrix */
	cl_int		dataoffset;	/* always 0 for matrix */
	cl_uint		elemtype;	/* always FLOAT4OID for matrix */
	union {
		struct {
			cl_int	width;		/* height of the matrix (=dim1) */
			cl_int	height;		/* width of the matrix (=dim2) */
			cl_int	lbound1;	/* always 1 for matrix */
			cl_int	lbound2;	/* always 1 for matrix */
			char	values[1];	/* to be variable length */
		} d2;
		struct {
			cl_int	height;		/* height of the vector */
			cl_int	lbound1;	/* always 1 for vector */
			char	values[1];	/* to be variable length */
		} d1;
	} u;
} MatrixType;

STATIC_INLINE(cl_bool)
VALIDATE_ARRAY_MATRIX(MatrixType *matrix)
{
	if (!VARATT_IS_4B(matrix))
		return false;
	if (matrix->dataoffset == 0 &&
#ifdef __CUDACC__
		(matrix->elemtype == PG_INT2OID ||
		 matrix->elemtype == PG_INT4OID ||
		 matrix->elemtype == PG_INT8OID ||
		 matrix->elemtype == PG_FLOAT4OID ||
		 matrix->elemtype == PG_FLOAT8OID)
#else	/* __CUDACC__ */
		(matrix->elemtype == INT2OID ||
		 matrix->elemtype == INT4OID ||
		 matrix->elemtype == INT8OID ||
		 matrix->elemtype == FLOAT4OID ||
		 matrix->elemtype == FLOAT8OID)
#endif	/* __CUDACC__ */
		)
	{
		if (matrix->ndim == 2)
		{
			if (matrix->u.d2.width > 0 &&
				matrix->u.d2.height > 0 &&
				matrix->u.d2.lbound1 == 1 &&
				matrix->u.d2.lbound2 == 1)
				return true;
		}
		else if (matrix->ndim == 1)
		{
			if (matrix->u.d1.height > 0 &&
				matrix->u.d1.lbound1 == 1)
				return true;
		}
		else
			return false;
	}
	return false;
}

#define ARRAY_MATRIX_ELEMTYPE(X)			\
	(((MatrixType *)(X))->elemtype)
#define ARRAY_MATRIX_HEIGHT(X)											\
	(((MatrixType *)(X))->ndim == 2 ? ((MatrixType *)(X))->u.d2.height : \
	 ((MatrixType *)(X))->ndim == 1 ? ((MatrixType *)(X))->u.d1.height : -1)
#define ARRAY_MATRIX_WIDTH(X)											\
	(((MatrixType *)(X))->ndim == 2 ? ((MatrixType *)(X))->u.d2.width :	\
	 ((MatrixType *)(X))->ndim == 1 ? 1 : -1)
#define ARRAY_MATRIX_DATAPTR(X)											\
	(((MatrixType *)(X))->ndim == 2 ? ((MatrixType *)(X))->u.d2.values : \
	 ((MatrixType *)(X))->ndim == 1 ? ((MatrixType *)(X))->u.d1.values : NULL)
#define ARRAY_MATRIX_RAWSIZE(typlen,height,width)		\
	offsetof(MatrixType, u.d2.values[(size_t)(typlen) *	\
									 (size_t)(height) *	\
									 (size_t)(width)])
#define ARRAY_VECTOR_RAWSIZE(typlen,nitems)				\
	offsetof(MatrixType, u.d1.values[(size_t)(typlen) *	\
									 (size_t)(nitems)])

#define INIT_ARRAY_VECTOR(X,_elemtype,_typlen,_nitems)			\
	do {														\
		size_t	__len = ARRAY_VECTOR_RAWSIZE(_typlen,_nitems);	\
																\
		SET_VARSIZE(X, __len);									\
		((MatrixType *)(X))->ndim = 1;							\
		((MatrixType *)(X))->dataoffset = 0;					\
		((MatrixType *)(X))->elemtype = (_elemtype);			\
		((MatrixType *)(X))->u.d1.height = (_nitems);			\
		((MatrixType *)(X))->u.d1.lbound1 = 1;					\
	} while(0)

#define INIT_ARRAY_MATRIX(X,_elemtype,_typlen,_height,_width)	\
	do {														\
		size_t	__len = ARRAY_MATRIX_RAWSIZE((_typlen),			\
											 (_height),			\
											 (_width));			\
		SET_VARSIZE(X, __len);									\
		((MatrixType *)(X))->ndim = 2;							\
		((MatrixType *)(X))->dataoffset = 0;					\
		((MatrixType *)(X))->elemtype = (_elemtype);			\
		((MatrixType *)(X))->u.d2.height = (_height);			\
		((MatrixType *)(X))->u.d2.width = (_width);				\
		((MatrixType *)(X))->u.d2.lbound1 = 1;					\
		((MatrixType *)(X))->u.d2.lbound2 = 1;					\
	} while(0)

#ifdef __CUDACC__

/* ----------------------------------------------------------------
 *
 * Bitonic Sorting Support for PL/CUDA functions
 *
 * pgstromMatrixSort(FP32|FP64)(cl_uint    *row_index,		:out
 *                              MatrixType *matrix,			:in
 *                              cl_uint    *sort_keys,		:in
 *                              cl_uint     num_keys,		:in
 *                              cl_bool     is_descending)	:in
 *
 * This service routine supports to sort the supplied matrix according
 * to the key values specified by the 'sort_keys' and 'num_keys'.
 * Its sorting results shall be saved at the 'row_index'. It assumes
 * that 'row_index' is initialized to point a particular row of the
 * matrix uniquely, and any of row_index[] is less than the height of
 * matrix. (usually, it shall be initialized by sequential number)
 * The 'sort_keys' is an array of column index of the supplied matrix.
 * Its range is between 0 and (width of the matrix - 1).
 *
 * ----------------------------------------------------------------
 */
#define PGSTROM_MATRIX_SORT_KEYCOMP_TEMPLATE(SUFFIX,BASETYPE)			\
	STATIC_INLINE(cl_int)												\
	matrix_sort_keycomp_##SUFFIX(BASETYPE *data_ptr,					\
								 cl_uint	height,						\
								 cl_uint	width,						\
								 cl_uint   *sortkeys,					\
								 cl_uint	num_keys,					\
								 cl_uint	x_index,					\
								 cl_uint	y_index)					\
	{																	\
		cl_uint		i, j;												\
		BASETYPE	x_value;											\
		BASETYPE	y_value;											\
																		\
		for (i=0; i < num_keys; i++)									\
		{																\
			j = sortkeys[i];											\
			assert(j < width);											\
																		\
			x_value = data_ptr[j * height + x_index];					\
			y_value = data_ptr[j * height + y_index];					\
																		\
			if (x_value < y_value)										\
				return -1;												\
			else if (x_value > y_value)									\
				return 1;												\
		}																\
		return 0;	/* both rows are equivalent */						\
	}

#define PGSTROM_MATRIX_SORT_LOCAL_TEMPLATE(SUFFIX,BASETYPE)				\
	KERNEL_FUNCTION_MAXTHREADS(void)									\
	matrix_sort_local_##SUFFIX(cl_uint	   *row_index,					\
							   BASETYPE	   *data_ptr,					\
							   kern_arg_t	height,						\
							   kern_arg_t	width,						\
							   kern_arg_t   valid_nrows,				\
							   cl_uint	   *sortkeys,					\
							   kern_arg_t	num_keys,					\
							   kern_arg_t	__direction)				\
	{																	\
		cl_int		direction = __direction;							\
		cl_uint	   *localIdx = SHARED_WORKMEM(cl_uint);					\
		cl_uint		localLimit;											\
		cl_uint		partSize = 2 * get_local_size();					\
		cl_uint		partBase = get_global_index() * partSize;			\
		cl_uint		blockSize;											\
		cl_uint		unitSize;											\
		cl_uint		i;													\
																		\
		/* Load index to localIdx[] */									\
		assert(valid_nrows <= height);									\
		localLimit = (partBase + partSize <= valid_nrows				\
					  ? partSize										\
					  : valid_nrows - partBase);						\
		for (i = get_local_id();										\
			 i < localLimit;											\
			 i += get_local_size())										\
			localIdx[i] = row_index[partBase + i];						\
		__syncthreads();												\
																		\
		for (blockSize = 2;	blockSize <= partSize; blockSize *= 2)		\
		{																\
			for (unitSize = blockSize; unitSize >= 2; unitSize /= 2)	\
			{															\
				cl_uint		unitMask		= (unitSize - 1);			\
				cl_uint		halfUnitSize	= (unitSize >> 1);			\
				cl_uint		halfUnitMask	= (halfUnitSize - 1);		\
				cl_uint		idx0, idx1;									\
																		\
				idx0 = (((get_local_id() & ~halfUnitMask) << 1) +		\
						(get_local_id() & halfUnitMask));				\
				idx1 = (unitSize == blockSize							\
						? ((idx0 & ~unitMask) | (~idx0 & unitMask))		\
						: (halfUnitSize + idx0));						\
				if (idx1 < localLimit)									\
				{														\
					cl_uint		pos0 = localIdx[idx0];					\
					cl_uint		pos1 = localIdx[idx1];					\
																		\
					assert(pos0 < height && pos1 < height);				\
																		\
					if (matrix_sort_keycomp_##SUFFIX(data_ptr,			\
													 height,			\
													 width,				\
													 sortkeys,			\
													 num_keys,			\
													 pos0,				\
													 pos1) == direction) \
					{													\
						/* swap */										\
						localIdx[idx0] = pos1;							\
						localIdx[idx1] = pos0;							\
					}													\
				}														\
				__syncthreads();										\
			}															\
		}																\
		/* write back the sorting result of this local block */			\
		for (i = get_local_id();										\
			 i < localLimit;											\
			 i += get_local_size())										\
		{																\
			assert(partBase + i < valid_nrows);							\
			row_index[partBase + i] = localIdx[i];						\
		}																\
		__syncthreads();												\
	}

#define PGSTROM_MATRIX_SORT_STEP_TEMPLATE(SUFFIX,BASETYPE)				\
	KERNEL_FUNCTION_MAXTHREADS(void)									\
	matrix_sort_step_##SUFFIX(cl_uint	   *row_index,					\
							  BASETYPE	   *data_ptr,					\
							  kern_arg_t	height,						\
							  kern_arg_t	width,						\
							  kern_arg_t    valid_nrows,				\
							  cl_uint	   *sortkeys,					\
							  kern_arg_t	num_keys,					\
							  kern_arg_t	__direction,				\
							  kern_arg_t	unitSize,					\
							  kern_arg_t	reversing)					\
	{																	\
		cl_int		direction = __direction;							\
		cl_uint		unitMask = unitSize - 1;							\
		cl_uint		halfUnitSize = unitSize >> 1;						\
		cl_uint		halfUnitMask = halfUnitSize - 1;					\
		cl_uint		idx0, idx1;											\
		cl_uint		pos0, pos1;											\
																		\
		idx0 = (((get_global_id() & ~halfUnitMask) << 1)				\
				+ (get_global_id() & halfUnitMask));					\
		idx1 = (reversing												\
				? ((idx0 & ~unitMask) | (~idx0 & unitMask))				\
				: (idx0 + halfUnitSize));								\
		if (idx1 < valid_nrows)											\
		{																\
			pos0 = row_index[idx0];										\
			pos1 = row_index[idx1];										\
			if (matrix_sort_keycomp_##SUFFIX(data_ptr,					\
											 height,					\
											 width,						\
											 sortkeys,					\
											 num_keys,					\
											 pos0,						\
											 pos1) == direction)		\
			{															\
				/* swap */												\
				row_index[idx0] = pos1;									\
				row_index[idx1] = pos0;									\
			}															\
		}																\
	}

#define PGSTROM_MATRIX_SORT_MERGE_TEMPLATE(SUFFIX,BASETYPE)				\
	KERNEL_FUNCTION_MAXTHREADS(void)									\
	matrix_sort_merge_##SUFFIX(cl_uint	   *row_index,					\
							   BASETYPE	   *data_ptr,					\
							   kern_arg_t	height,						\
							   kern_arg_t	width,						\
							   kern_arg_t   valid_nrows,				\
							   cl_uint	   *sortkeys,					\
							   kern_arg_t	num_keys,					\
							   kern_arg_t	__direction)				\
	{																	\
		cl_int		direction = __direction;							\
		cl_uint	   *localIdx = SHARED_WORKMEM(cl_uint);					\
		cl_uint		localLimit;											\
		cl_uint		partSize = 2 * get_local_size();					\
		cl_uint		partBase = get_global_index() * partSize;			\
		cl_uint		blockSize = partSize;								\
		cl_uint		unitSize;											\
		cl_uint		i;													\
																		\
		/* Load index to localIdx[] */									\
		localLimit = (partBase + partSize <= valid_nrows				\
					  ? partSize										\
					  : valid_nrows - partBase);						\
		for (i = get_local_id();										\
			 i < localLimit;											\
			 i += get_local_size())										\
			localIdx[i] = row_index[partBase + i];						\
		__syncthreads();												\
																		\
		/* merge two sorted blocks */									\
		for (unitSize = blockSize; unitSize >= 2; unitSize >>= 1)		\
		{																\
			cl_uint		halfUnitSize = (unitSize >> 1);					\
			cl_uint		halfUnitMask = (halfUnitSize - 1);				\
			cl_uint		idx0, idx1;										\
																		\
			idx0 = (((get_local_id() & ~halfUnitMask) << 1)				\
					+ (get_local_id() & halfUnitMask));					\
			idx1 = halfUnitSize + idx0;									\
																		\
			if (idx1 < localLimit)										\
			{															\
				cl_uint		pos0 = localIdx[idx0];						\
				cl_uint		pos1 = localIdx[idx1];						\
																		\
				if (matrix_sort_keycomp_##SUFFIX(data_ptr,				\
												 height,				\
												 width,					\
												 sortkeys,				\
												 num_keys,				\
												 pos0,					\
												 pos1) == direction)	\
				{														\
					/* swap */											\
					localIdx[idx0] = pos1;								\
					localIdx[idx1] = pos0;								\
				}														\
			}															\
			__syncthreads();											\
		}																\
		/* update the row_index[] */									\
		for (i = get_local_id();										\
			 i < localLimit;											\
			 i += get_local_size())										\
			row_index[partBase + i] = localIdx[i];						\
		__syncthreads();												\
	}

#define PGSTROM_MATRIX_SORT_TEMPLATE(SUFFIX,BASETYPE,PG_TYPEOID)		\
	PGSTROM_MATRIX_SORT_KEYCOMP_TEMPLATE(SUFFIX,BASETYPE)				\
	PGSTROM_MATRIX_SORT_LOCAL_TEMPLATE(SUFFIX,BASETYPE)					\
	PGSTROM_MATRIX_SORT_STEP_TEMPLATE(SUFFIX,BASETYPE)					\
	PGSTROM_MATRIX_SORT_MERGE_TEMPLATE(SUFFIX,BASETYPE)					\
																		\
	STATIC_FUNCTION(cudaError_t)										\
	pgstromMatrixPartialSort##SUFFIX(cl_uint     *row_index,			\
									 MatrixType  *matrix,				\
									 cl_uint      valid_nrows,			\
									 cl_uint     *sort_keys,			\
									 cl_uint      num_keys,				\
									 cl_bool      is_descending)		\
	{																	\
		cl_uint		height = ARRAY_MATRIX_HEIGHT(matrix);				\
		cl_uint		width = ARRAY_MATRIX_WIDTH(matrix);					\
		dim3		grid_sz;											\
		dim3		block_sz;											\
		cl_uint		min_block_sz = UINT_MAX;							\
		kern_arg_t	kern_argbuf[8];										\
		kern_arg_t *__kern_args;										\
		void	   *kern_funcs[3];										\
		cl_uint	   *__sort_keys;										\
		cl_uint		i, j, nhalf;										\
		cudaError_t	status = cudaSuccess;								\
																		\
		/* sanity checks */												\
		if (ARRAY_MATRIX_ELEMTYPE(matrix) != (PG_TYPEOID))				\
			return cudaErrorInvalidValue;								\
		if (num_keys == 0)												\
			return cudaErrorInvalidValue;								\
																		\
		/* setup common kernel arguments */								\
		__sort_keys = (cl_uint *)malloc(sizeof(cl_uint) * num_keys);	\
		if (!__sort_keys)												\
			return cudaErrorMemoryAllocation;							\
		memcpy(__sort_keys, sort_keys, sizeof(cl_uint) * num_keys);		\
																		\
		kern_argbuf[0] = (kern_arg_t)(row_index);						\
		kern_argbuf[1] = (kern_arg_t)ARRAY_MATRIX_DATAPTR(matrix);		\
		kern_argbuf[2] = (kern_arg_t)(height);							\
		kern_argbuf[3] = (kern_arg_t)(width);							\
		kern_argbuf[4] = (kern_arg_t)(valid_nrows);						\
		kern_argbuf[5] = (kern_arg_t)(__sort_keys);						\
		kern_argbuf[6] = (kern_arg_t)(num_keys);						\
		kern_argbuf[7] = (kern_arg_t)(is_descending ? -1 : 1);			\
																		\
		/*																\
		 * Ensure max available block size for each kernel functions.	\
		 * These are declared with KERNEL_FUNCTION_MAXTHREADS, we		\
		 * expect largest workgroup size is equivalent to H/W limit.	\
		 */																\
		kern_funcs[0] = (void *)matrix_sort_local_##SUFFIX;				\
		kern_funcs[1] = (void *)matrix_sort_step_##SUFFIX;				\
		kern_funcs[2] = (void *)matrix_sort_merge_##SUFFIX;				\
		for (i=0; i < 3; i++)											\
		{																\
			status = largest_workgroup_size(&grid_sz,					\
											&block_sz,					\
											kern_funcs[i],				\
											(valid_nrows + 1) / 2,		\
											0, 2 * sizeof(cl_uint));	\
			if (status != cudaSuccess)									\
				goto out;												\
			min_block_sz = Min(min_block_sz, block_sz.x);				\
		}																\
		/* block size to be 2^N */										\
		block_sz.x = (1 << (get_next_log2(min_block_sz + 1) - 1));		\
		block_sz.y = 1;													\
		block_sz.z = 1;													\
																		\
		/* nhalf is the least power of two value that is larger than	\
		 * or equal to half of the nitems. */							\
		nhalf = 1UL << (get_next_log2(height + 1) - 1);					\
																		\
		/*																\
		 * KERNEL_FUNCTION_MAXTHREADS(void)								\
		 * pgstrom_bitonic_local_#SUFFIX(...)							\
		 */																\
		__kern_args = (kern_arg_t *)									\
			cudaGetParameterBuffer(sizeof(kern_arg_t),					\
								   sizeof(kern_arg_t) * 8);				\
		if (!__kern_args)												\
		{																\
			status = cudaErrorLaunchOutOfResources;						\
			goto out;													\
		}																\
		memcpy(__kern_args, kern_argbuf, sizeof(kern_argbuf));			\
																		\
		grid_sz.x = ((valid_nrows + 1) / 2 +							\
					 block_sz.x - 1) / block_sz.x;						\
		grid_sz.y = 1;													\
		grid_sz.z = 1;													\
		status = cudaLaunchDevice((void *)matrix_sort_local_##SUFFIX,	\
								  __kern_args, grid_sz, block_sz,		\
								  2 * sizeof(cl_uint) * block_sz.x,		\
								  NULL);								\
		if (status != cudaSuccess)										\
			goto out;													\
		status = cudaDeviceSynchronize();								\
		if (status != cudaSuccess)										\
			goto out;													\
																		\
		/* inter blocks bitonic sorting */								\
		for (i = block_sz.x; i < nhalf; i *= 2)							\
		{																\
			for (j = 2 * i; j > block_sz.x; j /= 2)						\
			{															\
				cl_uint		unitSize = 2 * j;							\
				cl_uint		workSize;									\
																		\
				/*														\
				 * KERNEL_FUNCTION_MAXTHREADS(void)						\
				 * pgstrom_bitonic_step_#SUFFIX(...)					\
				 */														\
				__kern_args = (kern_arg_t *)							\
					cudaGetParameterBuffer(sizeof(kern_arg_t),			\
										   sizeof(kern_arg_t) * 10);	\
				if (!__kern_args)										\
				{														\
					status = cudaErrorLaunchOutOfResources;				\
					goto out;											\
				}														\
				memcpy(__kern_args, kern_argbuf, sizeof(kern_argbuf));	\
				__kern_args[8] = (kern_arg_t)(unitSize);				\
				__kern_args[9] = (kern_arg_t)(j == 2 * i ? true : false); \
																		\
				workSize = (((valid_nrows + unitSize - 1)				\
							 / unitSize) * unitSize / 2);				\
				grid_sz.x = (workSize + block_sz.x - 1) / block_sz.x;	\
				grid_sz.y = 1;											\
				grid_sz.z = 1;											\
																		\
				status = cudaLaunchDevice(								\
					(void *)matrix_sort_step_##SUFFIX,					\
					__kern_args, grid_sz, block_sz,						\
					0,													\
					NULL);												\
				if (status != cudaSuccess)								\
					goto out;											\
				status = cudaDeviceSynchronize();						\
				if (status != cudaSuccess)								\
					goto out;											\
			}															\
																		\
			/*															\
			 * Launch: pgstrom_bitonic_merge_SUFFIX						\
			 */															\
			__kern_args = (kern_arg_t *)								\
				cudaGetParameterBuffer(sizeof(kern_arg_t),				\
									   sizeof(kern_arg_t) * 8);			\
			if (!__kern_args)											\
			{															\
				status = cudaErrorLaunchOutOfResources;					\
				goto out;												\
			}															\
			memcpy(__kern_args, kern_argbuf, sizeof(kern_argbuf));		\
																		\
			grid_sz.x = ((valid_nrows + 1) / 2 +						\
						 block_sz.x - 1) / block_sz.x;					\
			grid_sz.y = 1;												\
			grid_sz.z = 1;												\
																		\
			status = cudaLaunchDevice((void *)matrix_sort_merge_##SUFFIX, \
									  __kern_args, grid_sz, block_sz,	\
									  2 * sizeof(cl_uint) * block_sz.x,	\
									  NULL);							\
			if (status != cudaSuccess)									\
				goto out;												\
			status = cudaDeviceSynchronize();							\
			if (status != cudaSuccess)									\
				goto out;												\
		}																\
	out:																\
		free(__sort_keys);												\
		return status;													\
	}																	\
																		\
	STATIC_INLINE(cudaError_t)											\
	pgstromMatrixSort##SUFFIX(cl_uint	   *row_index,					\
							  MatrixType   *matrix,						\
							  cl_uint	   *sort_keys,					\
							  cl_uint		num_keys,					\
							  cl_bool		is_descending)				\
	{																	\
		return pgstromMatrixPartialSort##SUFFIX(row_index,				\
											matrix,						\
											ARRAY_MATRIX_HEIGHT(matrix), \
											sort_keys,					\
											num_keys,					\
											is_descending);				\
	}

PGSTROM_MATRIX_SORT_TEMPLATE(FP32,cl_float,PG_FLOAT4OID)
PGSTROM_MATRIX_SORT_TEMPLATE(FP64,cl_double, PG_FLOAT8OID)

/* ----------------------------------------------------------------
 *
 * Group-by Support for PL/CUDA functions
 *
 * pgstromMatrixGroupAdd(FP32|FP64)(MatrixType *dst_matrix,
 *                                  MatrixType *src_matrix,
 *                                  cl_uint    *group_keys);
 * pgstromMatrixGroupMax(FP32|FP64)(...)
 * pgstromMatrixGroupMin(FP32|FP64)(...)
 *
 * These service routines support to make a summary according to the
 * grouping keys. Unlike SQL cases, grouping keys have to be integer
 * value between 0 and ARRAY_MATRIX_HEIGHT(dst_matrix) - 1.
 * The 'dst_matrix' has to be K rows x M columns matrix, the 'src_matrix'
 * has to be N rows x M columns matrix, and 'group_keys' have to be
 * an unsigned integer array with N-length that indicates the group of
 * individual rows of 'src_matrix'.
 * Each column of the 'dst_matrix' shall be added or updated on the row
 * indicated by the group_keys[].
 * We have two reduction mode: local and global. In case when large N
 * is mapped to small K, local reduction works well, then global reduction
 * will make the final results with less atomic confliction.
 * If 'K' is larger than 512, we try to update the 'dst_matrix' using atomic
 * operation directly, with no local reduction. It is heuristically better
 * choice than two step approach.
 * ----------------------------------------------------------------
 */
typedef struct
{
	cl_uint			errcode;
} matrixGroupState;

#define PGSTROM_MATRIX_GROUPBY_TEMPLATE(OP_NAME,SUFFIX,BASETYPE,		\
										ATOMIC_OPS,INIT_VAL)			\
	KERNEL_FUNCTION_MAXTHREADS(void)									\
	matrixGroup##OP_NAME##Twophase##SUFFIX(matrixGroupState *state,		\
										   MatrixType  *dst_matrix,		\
										   MatrixType  *src_matrix,		\
										   cl_uint	  *group_keys)		\
	{																	\
		cl_uint		width = ARRAY_MATRIX_WIDTH(src_matrix);				\
		BASETYPE   *src_addr;											\
		BASETYPE   *dst_addr;											\
		BASETYPE   *l_values;											\
		cl_uint	   *l_counts;											\
		cl_uint		dst_index;											\
		cl_uint		i, j, k, n;											\
		cl_bool		is_owner = false;									\
																		\
		k = ARRAY_MATRIX_HEIGHT(dst_matrix);							\
		n = ARRAY_MATRIX_HEIGHT(src_matrix);							\
																		\
		/* determine who is the owner of key */							\
		l_counts = SHARED_WORKMEM(cl_uint);								\
		for (i = get_local_id(); i < k; i++)							\
			l_counts[i] = 0;											\
		__syncthreads();												\
																		\
		if (get_global_id() < n)										\
		{																\
			dst_index = group_keys[get_global_id()];					\
			if (dst_index < k)											\
			{															\
				if (atomicAdd(&l_counts[dst_index], 1) == 0)			\
					is_owner = true;									\
			}															\
			else														\
			{															\
				/* invalid grouping key */								\
				atomicCAS(&state->errcode,								\
						  StromError_Success,							\
						  StromError_InvalidValue);						\
			}															\
		}																\
		else															\
			dst_index = (cl_uint)(-1U);	/* for compiler quiet */		\
		__syncthreads();												\
																		\
		src_addr = (BASETYPE *)ARRAY_MATRIX_DATAPTR(src_matrix);		\
		dst_addr = (BASETYPE *)ARRAY_MATRIX_DATAPTR(dst_matrix);		\
		for (i=0; i < width; i++)										\
		{																\
			/* clear the local summary buffer */						\
			l_values = SHARED_WORKMEM(BASETYPE);						\
			for (j = get_local_id(); j < k; j += get_local_size())		\
				l_values[j] = (INIT_VAL);								\
			__syncthreads();											\
																		\
			/* make a local summary of this column */					\
			if (get_global_id() < n && dst_index < k)					\
				ATOMIC_OPS(&l_values[dst_index],						\
						   src_addr[get_global_id()]);					\
			__syncthreads();											\
																		\
			/* put this local summary to the dst_matrix */				\
			if (is_owner)												\
			{															\
				assert(dst_index < k);									\
				ATOMIC_OPS(&dst_addr[dst_index],						\
						   l_values[dst_index]);						\
			}															\
			src_addr += n;												\
			dst_addr += k;												\
		}																\
	}																	\
																		\
	KERNEL_FUNCTION(void)												\
	matrixGroup##OP_NAME##Direct##SUFFIX(matrixGroupState *state,		\
										 MatrixType *dst_matrix,		\
										 MatrixType *src_matrix,		\
										 cl_uint    *group_keys)		\
	{																	\
		cl_uint		width = ARRAY_MATRIX_WIDTH(src_matrix);				\
		BASETYPE   *src_addr;											\
		BASETYPE   *dst_addr;											\
		cl_uint		dst_index;											\
		cl_uint		i, k, n;											\
																		\
		k = ARRAY_MATRIX_HEIGHT(dst_matrix);							\
		n = ARRAY_MATRIX_HEIGHT(src_matrix);							\
		if (get_global_id() < n)										\
		{																\
			dst_index = group_keys[get_global_id()];					\
			if (dst_index >= k)											\
				atomicCAS(&state->errcode,								\
						  StromError_Success,							\
						  StromError_InvalidValue);						\
		}																\
		else															\
			dst_index = (cl_uint)(0xffffffff);							\
																		\
		src_addr = (BASETYPE *)ARRAY_MATRIX_DATAPTR(src_matrix);		\
		dst_addr = (BASETYPE *)ARRAY_MATRIX_DATAPTR(dst_matrix);		\
		for (i=0; i < width; i++)										\
		{																\
			if (dst_index < k && get_global_id() < n)					\
				ATOMIC_OPS(&dst_addr[dst_index],						\
						   src_addr[get_global_id()]);					\
			src_addr += n;												\
			dst_addr += k;												\
		}																\
	}																	\
																		\
	STATIC_FUNCTION(cudaError_t)										\
	pgstromMatrixGroup##OP_NAME##SUFFIX(MatrixType *dst_matrix,			\
										MatrixType *src_matrix,			\
										cl_uint	   *group_keys)			\
	{																	\
		matrixGroupState *state;										\
		cl_uint		k, n;												\
		void	   *kern_func;											\
		cudaError_t	status;												\
																		\
		/* sanity checks */												\
		assert(ARRAY_MATRIX_ELEMTYPE(src_matrix) == PG_FLOAT4OID);		\
		assert(ARRAY_MATRIX_ELEMTYPE(dst_matrix) == PG_FLOAT4OID);		\
		assert(ARRAY_MATRIX_WIDTH(src_matrix) ==						\
			   ARRAY_MATRIX_WIDTH(dst_matrix));							\
																		\
		state = (matrixGroupState *)malloc(sizeof(matrixGroupState));	\
		if (!state)														\
			return cudaErrorMemoryAllocation;							\
																		\
		state->errcode = StromError_Success;							\
		n = ARRAY_MATRIX_HEIGHT(src_matrix);							\
		k = ARRAY_MATRIX_HEIGHT(dst_matrix);							\
		if (k > 512)													\
		{																\
			kern_func = (void *)matrixGroup##OP_NAME##Direct##SUFFIX;	\
			status = pgstromLaunchDynamicKernel4(kern_func,				\
												 (kern_arg_t)(state),	\
												 (kern_arg_t)(dst_matrix), \
												 (kern_arg_t)(src_matrix), \
												 (kern_arg_t)(group_keys), \
												 n,						\
												 0,						\
												 0);					\
			if (status != cudaSuccess)									\
				goto out;												\
		}																\
		else															\
		{																\
			kern_func = (void *)matrixGroup##OP_NAME##Twophase##SUFFIX;	\
			status = pgstromLaunchDynamicKernel4(kern_func,				\
												 (kern_arg_t)(state),	\
												 (kern_arg_t)(dst_matrix), \
												 (kern_arg_t)(src_matrix), \
												 (kern_arg_t)(group_keys), \
												 n,						\
												 0,						\
												 0);					\
			if (status != cudaSuccess)									\
				goto out;												\
		}																\
																		\
		if (state->errcode)												\
			status = cudaErrorInvalidValue;								\
	out:																\
		free(state);													\
		return status;													\
	}

PGSTROM_MATRIX_GROUPBY_TEMPLATE(Add,FP32,cl_float,atomicAdd,0.0)
//PGSTROM_MATRIX_GROUPBY_TEMPLATE(Max,FP32,cl_float,atomicMax,-FLT_MAX)
//PGSTROM_MATRIX_GROUPBY_TEMPLATE(Min,FP32,cl_float,atomicMin, FLT_MAX)


#endif	/* __CUDACC__ */
#endif	/* CUDA_MATRIX_H */
