#!/usr/bin/env python3

import pandas as pd
from scipy import stats
import numpy as np
import sys

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

plt.ioff()

df = np.loadtxt(sys.argv[1]).T

mean = df.mean(axis = 1)
std = df.std(axis = 1)

n = df.shape[1]
yerr = std / np.sqrt(n) * stats.t.ppf(1-0.05/2, n - 1)

x_axis = ['0.2', '10', '20']

plt.xlabel('Emulated packet corruption (%)')
plt.ylabel('File transfer time (s)')
plt.bar(x_axis, mean, yerr = yerr)
plt.savefig('plot-corruption.png')
