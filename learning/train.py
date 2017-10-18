#!/usr/bin/env python3

import argparse
import numpy as np
import model
import sys
import pandas as pd

from  sklearn.linear_model import LogisticRegression
from sklearn.ensemble import GradientBoostingClassifier
from sklearn import metrics

def pop_count(s):
  m = {}
  for i in s:
    if i not in m:
      m[i] = 0
    m[i] += 1
  return m

def CodeFormat(array, indent='', end=';\n', output=None):
  print(indent+ '{', file=output)
  if array.ndim == 1:
    print(indent + '  ', end='', file=output)
    line = 0
    for value in array:
      if line >= 4:
        print('', file=output)
        print(indent + '  ', end='', file=output)
        line = 0
      print('{: 2.8f},'.format(value), end='', file=output)
      line += 1
    print('', file=output)
  else:
    for subarray in array:
      CodeFormat(subarray, indent + '  ', end=',\n', output=output)
  print(indent + '}', end=end, file=output)

def GenCode(output, weights, bias):
  for i in range(len(weights)):
      w = weights[i]
      b = bias[i]
      output.write('float weights{idx}[{rows}][{cols}] = '.format(idx=i, rows=len(w), cols=len(w[0])))
      CodeFormat(w, output=output)
      output.write('\n')
      output.write('float bias{idx}[{rows}] = '.format(idx=i, rows=len(b)))
      CodeFormat(b, output=output)
      output.write('\n')

def main(args):
    features = []
    y_true = []
    outcomes = []
  
    data = []
    for f in args.files:
        df = pd.read_csv(f)
        if len(df) == 0:
            continue
        data.append(df)

    data = pd.concat(data)

    print(data.describe())

    FEATURES = [
            'ply',
            'edge_diff',
            'area_diff',
            'connected',
    ]
    TARGET = 'result'
  
    features = data[FEATURES]
    y_true = data[TARGET]
  
    print('size: {}'.format(len(features)))
    print('left: {:.2f}%'.format((y_true == 0).mean() * 100))
    print('right: {:.2f}%'.format((y_true == 2).mean() * 100))
    print('draws: {:.2f}%'.format((y_true == 1).mean() * 100))
  
    features = np.array(features, dtype='float32')
    y_true = np.array(y_true, dtype='int32')
  
    # classifier = GradientBoostingClassifier()
    # classifier = LogisticRegression()
    classifier = model.Classifier(len(FEATURES), 3)
    classifier.fit(features, y_true)
    # print('coefs:')
    # CodeFormat(classifier.coef_)
    # print('bias:')
    # CodeFormat(classifier.intercept_)
  
    y_pred_proba = classifier.predict_proba(features)
    print('proba:')
    print(y_pred_proba)

    y_pred = np.argmax(y_pred_proba, axis=1)
  
    print('accuracy: {:.2f}%'.format(classifier.score(features, y_true) * 100))
  
    print('Confusion matrix:')
    print(metrics.confusion_matrix(y_true, y_pred))

    weights, bias = classifier.model()
  
    if args.codegen:
        with open('model.cpp', 'w') as f:
            GenCode(f, weights, bias)
  
    # print('code:')
    # GenCode(sys.stdout, weights, bias)
  
    # skclassifier = LogisticRegression(C=1000.0)
    # skfeatures = features.reshape([-1, len(features[0][0])])
    # sky_true = (y_true_cat.reshape([-1, 1]) + np.array([0, 0, 0, 0])).reshape([-1])
    # skclassifier.fit(skfeatures, sky_true)
    # print('skscore: {:.2f}%'.format(skclassifier.score(skfeatures, sky_true)*100))
    # sky_pred = skclassifier.predict(skfeatures)
    # print('SK Confusion matrix:')
    # print(metrics.confusion_matrix(sky_true, sky_pred))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Run two bots againts each other.')
    parser.add_argument('files', nargs='+', help='The files to analyse')
    parser.add_argument('--codegen', default=False, action='store_true')
    args = parser.parse_args()
    main(args)
