/* Common code for calling BLAS/Lapack from GUFuncs
*/
/*
Adapted from https://github.com/numpy/numpy/numpy/linalg/umath_linalg.c.src
Copyright/licence info for that file:
* Copyright (c) 2005-2017, NumPy Developers.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*   - Redistributions of source code must retain the above
*     copyright notice, this list of conditions and the
*     following disclaimer.
*   - Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer
*     in the documentation and/or other materials provided with the
*     distribution.
*   - Neither the name of the author nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*         Table of Contents
47.  Includes
63.  Fortran compatibility tools
95.  Structs used for data rearrangement
*/

#ifndef GUF_INCLUDE
#define GUF_INCLUDE

/*
*****************************************************************************
**                         To use BLAS/LAPACK                              **
*****************************************************************************
*/

#define NPY_NO_DEPRECATED_API NPY_API_VERSION

#include "Python.h"
#include "numpy/ndarraytypes.h"
#include "numpy/arrayobject.h"
#include "numpy/ufuncobject.h"
#include "numpy/npy_math.h"
#include "numpy/npy_3kcompat.h"
// #include "npy_config.h"

/*
*****************************************************************************
**                         To use BLAS/LAPACK                              **
*****************************************************************************
*/

typedef int               fortran_int;
typedef float             fortran_real;
typedef double            fortran_doublereal;

static NPY_INLINE fortran_int
fortran_int_min(fortran_int x, fortran_int y) {
 return x < y ? x : y;
}

static NPY_INLINE fortran_int
fortran_int_max(fortran_int x, fortran_int y) {
 return x > y ? x : y;
}

#ifdef NO_APPEND_FORTRAN
# define FNAME(x) x
#else
# define FNAME(x) x##_
#endif

#define BLAS(FUNC)                              \
    FNAME(FUNC)

#define LAPACK(FUNC)                            \
    FNAME(FUNC)

/*
*****************************************************************************
**               Structs used for data rearrangement                       **
*****************************************************************************
*/

/*
* this struct contains information about how to linearize a matrix in a local
* buffer so that it can be used by blas functions.  All strides are specified
* in bytes and are converted to elements later in type specific functions.
*
* rows: number of rows in the matrix
* columns: number of columns in the matrix
* row_strides: the number bytes between consecutive rows.
* column_strides: the number of bytes between consecutive columns.
* output_lead_dim: BLAS/LAPACK-side leading dimension, in elements
*/
typedef struct linearize_data_struct
{
    npy_intp rows;
    npy_intp columns;
    npy_intp row_strides;
    npy_intp column_strides;
    npy_intp output_lead_dim;
} LINEARIZE_DATA_t;

static NPY_INLINE void
init_linearize_data_ex(LINEARIZE_DATA_t *lin_data,
                        npy_intp rows,
                        npy_intp columns,
                        npy_intp row_strides,
                        npy_intp column_strides,
                        npy_intp output_lead_dim)
{
    lin_data->rows = rows;
    lin_data->columns = columns;
    lin_data->row_strides = row_strides;
    lin_data->column_strides = column_strides;
    lin_data->output_lead_dim = output_lead_dim;
}

static NPY_INLINE void
init_linearize_data(LINEARIZE_DATA_t *lin_data,
                     npy_intp rows,
                     npy_intp columns,
                     npy_intp row_strides,
                     npy_intp column_strides)
{
    init_linearize_data_ex(
        lin_data, rows, columns, row_strides, column_strides, columns);
}

/*
* this struct contains information about how to linearize a vector in a local
* buffer so that it can be used by blas functions.  All strides are specified
* in bytes and are converted to elements later in type specific functions.
*
* len: number of elements in the vector
* strides: the number bytes between consecutive elements.
*/
typedef struct linearize_vdata_struct
{
  npy_intp len;
  npy_intp strides;
} LINEARIZE_VDATA_t;


static NPY_INLINE void
init_linearize_vdata(LINEARIZE_VDATA_t *lin_data,
                    npy_intp len,
                    npy_intp strides)
{
    lin_data->len = len;
    lin_data->strides = strides;
}

#endif
