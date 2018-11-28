import pandas as pd
import matplotlib.pyplot as plt
from scipy import stats

mean = df.mean(axis = 1)
std = df.std(axis = 1)

n= df.shape[1]
yerr = std / np.sqrt(n) * stats.t.ppf(1-0.05/2, n - 1)

plt.figure()
plt.bar(range(df.shape[0]), mean, yerr = yerr)
plt.savefig('plt.png')
