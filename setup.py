# -*- coding: utf-8 -*-
"""
Created on Fri May  4 18:22:44 2018

@author: Subhy
"""

# from distutils.core import setup, Extension
import os.path as osp
# import sys
# from setuptools import setup, Extension, find_packages
# from setuptools import setup
# from numpy.lib import get_include
# from numpy.distutils.system_info import get_info as get_sys_info
from numpy.distutils.core import setup
from numpy.distutils.misc_util import Configuration, get_numpy_include_dirs
from numpy.distutils.misc_util import get_info as get_misc_info
from distutils.sysconfig import get_python_inc

# =========================================================================
config = Configuration('MfldProj', 'RandProjRandMan')

inc_dirs = [get_python_inc()]
if inc_dirs[0] != get_python_inc(plat_specific=1):
    inc_dirs.append(get_python_inc(plat_specific=1))
inc_dirs.append(get_numpy_include_dirs())

config.add_extension('_distratio',
                     sources=[osp.join('RandProjRandMan', 'MfldProj',
                                       'distratio.c')],
                     include_dirs=inc_dirs,
                     extra_info=get_misc_info("npymath"))

setup(**config.todict())
# =============================================================================
# numpy_inc = get_include()
# numpy_lib = osp.normpath(osp.join(numpy_inc, '..', 'lib'))
# py_inc = osp.join(sys.prefix, 'include')
# py_lib = osp.join(sys.prefix, 'libs')
#
# module1 = Extension('RandProjRandMan.MfldProj.distratio',
#                     sources=['RandProjRandMan/MfldProj/distratio.c'],
#                     include_dirs=[numpy_inc, py_inc],
#                     library_dirs=[numpy_lib, py_lib],
#                     libraries=['npymath'])
# # extra_compile_args
# # extra_link_args
#
# setup(name='RandProjRandMan.MfldProj.distratio',
#       version='1.0',
#       description='Ratios of cross/pair-wise distances squared',
#       packages=find_packages(),
#       ext_modules=[module1])
# =============================================================================