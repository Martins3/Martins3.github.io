import numpy as np
from PyQt5 import QtWidgets, QtGui, QtCore
from PyQt5.QtWidgets import QPushButton
from sklearn.linear_model import Perceptron
from SimplePerceptron import SimplePerceptron



class ImageScroller(QtWidgets.QWidget):

    def __init__(self):
        self.blue_points = []
        self.green_points = []
        QtWidgets.QWidget.__init__(self)
        self._image = QtGui.QPixmap("/home/martin/Pictures/Wallpapers/2d32ec75baaa952c4a4865440b4d794c.jpg")

        self.pybutton = QPushButton('Run', self)
        self.pybutton.resize(32,32)
        self.pybutton.move(0, 0)
        self.pybutton.clicked.connect(self.clickMethod)

        self.pybutton = QPushButton('Clear', self)
        self.pybutton.resize(32,32)
        self.pybutton.move(0, 32)
        self.pybutton.clicked.connect(self.clearMethod)

        self.classify = False

    def drawPerceptronLine(self, painter):
        self.classify = False
        X = np.empty(shape=[0, 2], dtype = np.float)
        Y = []
        for pos in self.blue_points:
            data = np.array([pos.x(), pos.y()])
            X = np.vstack((X, data))
            Y.append(1)

        for pos in self.green_points:
            data = np.array([pos.x(), pos.y()])
            X = np.vstack((X, data))
            Y.append(-1)

        Y = np.asarray(Y)
        print(X)
        print(Y)

        k = 0
        w = 0
        if(False):
            P = Perceptron(max_iter = 1000000, penalty = 'l2')
            clf = P.fit(X, Y)
            k =  -P.coef_[0][0] / P.coef_[0][1]
            w = -P.intercept_[0] / P.coef_[0][1]
        else:
            P = SimplePerceptron()
            P.fit(X, Y)
            print(P.w)
            k = -P.w[0][0] / P.w[1][0]
            w =  -P.b / P.w[1][0]
        print("k", k)
        print("w", w)

        # find the data
        sx = 0
        sy = 0
        if(w > 0):
            sx = 0
            sy = w
        else:
            sx = -w / k
            sy = 0

        ex = 1000
        ey = ex * k + w

        pen = QtGui.QPen()
        pen.setWidth(6)

        pen.setColor(QtGui.QColor.fromRgb(0, 255, 255))
        painter.setPen(pen)
        painter.setRenderHint(QtGui.QPainter.Antialiasing, True)
        painter.drawLine(sx, sy, ex, ey)


    def resizeEvent(self, event):
        print("")
        # handle the relative thing

    def clickMethod(self):
        self.classify = True
        self.update()

    def clearMethod(self):
        self.blue_points = []
        self.green_points = []
        self.update()


    def paintEvent(self, paint_event):
        painter = QtGui.QPainter(self)
        pen = QtGui.QPen()
        pen.setWidth(6)

        pen.setColor(QtGui.QColor.fromRgb(255, 255, 0))
        painter.setPen(pen)
        painter.setRenderHint(QtGui.QPainter.Antialiasing, True)
        for pos in self.green_points:
            painter.drawPoint(pos)

        pen.setColor(QtGui.QColor.fromRgb(255, 0, 255))
        painter.setPen(pen)
        painter.setRenderHint(QtGui.QPainter.Antialiasing, True)
        for pos in self.blue_points:
            painter.drawPoint(pos)
        if(self.classify):
            qp = QtGui.QPainter(self)
            qp.begin(self)
            self.drawPerceptronLine(qp)
            qp.end()



    def mouseReleaseEvent(self, cursor_event):
        if(cursor_event.button() == 1):
            self.green_points.append(cursor_event.pos())
        if(cursor_event.button() == 2):
            self.blue_points.append(cursor_event.pos())

        # self.chosen_points.append(cursor_event.pos())
        # self.chosen_points.append(self.mapFromGlobal(QtGui.QCursor.pos()))
        self.update()


if __name__ == '__main__':
    import sys
    app = QtWidgets.QApplication(sys.argv)
    w = ImageScroller()
    w.resize(640, 480)
    w.show()
    sys.exit(app.exec_())
