/**
# -*- c -*-
# -*- coding: utf-8 -*-
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

/*              Table of Contents
51.  Includes
70.  Docstrings
135. Structs used for array iteration
171. PDIST_RATIO and CDIST_RATIO
319. MATMUL
377. NORM
412. Ufunc definition
434. Module initialization stuff
*/

/*
*****************************************************************************
**                            INCLUDES                                     **
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

#include "gufunc_common.h"

static const char* gufuncs_cloop_version_string = "0.1.1";

/*
*****************************************************************************
**                   Doc string for Python functions                       **
*****************************************************************************
*/

PyDoc_STRVAR(pdist_ratio__doc__,
//"pdist_ratio(X: ndarray, P: ndarray) -> (drmax: float, drmin: float)\n\n"
"Maximum and minimum ratio of pair-wise distances squared beween correspoinding "
"pairs of points in two sets.\n\n"
"Parameters\n-----------\n"
"X: ndarray (...,P,N)\n"
"    Set of points between which we compute pairwise distances for the denominator. "
"    Each point is a row.\n"
"P: ndarray (...,P,M)\n"
"    Set of points between which we compute pairwise distances for the numerator.\n\n"
"Returns\n-------\n"
"drmax: float\n"
"    Maximum ratio of distances squared.\n"
"drmin: float\n"
"    Minimum ratio of distances squared.\n");

PyDoc_STRVAR(cdist_ratio__doc__,
//"cdist_ratio(XA: ndarray, XB: ndarray, PA: ndarray, PB: ndarray) -> "
//"(drmax: float, drmin: float)\n\n"
"Maximum and minimum ratio of cross-wise distances squared beween corresponding "
"pairs of points in two groups of two sets.\n\n"
"Parameters\n-----------\n"
"XA: ndarray (...,P,N)\n"
"    Set of points *from* which we compute pairwise distances for the denominator. "
"    Each point is a row.\n"
"XB: ndarray (...,R,N)\n"
"    Set of points *to* which we compute pairwise distances for the denominator.\n"
"PA: ndarray (...,P,M)\n"
"    Set of points *from* which we compute pairwise distances for the numerator.\n"
"PB: ndarray (...,R,M)\n"
"    Set of points *to* which we compute pairwise distances for the numerator.\n\n"
"Returns\n-------\n"
"drmax: float\n"
"    Maximum ratio of distances squared.\n"
"drmin: float\n"
"    Minimum ratio of distances squared.\n");

PyDoc_STRVAR(matmul__doc__,
//"matmul(X: ndarray, Y: ndarray) -> (Z: ndarray)\n\n"
"Matrix-matrix product.\n\n"
"Parameters\n-----------\n"
"X: ndarray (...,M,N)\n"
"    Matrix multiplying from left.\n"
"Y: ndarray (...,N,P)\n"
"    Matrix multiplying from right.\n\n"
"Returns\n-------\n"
"Z: ndarray (...,M,P)\n"
"    Result of matrix multiplication.");

PyDoc_STRVAR(norm__doc__,
//"matmul(X: ndarray, Y: ndarray) -> (Z: ndarray)\n\n"
"Euclidean norm of a vector.\n\n"
"Parameters\n-----------\n"
"X: ndarray (...,N)\n"
"    Vector, or array of vectors.\n\n"
"Returns\n-------\n"
"Z: float\n"
"    Euclidean norm of X.");

/*
*****************************************************************************
**               Structs used for array iteration                          **
*****************************************************************************
*/

/*
* this struct contains information about how to iterate through an array
*
* len: number of elements in the vector
* strides: the number bytes between consecutive elements.
* back_strides: the number of bytes from start to end of vector.
*/
typedef struct linearize_data_struct
{
    npy_intp len;
    npy_intp strides;
    npy_intp back_strides;
} LINEARIZE_DATA_t;


static NPY_INLINE void
init_linearize_vdata(LINEARIZE_DATA_t *lin_data,
                    npy_intp len,
                    npy_intp strides)
{
    lin_data->len = len;
    lin_data->strides = strides;
    lin_data->back_strides = len * strides;
}

