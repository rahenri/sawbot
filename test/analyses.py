#!/usr/bin/env python3

import pandas

# import matplotlib.pyplot as plt

import numpy as np
from scipy.optimize import curve_fit, minimize

def sigmoid(value):
  return 1.0 / (1.0 + np.exp(-value))

def max_point(xdata, ydata):
  variables = len(xdata)
  weights = (variables + 1) * (variables + 2) // 2

  def fit_func(X, *params):
    prod = 1.0
    assert len(X) == variables
    ret = params[0]
    var_id = 1
    for i in range(variables):
      x = X[i]
      ret += x * params[var_id]
      var_id += 1
      for j in range(i, variables):
        y = X[j]
        ret += x * y * params[var_id]
        var_id += 1
    assert var_id == len(params)
    ret = sigmoid(ret / 10.0)
    print(ret)
    return ret

  p0 = np.array((1,) * weights)
  popt, pcov = curve_fit(fit_func, xdata, ydata, p0)
  print('popt: {}'.format(popt))

  minmax = (xdata.min(1), xdata.max(1))
  bounds = np.array(list(zip(*minmax)))
  print('minmax:\n', minmax)
  print('bounds:\n', bounds)
  fm = lambda x: -fit_func(x, *popt)
  r = minimize(fm, bounds.mean(1), bounds=bounds)
  return r["x"], fit_func(r["x"], *popt)

def main():

  data = pandas.read_csv('train_result.csv')

  print(data.describe())

  buckets = {}
  count = {}

  color_map = {
    -1: (1, 0, 0),
     0: (0, 0, 1),
     1: (0, 1, 1),
  }

  column_names = [
          'up_weight',
          'down_weight',
          'left_weight',
          'right_weight',
  ]


  # X = []
  # Y = []
  # c = []
  # c = np.array([color_map[v] for v in data.result])

  print('Samples: {}'.format(len(data)))

  result = np.array((data.result + 1) / 2)

  columns = np.array([data[name] for name in column_names]) / 10000

  maximum_coords, max_value = max_point(columns, result)

  print('Maximum value: {}'.format(max_value))
  for name, value in zip(column_names, maximum_coords):
    print('{}: {}'.format(name, value))

  # buckets = {}
  # bucket_count = {}
  # for i in range(len(data)):
  #   k = data.approach_weapon[i]
  #   v = result[i]
  #   if not k in buckets:
  #     buckets[k] = 0
  #     bucket_count[k] = 0
  #   buckets[k] += v
  #   bucket_count[k] += 1

  # for k in buckets:
  #   buckets[k] /= bucket_count[k]

  # print(sorted(buckets.items()))


if __name__ == '__main__':
  main()
