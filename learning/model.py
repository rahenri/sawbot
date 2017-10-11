import tensorflow as tf
import numpy as np


class DataSet:
    def __init__(self, features, labels):
        self._features = features
        self._labels = labels
        self._offset = 0
        self._examples = features.shape[0]
        self._indices = np.arange(self._examples)
        self.epochs = 0

    def Next(self, batch_size):
        if self._offset + batch_size >= self._examples:
            self._offset = 0
            np.random.shuffle(self._indices)
            self._features = self._features[self._indices]
            self._labels = self._labels[self._indices]
            self.epochs += 1

        start = self._offset
        end = self._offset + batch_size

        self._offset += batch_size

        return self._features[start:end], self._labels[start:end]

    def __len__(self):
        return len(self._features)


def infinity():
    i = 0
    while True:
        yield i
        i += 1


def weight_variable(shape):
    initial = tf.truncated_normal(shape, stddev=0.1)
    return tf.Variable(initial)


def bias_variable(shape):
    initial = tf.constant(0.1, shape=shape)
    return tf.Variable(initial)


class Classifier:
    def __init__(self, features_count, classes, T=100):

        self.features_count = features_count
        self.classes = classes

        self.features = tf.placeholder(
                'float', shape=[None, features_count], name='x')
        self.y_true = tf.placeholder('int32', shape=[None], name='y_')

        one_hot_y = tf.one_hot(self.y_true, classes)
        print(one_hot_y.get_shape())

        # self.keep_prob = tf.placeholder('float')
        # self.keep_prob_value = keep_prob

        weights = []
        biases = []

        # Build inner layers
        last = self.features
        last_size = features_count
        var_count = 0
        for layer in [10, 10]:
            print('CompleteLayer({})'.format(layer))
            w = weight_variable([last_size, layer])
            b = bias_variable([layer])
            var_count += layer * last_size
            weights.append(w)
            biases.append(b)
            last = tf.sigmoid(tf.matmul(last, w) + b)
            last_size = layer

        # if keep_prob < 1.0:
        #   print('Dropout(KeepProb: {})'.format(keep_prob))
        #   last = tf.nn.dropout(last, keep_prob)

        # Readout layer
        w = weight_variable([last_size, classes])
        b = bias_variable([classes])
        self.y = tf.nn.softmax(tf.matmul(last, w) + b)
        var_count += last_size * classes
        weights.append(w)
        biases.append(b)

        self.weights = weights
        self.biases = biases

        # self.threshold = tf.placeholder('float32', name='threshold')

        self.cross_entropy = -tf.reduce_mean(
                one_hot_y*tf.log(tf.maximum(0.00001, self.y)))

        reg = sum([tf.reduce_mean(
            tf.square(w)) for w in self.weights]) / var_count

        self.loss = self.cross_entropy + reg / T

        optimizer = tf.train.AdamOptimizer(0.001)
        self.train = optimizer.minimize(self.loss)

        init = tf.global_variables_initializer()

        self.sess = tf.Session()
        self.sess.run(init)

    def fit(self, features, y_true):
        data_set = DataSet(features, y_true)

        BATCH_SIZE = 400
        try:
            last_improve = 0
            best_loss = 1.0e10
            CHECK = 1000
            for step in infinity():
                batchx, batchy = data_set.Next(BATCH_SIZE)
                feed = {self.features: batchx, self.y_true: batchy}
                self.sess.run(self.train, feed_dict=feed)
                if step % CHECK == 0:
                    epoch = step // CHECK
                    feed = {
                            self.features: data_set._features,
                            self.y_true: data_set._labels}
                    score = self.sess.run(self.loss, feed_dict=feed)
                    print(step, score)
                    if score < best_loss:
                        best_loss = score
                        last_improve = epoch
                    elif epoch - last_improve > 5:
                        # Stop if it didn't improve for a while
                        break
        except KeyboardInterrupt:
            pass

    @property
    def coef_(self):
        return self.sess.run(self.W)

    @property
    def intercept_(self):
        return self.sess.run(self.b)

    def predict_proba(self, features):
        feed = {self.features: features}
        return self.sess.run(self.y, feed_dict=feed)

    def predict(self, features):
        proba = self.predict_proba(features)
        return np.argmax(proba, axis=1)

    def score(self, features, y_true):
        pred = self.predict(features)
        return (y_true == pred).mean()

    def model(self):
        weights = self.sess.run(self.weights)
        biases = self.sess.run(self.biases)
        return weights, biases