/*
*****************************************************************************
**                             UFUNC LOOPS                                 **
*****************************************************************************
*/

/* **********************************
    PDIST_RATIO and CDIST_RATIO
********************************** */

static void
DOUBLE_dist(const char *X, const char *Y, npy_double *dist,
            const LINEARIZE_DATA_t *x_in, const LINEARIZE_DATA_t *y_in)
{
    npy_intp m;
    npy_double separation;
    for (m = 0; m < x_in->len; m++) {
        separation = ((*(npy_double *)X) - (*(npy_double *)Y));
        *dist += separation * separation;

        X += x_in->strides;  // next vec element
        Y += y_in->strides;
    }  // for m
    X -= x_in->back_strides;  // reset to start of vec
    Y -= y_in->back_strides;
}

// char *pdist_ratio_signature = "(d,m),(d,n)->(),()";

static void
DOUBLE_pdist_ratio(char **args, npy_intp *dimensions, npy_intp *steps,
                   void *NPY_UNUSED(func))
{
INIT_OUTER_LOOP_4
    npy_intp len_d = *dimensions++;  // number of points
    npy_intp len_m = *dimensions++;  // dimensions of numerator
    npy_intp len_n = *dimensions++;  // dimensions of denominator
    npy_intp stride_num_d = *steps++;  // numerator
    npy_intp stride_m = *steps++;
    npy_intp stride_den_d = *steps++;  // denominator
    npy_intp stride_n = *steps++;
    npy_intp d1, d2;
    npy_intp iback_m = len_m * stride_m;  // step back at end of loop
    npy_intp iback_n = len_n * stride_n;
    LINEARIZE_DATA_t num_in, den_in;

    init_linearize_vdata(&num_in, len_m, stride_m);
    init_linearize_vdata(&den_in, len_n, stride_n);

    BEGIN_OUTER_LOOP_4

        const char *ip_num_fr = args[0];  //  from-ptr: numerator
        const char *ip_den_fr = args[1];  //  from-ptr: denominator
        char *op1 = args[2], *op2 = args[3];
        npy_double dr_min = d_inf, dr_max = d_zero;  // running min/max distance ratio

        for (d1 = 0; d1 < len_d-1; d1++) {

            const char *ip_num_to = ip_num_fr + stride_num_d;  //  to-ptr: numerator
            const char *ip_den_to = ip_den_fr + stride_den_d;  //  to-ptr: denominator

            for (d2 = d1 + 1; d2 < len_d; d2++) {

                npy_double numerator = d_zero, denominator = d_zero;

                DOUBLE_dist(ip_num_fr, ip_num_to, &numerator, &num_in, &num_in);
                DOUBLE_dist(ip_den_fr, ip_den_to, &denominator, &den_in, &den_in);

                npy_double ratio = numerator / denominator;
                if (ratio < dr_min) dr_min = ratio;  // update running max/min
                if (ratio > dr_max) dr_max = ratio;

                ip_num_to += stride_num_d;  // next point: to
                ip_den_to += stride_den_d;

            }  // for d2
            ip_num_fr += stride_num_d;  // next point: from
            ip_den_fr += stride_den_d;

        }  // for d1
        *(npy_double *)op1 = npy_sqrt(dr_min);
        *(npy_double *)op2 = npy_sqrt(dr_max);

    END_OUTER_LOOP_4
}

// char *cdist_ratio_signature = "(d1,m),(d2,m),(d1,n),(d2,n)->(),()";

