# Embedded Systems

## E.1 Introduction
Two other key characteristics exist in many embedded applications: the need to
minimize memory and the need to minimize power.

In practice, embedded problems are usually solved by one of three approaches:
1. The designer uses a combined hardware/software solution that includes some
custom hardware and an embedded processor core that is integrated with the
custom hardware, often on the same chip.
2. The designer uses custom software running on an off-the-shelf embedded
processor.
3. The designer uses a digital signal processor and custom software for the processor. Digital signal processors are processors specially tailored for signalprocessing applications. We discuss some of the important differences between
digital signal processors and general-purpose embedded processors below.

Why do these vendors favor VLIW over superscalar? For
the embedded space, code compatibility is less of a problem, and so new applications can be either hand tuned or recompiled for the newest generation of processor. The other reason superscalar excels on the desktop is because the compiler
cannot predict memory latencies at compile time. In embedded, however, memory
latencies are often much more predictable. In fact, hard real-time constraints force
memory latencies to be statically predictable. Of course, a superscalar would also
perform well in this environment with these constraints, but the extra hardware to
dynamically schedule instructions is both wasteful in terms of precious chip area
and in terms of power consumption. Thus VLIW is a natural choice for highperformance embedded.

Although the interprocessor interactions in such
designs are highly regimented and relatively simple—consisting primarily of a
simple communication channel—because much of the design is committed to silicon, ensuring that the communication protocols among the input/output processors and the general-purpose processor are correct is a major challenge in such
designs.

Multiprocessing is becoming widespread in the embedded computing arena for
two primary reasons.
First, the issues of binary software compatibility, which plague desktop and server systems, are less relevant in the embedded space
Second, the applications often have natural parallelism, especially at the high end of the embedded space
