## introduction
fetile

five 2D mesh networks for IO

two kinds of connection:
1. shared memory
2. directed

multicore Hardwall
##  The Processor Architecture Overview
tile comprocessor architecture consists of a 2D grid of identical compute elements, called tiles

five networks, every network follows it's own path.


application : network, router ??? why
problems : 

## hardware

### Tile 64 processor
64 core

rich IO resources

### Interconneted hardware
user dynamic network
IO dynamic network
IO dynamic network
static network
<!-- IO dynamic network -->
<!-- IO dynamic network -->
Why only six network or why so many networks ?

Logical or physical network ?

### Network-to-tile Interface

### Receive-side hardware demultipexing
one software-only solution to receive-side demultiplexing is to augment each message with a tag

### Protection

## Software
C-based lib for connection, similiar to MPI
