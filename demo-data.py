# -*- coding: utf-8 -*-
"""
Created on Fri Nov 17 16:16:37 2017

@author: Subhy

Quick demo of generating data for plots
"""

from RandProjRandMan import run


if __name__ == "__main__":
    """
    First block: quick demo of generating & saving data
    """

    run.icc_data(False, '_test')
    run.ics_data(False, '_test')
    run.gs_data(False, '_test')
    run.rpm_num(False, '_test')