static void
DOUBLE_cdist_ratio(char **args, npy_intp *dimensions, npy_intp *steps,
                   void *NPY_UNUSED(func))
{
INIT_OUTER_LOOP_6
    npy_intp len_fr_d = *dimensions++;  // number of points, from
    npy_intp len_m = *dimensions++;  // dimensions of numerator
    npy_intp len_to_d = *dimensions++;  // number of points, to
    npy_intp len_n = *dimensions++;  // dimensions of denominator
    npy_intp stride_num_fr_d = *steps++;  // numerator, from
    npy_intp stride_fr_m = *steps++;
    npy_intp stride_num_to_d = *steps++;  // numerator, to
    npy_intp stride_to_m = *steps++;
    npy_intp stride_den_fr_d = *steps++;  // denominator, from
    npy_intp stride_fr_n = *steps++;
    npy_intp stride_den_to_d = *steps++;  // denominator, to
    npy_intp stride_to_n = *steps++;
    npy_intp d1, d2;
    npy_intp iback_fr_m = len_m * stride_fr_m;  // step back at end of loop
    npy_intp iback_to_m = len_m * stride_to_m;
    npy_intp iback_fr_n = len_n * stride_fr_n;
    npy_intp iback_to_n = len_n * stride_to_n;
    LINEARIZE_DATA_t num_fr_in, num_to_in, den_fr_in, den_to_in;

    init_linearize_vdata(&num_fr_in, len_m, stride_fr_m);
    init_linearize_vdata(&num_to_in, len_m, stride_to_m);
    init_linearize_vdata(&den_fr_in, len_n, stride_fr_n);
    init_linearize_vdata(&den_to_in, len_n, stride_to_n);

    BEGIN_OUTER_LOOP_6

        const char *ip_num_fr = args[0];  //  from-ptr: numerator
        const char *ip_den_fr = args[2];  //  from-ptr: denominator
        char *op1 = args[4], *op2 = args[5];
        npy_double dr_min = d_inf, dr_max = d_zero;  // min/max distance ratio

        for (d1 = 0; d1 < len_fr_d; d1++) {

            const char *ip_num_to = args[1];  //  to-ptr: numerator
            const char *ip_den_to = args[3];  //  to-ptr: denominator

            for (d2 = 0; d2 < len_to_d; d2++) {

                npy_double numerator = d_zero, denominator = d_zero;

                DOUBLE_dist(ip_num_fr, ip_num_to, &numerator, &num_fr_in, &num_to_in);
                DOUBLE_dist(ip_den_fr, ip_den_to, &denominator, &den_fr_in, &den_to_in);

                npy_double ratio = numerator / denominator;
                if (ratio < dr_min) dr_min = ratio;  // update running max/min
                if (ratio > dr_max) dr_max = ratio;

                ip_num_to += stride_num_to_d;  // next point: to
                ip_den_to += stride_den_to_d;

            }  // for d2
            ip_num_fr += stride_num_fr_d;  // next point: from
            ip_den_fr += stride_den_fr_d;

        }  // for d1
        *(npy_double *)op1 = npy_sqrt(dr_min);
        *(npy_double *)op2 = npy_sqrt(dr_max);

    END_OUTER_LOOP_6
}

/* **********************************
            MATMUL
********************************** */
// char *matmul_signature = "(m,n),(n,p)->(m,p)";

static void
DOUBLE_matmul(char **args, npy_intp *dimensions, npy_intp *steps,
              void *NPY_UNUSED(func))
{
INIT_OUTER_LOOP_3

    npy_intp len_m = *dimensions++;  // dimensions of left
    npy_intp len_n = *dimensions++;  // dimensions of inner
    npy_intp len_p = *dimensions++;  // dimensions of right
    npy_intp stride_x_m = *steps++;  // 1st arg
    npy_intp stride_x_n = *steps++;
    npy_intp stride_y_n = *steps++;  // 2nd arg
    npy_intp stride_y_p = *steps++;
    npy_intp stride_z_m = *steps++;  // output
    npy_intp stride_z_p = *steps++;
    npy_intp m, n, p;
    npy_intp iback_x_n = len_n * stride_x_n;  // step back at end of loop
    npy_intp iback_y_n = len_n * stride_y_n;
    npy_intp iback_y_p = len_p * stride_y_p;  // step back at end of loop
    npy_intp iback_z_p = len_p * stride_z_p;

    BEGIN_OUTER_LOOP_3

        const char *ip_x= args[0];  //  1st arg
        const char *ip_y= args[1];  //  2nd arg
        char *op_z = args[2];       //  output

        for (m = 0; m < len_m; m++) {
            for (p = 0; p < len_p; p++) {
                *(npy_double *)op_z = d_zero;

                for (n = 0; n < len_n; n++) {
                    *(npy_double *)op_z += (*(npy_double *)ip_x) * (*(npy_double *)ip_y);

                    ip_x += stride_x_n;
                    ip_y += stride_y_n;
                }
                ip_x -= iback_x_n;
                ip_y -= iback_y_n;

                ip_y += stride_y_p;
                op_z += stride_z_p;
            }
            ip_y -= iback_y_p;
            op_z -= iback_z_p;

            ip_x += stride_x_m;
            op_z += stride_z_m;
        }

    END_OUTER_LOOP_3
}

