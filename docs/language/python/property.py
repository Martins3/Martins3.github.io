## getter / setter
# https://stackoverflow.com/questions/2627002/whats-the-pythonic-way-to-use-getters-and-setters
class C:
    def __init__(self):
        self._x = None

    @property
    def x(self):
        """I'm the 'x' property."""
        print("getter of x called")
        return self._x

    @x.setter
    def x(self, value):
        print("setter of x called")
        self._x = value

    @x.deleter
    def x(self):
        print("deleter of x called")
        del self._x


c = C()
c.x = "foo"  # setter called
foo = c.x  # getter called
del c.x  # deleter called
