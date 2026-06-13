from mnist import MNIST
import numpy as np

PATH = '/home/martin/Documents/ML/data/mnist/'
IMAGES = "images.npy"
LABELS = "labels.npy"

def get_data():
    mndata = MNIST(PATH)
    mndata.gz = True
    images, labels = mndata.load_training()
    images = np.asarray(images)
    labels = np.asarray(labels)
    np.save(PATH + "images", images)
    np.save(PATH + "labels", labels)
    print(images.shape)
    print(labels.shape)

def get_two_num(m, n):
    X = np.load(PATH + IMAGES)
    y = np.load(PATH + LABELS)
    print(X.shape)
    print(y.shape)

    length = X.shape[0]
    images = np.empty((0, X.shape[1]), dtype=int)
    labels = []
    for i in range(length):
        if(y[i] == m or y[i] == n):
            print(i)
            images = np.vstack((images, X[i]))
            labels.append(y[i])
    labels = np.asarray(labels)
    np.save(PATH + "images" + str(m) + str(n), images)
    np.save(PATH + "labels"+ str(m) + str(n), labels)




if __name__ == '__main__':
    get_two_num(2, 7)
