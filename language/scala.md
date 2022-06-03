# Scala

## Learn the basic syntax

### Class
> primary constructor
> all kinds of getter and setter

## Object
Use an object in Scala whenever you would have used a singleton object in Java
or C++:
• As a home for utility functions or constants
• When a single immutable instance can be shared efficiently
• When a single instance is required to coordinate some service (the singleton
design pattern)

The class and its companion object can access each other’s private features. They
must be located in the same source file

[class and object in scala](https://stackoverflow.com/questions/1755345/difference-between-object-and-class-in-scala)
[implict class](https://stackoverflow.com/questions/40878893/implicit-classes-in-scala)
[case class](https://stackoverflow.com/questions/5270752/difference-between-case-object-and-object)
[case object](https://madusudanan.com/blog/scala-tutorials-part-10-case-objects-in-scala/)


## questions
1. parameters of class
```scala
class Animal(val name: String, val species: String)
class Human(val name: String)
implicit def human2animal(h: Human): Animal = {
    new Animal(h.name, "Homo sapiens")
}
val me = new Human("Adam")
println(me.species)
```

## Inherience
> Superclass Construction

#### Use Vim
http://seenaburns.com/vim-setup-for-scala/

1. Scala functions that do not have any arguments do not require empty parenthese
2. By convention, argument-less functions that do not have side effects (i.e. calling them does not change anything and they simply return a value) do not use parentheses, and functions that do have side effects (perhaps they change class variables or print stuff out) should require parentheses.
3. Overloaded functions should be avoided, though.
