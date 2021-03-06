# -*- coding: utf-8 -*-
# =============================================================================
# Created on Mon Jun 19 13:29:22 2017
#
# @author: Subhy based on Peiran's Matlab code
#
# Module: rand_proj_mfld_mem
# =============================================================================
"""
Calculation of distribution of maximum distortion of Gaussian random manifolds
under random projections, low memory version

Functions
=========
region_inds_list
    indices of points and pairs of points on mfld corresponding to the central
    region of the manifold
distortion_m
    Maximum distortion of all chords between points on the manifold,
    sampling projectors, for each V, M
"""
from typing import Sequence, Tuple, List, Mapping
from numbers import Real
import numpy as np

from ..myarray import array, pdist_ratio, cdist_ratio
from ..iter_tricks import dbatch, denumerate, rdenumerate
from ..mfld.gauss_mfld import SubmanifoldFTbundle
from . import rand_proj_mfld_util as ru

Nind = array  # Iterable[int]  # Set[int]
Pind = array  # Iterable[Tuple[int, int]]  # Set[Tuple[int, int]]
Inds = Tuple[Nind, Pind]

# =============================================================================
# %%* region indexing
# =============================================================================


def region_inds_list(shape: Sequence[int],
                     mfld_fs: Sequence[float]) -> List[List[Inds]]:
    """
    List of index sets for different sized regions, each index set being the
    indices of condensed matrix returned by pdist corresponding to the
    central region of the manifold

    Parameters
    ----------
    shape
        tuple of nujmber of points along each dimension (max(K),)
    mfld_fs
        list of fractions of manifold to keep (#(V))

    Returns
    -------
    region_inds
        list of lists of tuples of arrays containing indices of: new & previous
        points in K-d subregions (#(V),#(K),2), each element an array of
        indices of shape ((fL)^K - #(prev),) or (#(prev),), where:
        #(prev) = (fL)^K-1 + (f'L)^K - (f'L)^K-1
    """
    # new indices, & those seen before, for each f, K
    region_inds = []
    # all indices for previous f, for all K
    prev_fs = [np.array([], int) for i in range(len(shape))]
    # loop over f
    for frac in mfld_fs:
        # all indices, for this f, for all K
        all_inds = ru.region_indices(shape, frac)
        # arrays to store new & previous for this f, all K
        ind_arrays = []
        # all indices, for new f, for previous K
        prev_K = np.array([], int)
        # loop over K
        for aind, prev_f in zip(all_inds, prev_fs):
            # indices seen before this f & K
            pind = np.union1d(prev_f, prev_K)
            # remove previous f & K to get new indices
            nind = np.setdiff1d(aind, pind, assume_unique=True)
            # store new & previous for this K
            ind_arrays.append((nind, pind))
            if len(np.intersect1d(nind, pind)):
                print(np.intersect1d(nind, pind))
            # update all indices for this f, previous K (next K iteration)
            prev_K = aind
        # store new & previous for this f, all K
        region_inds.append(ind_arrays)
        # update all indices for previous f, all K (next f iteration)
        prev_fs = all_inds
    return region_inds


# =============================================================================
# %%* distortion calculations
# =============================================================================


def distortion(vecs: array, pvecs: array, inds: Inds) -> array:
    """Distortion of a chord

    Parameters
    ----------
    vecs : array (L,N)
        points in the manifold
    pvecs : array (S,L,M)
        corresponding points in the projected manifold
    inds : Tuple(array[int], array[int])
        tuples of arrays containing indices of: new & previous points in
        subregions (2,), each element an array of indices of shape
        ((fL)^K - #(prev),) or (#(prev),),
        where: #(prev) = (fL)^K-1 + (f'L)^K - (f'L)^K-1

    Returns
    -------
    distortion : array (S,)
        maximum distortion of chords
    """
    scale = np.sqrt(vecs.shape[-1] / pvecs.shape[-1])
    distn = np.zeros(pvecs.shape[:1])  # (S,)
    ninds, pinds = inds
    if len(ninds) > 0:
        # (S, 2)
        lratio = np.stack(pdist_ratio(pvecs[:, ninds], vecs[ninds]), axis=-1)
        # use fmax to ignore NaN, (S,)
        distn = np.fmax(distn, np.abs(scale * lratio - 1.).max(axis=-1))
        if len(pinds) > 0:
            lratio = np.stack(cdist_ratio(pvecs[:, ninds], pvecs[:, pinds],
                                          vecs[ninds], vecs[pinds]), axis=-1)
            distn = np.fmax(distn, np.abs(scale * lratio - 1.).max(axis=-1))
    return distn


