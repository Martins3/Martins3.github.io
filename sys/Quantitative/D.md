# Storage System


## D.1 Introduction
#### D2
This shift in focus from computation to communication and storage of information emphasizes reliability and scalability as well as cost-performance.

yet it also has its own rich performance theory—queuing theory—that balances throughput versus response time

The software that determines which processor features get used is the compiler, but the operating system usurps that role for storage.

## Advanced Topics in Disk Storage
Cost per gigabyte has dropped at least as fast as areal density has increased, with smaller diameter drives playing the larger role in this improvement

#### D3

#### D4
While disks will remain viable for the foreseeable future, the conventional sector-track-cylinder model did not
1. First, disks started offering higher-level intelligent interfaces, like ATA and
SCSI, when they included a microprocessor inside a disk
2. Second, shortly after the microprocessors appeared inside disks, the disks
included buffers to hold the data until the computer was ready to accept it, and later
caches to avoid read accesses

> both this two method isn't understood clearly.

#### D5
> *Finally, the number of platters shrank from 12 in the past to 4 or even 1 today,
so the cylinder has less importance than before because the percentage of data in a
cylinder is much less.* I can not understand this

Thus, smaller platters, slower rotation, and fewer platters all help reduce disk motor
power, and most of the power is in the motor.
#### D6
> skip, becaues RAID has alread learned, D7 D8 D9 has same operation

#### D7
#### D8
#### D9
#### D10
The terms fault, error, and failure are often used interchangeably, but they have different meanings in the dependability literature. 

definition of **dependability**
#### D11
define what the terminology
#### D12
> skip until D.4

## I/O Performance, Reliability Measures, and Benchmarks
I/O throughput is sometimes called I/O bandwidth and response time is sometimes called latency.
#### D16
> *Another measure of I/O performance is the interference of I/O with processor
execution. Transferring data may interfere with the execution of another process.
There is also overhead due to handling I/O interrupts. Our concern here is how
much longer a process will take because of I/O for another process.* how to interfere with the execution of another process, show a example ?

#### D17
a graph describes throughput versus respones time

entry time, system response time, think time
#### D18
show three IO benchmark
> skip several pages, because I don't want to know the details of IO benchmark

#### D21
examples of benchmark of dependability

Brown and Patterson proposed that availability be measured by
examining the variations in system quality-of-service metrics over time as faults
are injected into the system

the longer the reconstruction (MTTR), the lower the availability
> wait, what is the definition of availability

there is a policy choice between taking
a performance hit during reconstruction or lengthening the window of vulnerability
and thus lowering the predicted MTTF
> what's MTTF ?

#### D22
#### D23
Because of the probabilistic nature of I/O events and because of sharing of I/O resources, we can give a set of simple
theorems that will help calculate response time and throughput of an entire I/O system. This helpful field is called queuing theory.
#### D24
> *This flow-balanced state is necessary but not sufficient for steady state* why flow-balanced is not suffcient ? 

Mean number of tasks in system ¼ Arrival rateMean response time
#### D25



