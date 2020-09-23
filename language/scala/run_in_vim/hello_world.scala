object HelloWorld {
   def main(args: Array[String]): Unit = {
     println("Hello, world!")

val Red = Value(0, "Stop")
val Yellow = Value(10) // Name "Yellow"
val Green = Value("Go") // ID 11

     val tr = TrafficLightColor.withName("Red")

     println(doWhat(tr));
   }
}

object TrafficLightColor extends Enumeration {
type TrafficLightColor = Value
val Red, Yellow, Green = Value
}

def doWhat(color: TrafficLightColor) = {
if (color == Red) "stop"
else if (color == Yellow) "hurry up"
else "go"
}