def distortion_v(mfld: SubmanifoldFTbundle,
                 proj_mflds: SubmanifoldFTbundle,
                 region_inds: Sequence[Sequence[Inds]]) -> array:
    """
    Max distortion of all tangent vectors and chords between points in various
    regions manifold, for all V

    Parameters
    ----------
    mfld: SubmanifoldFTbundle
        mfld[st...,i]
            = phi_i(x[s],y[t],...), (L,N)
            Embedding functions of random surface
        gmap[st...,i,A]
            = e_A^i(x[s], y[t]).
            orthonormal basis for tangent space, (L,N,K)
            e_(A=0)^i must be parallel to d(phi^i)/dx^(a=0)
    proj_mflds: SubmanifoldFTbundle
        mfld[q,st...,i]
            = phi_i(x[s],y[t],...), (S,L,M)
            Embedding functions of random surface
        gmap[q,st...,i,A]
            = e_A^i(x[s], y[t]).
            orthonormal basis for tangent space, (S,L,M,K)
            e_(A=0)^i must be parallel to d(phi^i)/dx^(a=0)
    region_inds
        list of lists of tuples of arrays containing indices of: new & previous
        points in K-d subregions (#(V),#(K),2), each element an array of
        indices of shape ((fL)^K - #(prev),) or (#(prev),), where:
        #(prev) = (fL)^K-1 + (f'L)^K - (f'L)^K-1

    Returns
    -------
    epsilon = max distortion of all chords (#(K),#(V),S)
    """
    # tangent space distortions, (K,)(S,L)
    gdistn = ru.distortion_gmap(proj_mflds, mfld.ambient)

    distn = np.empty((len(region_inds[0]),
                      len(region_inds),
                      proj_mflds.shape[0]))  # (#(K),#(V),S)

    for v, inds in denumerate('Vol', region_inds):
        for k, gdn, pts in denumerate('K', gdistn, inds):
            distn[k, v] = gdn[:, pts[0]].max(axis=-1)  # (S,)
            np.maximum(distn[k, v],
                       distortion(mfld.mfld, proj_mflds.mfld, pts),
                       out=distn[k, v])

    # because each entry in region_inds  only contains new points
    np.maximum.accumulate(distn, axis=0, out=distn)  # (#(K),#(V),S)
    np.maximum.accumulate(distn, axis=1, out=distn)  # (#(K),#(V),S)

    return distn


def distortion_m(mfld: SubmanifoldFTbundle,
                 proj_dims: array,
                 uni_opts: Mapping[str, Real],
                 region_inds: Sequence[Sequence[Inds]]) -> array:
    """
    Maximum distortion of all chords between points on the manifold,
    sampling projectors, for each V, M

    Parameters
    ----------
    mfld: SubmanifoldFTbundle
        mfld[st...,i]
            = phi_i(x[s],y[t],...), (L,N)
            Embedding functions of random surface
        gmap[st...,i,A]
            = e_A^i(x[s], y[t]).
            orthonormal basis for tangent space, (L,N,K)
            e_(A=0)^i must be parallel to d(phi^i)/dx^(a=0)
    proj_dims
        ndarray of M's, dimensionalities of projected space (#(M),)
    uni_opts
            dict of scalar options, used for all parameter values, with fields:
        num_samp
            number of samples of distortion for empirical distribution
        batch
            sampled projections are processed in batches of this length.
            The different batches are looped over (mem version).
        chunk
            chords are processed (vectorised) in chunks of this length.
            The different chunks are looped over (mem version).
    region_inds
        list of lists of tuples of arrays containing indices of: new & previous
        points in K-d subregions (#(V),#(K),2), each element an array of
        indices of shape ((fL)^K - #(prev),) or (#(prev),), where:
        #(prev) = (fL)^K-1 + (f'L)^K - (f'L)^K-1

    Returns
    -------
    epsilon = max distortion of chords for each (#(K),#(V),#(M),S)
    """
    # preallocate output. (#(K),#(V),#(M),S)
    distn = np.empty((len(region_inds[0]), len(region_inds),
                      len(proj_dims), uni_opts['samples']))

    batch = uni_opts['batch']
    for s in dbatch('Sample', 0, uni_opts['samples'], batch):
        # projected manifold for each sampled proj, (S,Lx*Ly...,max(M))
        # gauss map of projected mfold for each proj, (#K,)(S,L,K,max(M))
        pmflds = ru.project_mfld(mfld, proj_dims[-1], batch)

        # loop over M
        for m, M in rdenumerate('M', proj_dims):
            # distortions of all chords in (K-dim slice of) manifold
            distn[..., m, s] = distortion_v(mfld, pmflds.sel_ambient(M),
                                            region_inds)
    return distn


# =============================================================================
# test code
# =============================================================================


if __name__ == "__main__":
    print('Run from outside package.')
