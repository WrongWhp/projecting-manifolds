# -*- coding: utf-8 -*-
"""
Created on Mon Jun 19 16:23:33 2017

@author: Subhy

Package: mfld
=======================
Testing formulae for geometry or random manifolds.

Modules
=======
gauss_curve
    Simulate and plot tests of formulae for geometry of random curves.
gauss_curve_plot
    Plot tests of formulae for geometry of random curves.
gauss_curve_theory
    Formulae for geometry of random curves.
gauss_surf
    Simulate geometry of random surfaces.
gauss_surf_theory
    Formulae for geometry of random surfaces.
gauss_surf_plot
    Plot tests of formulae for geometry of random surfaces.
"""
from . import (gauss_curve, gauss_curve_plot, gauss_mfld, gauss_mfld_plot,
               gauss_mfld_theory)
assert all((gauss_curve, gauss_curve_plot, gauss_mfld, gauss_mfld_plot,
            gauss_mfld_theory))