/* **********************************
            NORM
********************************** */
// char *norm_signature = "(n)->()";

static void
DOUBLE_norm(char **args, npy_intp *dimensions, npy_intp *steps,
              void *NPY_UNUSED(func))
{
INIT_OUTER_LOOP_2

    npy_intp len_n = *dimensions++;  // dimensions of inner
    npy_intp stride_n = *steps++;
    npy_intp n;
    npy_double normsq;

    BEGIN_OUTER_LOOP_2

        const char *ip_x= args[0];  //  1st arg
        char *op_r = args[1];       //  output
        normsq = d_zero;

        for (n = 0; n < len_n; n++) {
            normsq += *(npy_double *)ip_x * *(npy_double *)ip_x;

            ip_x += stride_n;
        }
        *(npy_double *)op_r = npy_sqrt(normsq);

    END_OUTER_LOOP_2
}



/*
*****************************************************************************
**                             UFUNC DEFINITION                            **
*****************************************************************************
*/

GUFUNC_FUNC_ARRAY_REAL(pdist_ratio);
GUFUNC_FUNC_ARRAY_REAL(cdist_ratio);
GUFUNC_FUNC_ARRAY_REAL(matmul);
GUFUNC_FUNC_ARRAY_REAL(norm);

GUFUNC_DESCRIPTOR_t gufunc_descriptors[] = {
    {"pdist_ratio", "(d,m),(d,n)->(),()", pdist_ratio__doc__,
     1, 2, 2, FUNC_ARRAY_NAME(pdist_ratio), ufn_types_1_4},
    {"cdist_ratio", "(d1,m),(d2,m),(d1,n),(d2,n)->(),()", cdist_ratio__doc__,
     1, 4, 2, FUNC_ARRAY_NAME(cdist_ratio), ufn_types_1_6},
    {"matmul", "(m,n),(n,p)->(m,p)", matmul__doc__,
     1, 2, 1, FUNC_ARRAY_NAME(matmul), ufn_types_1_3},
    {"norm", "(n)->()", norm__doc__,
     1, 1, 1, FUNC_ARRAY_NAME(norm), ufn_types_1_2}
};

/*
*****************************************************************************
**               Module initialization stuff                               **
*****************************************************************************
*/

static PyMethodDef GUfuncs_Cloop_Methods[] = {
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "_gufuncs_cloop",
        NULL,
        -1,
        GUfuncs_Cloop_Methods,
        NULL,
        NULL,
        NULL,
        NULL
};

PyObject *PyInit__gufuncs_cloop(void)
{
    PyObject *m;
    PyObject *d;
    PyObject *version;
    int failure;

    init_constants();
    m = PyModule_Create(&moduledef);
    if (m == NULL) {
        return NULL;
    }

    import_array();
    import_ufunc();

    d = PyModule_GetDict(m);

    version = PyString_FromString(gufuncs_cloop_version_string);
    PyDict_SetItemString(d, "__version__", version);
    Py_DECREF(version);

    /* Load the ufunc operators into the module's namespace */
    failure = addUfuncs(d, gufunc_descriptors, 4);

    if (PyErr_Occurred() || failure) {
        PyErr_SetString(PyExc_RuntimeError,
                        "cannot load _gufuncs_cloop module.");
        return NULL;
    }

    return m;
}
