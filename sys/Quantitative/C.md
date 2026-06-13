# Pipelining: Basic and Intermediate Concepts

## C.1 Introduction

#### C2
> *data path implication* is ?

#### C3
Define *pipe stage* *pipe segment* *throughput* *processor cycle*

Show two advantages of pipeline: *not visible to programmer* *reduce CPI*
#### C4
List three properties of RISC lead to dramatic simplification of implementation of pipeline
#### C5
Explain five stage: IF ID EC MEM WB
#### C6
#### C7
Three explanation about the ideal pipeline
#### C8
pipeline register
0. between pipeline stage
1. source and destination is not directly adjacent
2. multicyle data path

#### C9

pipeline lead to some overhead
#### C10

## C.2 The Major Hurdle of Pipelining---Pipeline Hazards

#### C11
structural hazards occur primarily in special purpose functional units and we focus on data hazard and control hazard

analyze performance of pipeline with stalls
#### C12
define data hazard
#### C13
use example to explain data hazard
#### C14
minimizing data hazard stalls by forwarding
#### C15
#### C16
data hazard requiring stalls
#### C17
branch hazard
> the part has been skip, read it if you need

#### C22
As pipelines get deeper and the potential penalty of branches increases, using **delayed branches** and similar schemes becomes insufficient.
> I didn't cover delayed branches, it seems related to delayed branch slot, cover it soon

Instead, we need to turn to more aggressive means for predicting branches. Such schemes fall into two classes:
1. low-cost static schemes that rely on information available at compile time and
2. strategies that predict branches dynamically based on program behavior.

**static branch prediction**
A key way to improve compile-time branch prediction is to use profile information collected from earlier runs.

The key observation that makes this worthwhile is that the behavior of branches is often bimodally distributed; 

The fact that the misprediction rate for the integer programs is higher and such programs typically have a higher branch frequency is a major limitation for static branch prediction

The actual performance depends on both the prediction accuracy and the branch frequency
#### C23
The simplest dynamic branch-prediction scheme is a branch-prediction buffer or branch history table. 

A branch-prediction buffer is a small memory indexed by the lower portion of the address of the branch instruction. The memory contains a bit
that says whether the branch was recently taken or not. This scheme is the simplest sort of buffer; it has no tags and *is useful only to reduce the branch delay when it is
longer than the time to compute the possible target PCs*
> disagree, why cost more time can be more useful

With such a buffer, we don’t know, in fact, if the prediction is correct—it may
have been put there by another branch that has the same low-order address bits

This buffer is effectively a cache where every access is a hit, and, as we will see,
the performance of the buffer depends on both how often the prediction is for the
branch of interest and how accurate the prediction is when it matches

A branch-prediction buffer can be implemented as a small, special “cache” accessed with the instruction address during the IF pipe stage,
or *as a pair of bits attached to each block in the instruction cache* and fetched with the instruction.

If the instruction is decoded as a branch and if the branch is predicted as taken, fetching begins from the target as soon as the PC is known.
Otherwise, sequential fetching and executing continue.


## C.3 How is Pipeline Implemented ?

## C.4 What makes Pipelining Hard to Implement?

#### C38
list types of Exception

#### C39
the requirements on exceptions can be characterized on five semi-independent axes:

> What is user requested versus coerced, can you show any example of this two type of exception ?

#### C40
define different exception types with five categories
#### C41
pipeline takes three steps to save the pipeline state safely

> the definition of precise exceptions have no clear specification of the behavior of faulting instruction although it define the before and after

#### C42
for interger pipeline, precise exception is critical, for floating-point exception, two mode is supported
#### C43
show how to exceptions in RISC-V

hardware posts all exceptions caused by a given instruction in a status vector associated with that instruction
#### C44
> skiped

#### C45
> skiped

#### C46
two changed for FP pipeline

assume four separate functional units
#### C47
show pipeline structure if execution stages are not pipelined
#### C48
show pipeline structure if execution stages are pipelined
#### C49
hazard and forwarding in longer latency pipelines
#### C50
> *We could increase the number of write ports to solve this, but that solution may
be unattractive because the additional write ports would be used only rarely. This
is because the maximum steady-state number of write ports needed is 1*  where indicate the truth that *maximum steady-state number of write ports needed is 1*

#### C51
> *A simple, though sometimes suboptimal, heuristic is to give priority to the unit
with the longest latency, because that is the one most likely to have caused another
instruction to be stalled for a RAW hazard* firstly, there seems to have a English syntax fault, "A simple is to give", Secondly, why it can cause RAW porblem.


> *we can use a simple solution: if an instruction in ID wants to write the same register as an instruction already issued, do not issue the instruction to EX* it seems no easy, not only needs *knowing the length of the pipeline and the current position of the fadd.d*, but also also should add some more logic to know make sure it can do.

#### C52
All integer instructions operate on the integer registers, while
the FP operations operate only on their own registers

Assuming that the pipeline does all hazard detection in ID, there are three checks that must be performed
before an instruction can issue:
1. Check for structural hazards
2. Check for a RAW data hazard
3. Check for a WAW data hazard

why not check war data hazard, because ID stage is sequencial, this problem is raised,

 
#### C53
> It seems that we have already discussed *precise exception* already, but now why should we still have a section for, anything new ?

#### C54
#### C55
> what is *FP result stall* ?

## Putting It All Together: The MIPS R4000 Pipeline
> skip this chapter, and we will review it if we have time

## Cross-Cutting Issues

#### C65
We have already discussed the advantages of instruction set simplicity in building
pipelines. Simple instruction sets offer another advantage: they make it easier to
schedule code to achieve efficiency of execution in a pipeline

All the techniques discussed in this appendix so far use in-order instruction
issue, which means that if an instruction is stalled in the pipeline, no later instructions can proceed

#### C66
> *To allow an instruction to begin execution as soon as its operands are available, even if a predecessor is stalled, we must separate the issue process
into two parts: checking the structural hazards and waiting for the absence of a data
hazard* why by separating ID into two part, we can issue dynamicly

In a dynamically scheduled pipeline, all instructions pass through the issue stage in
order (in-order issue); however, they can be stalled or bypass each other in the second stage (read operands) and thus enter execution out of order

Scoreboarding is a
technique for allowing instructions to execute out of order when there are sufficient
resources and no data dependences

#### C67
all of this page is important
#### C68
#### C69
wiki is better
https://en.wikipedia.org/wiki/Scoreboarding

some question about scoreboard:
1. should a instruction hold the execution uint at the begining of the issue and hold it until it write back,
if all instructions are integer Arthimtica, then pipeline will stall for ever

2. http://users.utcluj.ro/~sebestyen/_Word_docs/Cursuri/SSC_course_5_Scoreboard_ex.pdf
http://ece-research.unm.edu/jimp/611/slides/chap4_3.html
