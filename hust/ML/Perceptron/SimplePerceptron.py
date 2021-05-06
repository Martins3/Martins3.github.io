import numpy as np
from numpy import linalg as LA
from sklearn.linear_model import Perceptron
from sklearn.preprocessing import PolynomialFeatures
class SimplePerceptron():
    def __init__(self, max_iter = 10000):
        self.max_iter = max_iter

    def fit(self, X, y):
        """
        get the w and b
        """
        # check input

        O = np.ones((X.shape[0], 1))
        X = X / 600
        X = np.hstack((X, O))
        self.w = np.zeros((X.shape[1], 1))
        # self.w[0] = 0
        # self.w[1] = 0
        self.w[2] = 500
        print(X)
        while(True):
            ok = True
            for i in range(X.shape[0]):
                if(y[i] * np.matmul(X[i], self.w) <= 0):
                    ok = False
                    #  self.w = self.w  + np.reshape(y[i] * X[i], (-1, 1))
                    delta = np.reshape(y[i] * X[i], (-1, 1))
                    self.w = self.w  + delta
                    # break
            if(ok):
                break
            self.max_iter = self.max_iter - 1
            if(self.max_iter == 0):
                break
        self.b = self.w[-1][0] * 600
        self.w = self.w[0:-1]

    def setData(self, X, y):
        O = np.ones((X.shape[0], 1), dtype=np.float64)
        self.X = np.hstack((X, O))
        self.w = np.zeros((self.X.shape[1], 1), dtype=np.float64)
        self.y = y

    def next(self):
        ok = True
        for i in range(self.X.shape[0]):
            if(self.y[i] * np.matmul(self.X[i], self.w) <= 0):
                ok = False
                self.w = self.w  +  np.reshape(self.y[i] * self.X[i], (-1, 1))
        if(ok):
            return False

        self.max_iter = self.max_iter - 1
        if(self.max_iter == 0):
            return False
        self.distance = (np.matmul(self.X, self.w) * self.y) / LA.norm(self.w)
        return True

