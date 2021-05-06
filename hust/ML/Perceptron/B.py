import numpy as np
from numpy import linalg as LA
import sys
import os
import random
from random import randint
from skimage.io import imsave
from skimage import io
from PyQt5 import QtWidgets, QtGui, QtCore
from PyQt5.QtWidgets import QPushButton
from SimplePerceptron import SimplePerceptron
import time
from threading import Thread


PATH = "/home/martin/Documents/ML/data/mnist/"
IMAGES = "images27.npy"
LABELS = "labels27.npy"


class MNIST_Perceptron():
    def __init__(self, size):
        X = np.load(PATH + IMAGES)
        y = np.load(PATH + LABELS)

        self.y = np.empty((0, 1))
        self.X = np.empty((0, X.shape[1]))
        self.images = []
        index = set()

        filelist = [ f for f in os.listdir(PATH + "img/")]
        for f in filelist:
            os.remove(os.path.join(PATH + "img/", f))

        while(size != len(index)):
            i = randint(0, X.shape[0])
            index.add(i)

            data = np.reshape(X[i], (1, - 1))
            self.X = np.vstack((self.X, data))
            if(y[i] == 2):
                self.y = np.vstack((self.y, 1))
            else:
                self.y = np.vstack((self.y, -1))
            self.images.append(str(i) + '.png')
            imsave(PATH + 'img/' + str(i) + '.png', np.reshape(X[i], (28, 28)))


    def window(self):
        app=QtWidgets.QApplication(sys.argv)
        w=QtWidgets.QWidget()
        w.setGeometry(300,300,600,480)
        w.setWindowTitle('Mnist Perceptron')

        P = SimplePerceptron(2000)
        P.setData(self.X, self.y)

        # painter = QtGui.QPainter(w)
        # pen = QtGui.QPen()
        # pen.setWidth(6)
        # pen.setColor(QtGui.QColor.fromRgb(0, 255, 255))
        # painter.setPen(pen)
        # painter.setRenderHint(QtGui.QPainter.Antialiasing, True)
        # painter.drawLine(0, 240, 600, 240)


        qlabels = []
        for f in self.images:
            l1 = QtWidgets.QLabel(w)
            png = QtGui.QPixmap(PATH + "img/" + f)
            l1.setPixmap(png)
            qlabels.append(l1)
        w.show()

        def move_labels():
            loop = 0

            for i in range(self.y.shape[0]):
                qlabels[i].move(28 * i, 240)
                print(self.y[i])
            time.sleep(1)

            while(P.next()):
                max_dis = 0
                dis = np.reshape(P.distance, (-1))

                i = 0
                for i in dis:
                    max_dis = max(max_dis, abs(i))
                    i = i + 1

                i = 0
                for l in qlabels:
                    l.move(28 * i, dis[i] * self.y[i] / max_dis * 240 - 28 + 240)
                    i = i + 1
                print("loop : ", loop)
                loop = loop + 1
                time.sleep(1)

        t = Thread(target=move_labels)
        t.start()

        app.exit(app.exec_())





def t():
    a = np.array([[ 2, 3]])
    b = np.array([[3, 4]])
    print(a)
    print(b)
    print(a * b)
    print(LA.norm(b))



if __name__ == '__main__':
    M = MNIST_Perceptron(20)
    M.window()
    t()

