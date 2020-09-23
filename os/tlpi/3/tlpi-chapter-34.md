# Linux Programming Interface: Process Group, Sessions, and Job Control



Process groups and sessions form a two-level hierarchical relationship between processes: a process group is a collection of related processes, and a session is a collection of related process groups.

Process groups and sessions are abstractions defined to **support shell job control**,
which allows interactive users to run commands in the foreground or in the
background. The term `job` is often used synonymously with the term **process group.**

## 34.1 Overview
process group is a set of one or more processes sharing the same process group identifier (PGID)

A process group ID is a number of the same type (pid_t) as a process ID. 
A process group has a process group leader, which is the process that creates
the group and whose process ID becomes the process group ID of the group.
A new process inherits its parent’s process group ID.

A session is a collection of process groups. A process’s session membership is
determined by its session identifier (SID), which, like the process group ID, 
is a number of type pid_t. A session leader is the process that creates a new session and whose
process ID becomes the session ID. A new process inherits its parent’s session ID

**All of the processes in a session share a single controlling terminal.**
**The controlling terminal is established when the session leader first opens a terminal device. A
terminal may be the controlling terminal of at most one session.**

At any point in time, one of the process groups in a session is the foreground
process group for the terminal, and the others are background process groups. 
Only processes in the foreground process group can read input from the controlling terminal.

As a consequence of establishing the connection to (i.e., opening) the controlling terminal, the session leader becomes the controlling process for the terminal.
The principal significance of being the controlling process is that the kernel sends
this process a `SIGHUP` signal if a terminal disconnect occurs.

**The main use of sessions and process groups is for shell job control.**
**Looking at a specific example** from this domain helps clarify these concepts. For an interactive
login, the controlling terminal is the one on which the user logs in. The login shell
becomes the session leader and the controlling process for the terminal, and is also
made the sole member of its own process group. Each command or pipeline of
commands started from the shell results in the creation of one or more processes, and
the shell places all of these processes in a new process group. (These processes are
initially the only members of that process group, although any child processes that
they create will also be members of the group.) A command or pipeline is created
as a background process group if it is terminated with an ampersand (&). Otherwise, it becomes the foreground process group. All processes created during the
login session are part of the same session.


In a windowing environment, the controlling terminal is a pseudoterminal,
and there is a separate session for each terminal window, with the window’s
startup shell being the session leader and controlling process for the terminal.

**Process groups occasionally find uses in areas other than job control,
since they have two useful properties: a parent process can wait on any of its
children in a particular process group (Section 26.1.2), and a signal can be sent
to all of the members of a process group (Section 20.5).**

## 34.2 Process Groups
The typical callers of `setpgid()` (and `setsid()`, described in Section 34.3) are programs such as the shell and `login(1)`. In Section 37.2, we’ll see that a program also
calls setsid() as one of the steps on the way to becoming a daemon.

Several restrictions apply when calling setpgid():
- The pid argument may specify only the calling process or one of its children.
Violation of this rule results in the error ESRCH.
- When moving a process between groups, the calling process and the process
specified by pid (which may be one and the same), as well as the target process group, must all be part of the same session. Violation of this rule results in
the error EPERM.
- The pid argument may not specify a process that is a session leader. Violation
of this rule results in the error EPERM.
- A process may not change the process group ID of one of its children after that
child has performed an exec(). Violation of this rule results in the error EACCES.
*The rationale for this constraint is that it could confuse a program if its process
group ID were changed after it had commenced.*
> 其实这几条限制为了切换 process group leader 的时候都是在 session 内部实现的 

* ***Using setpgid() in a job-control shell***
> 说明由于上述的限制，所以 setpgid 实际上的工作方式是什么 ?

* ***Other (obsolete) interfaces for retrieving and modifying process group IDs***
> skip 
## 34.3 Sessions
> 讲解 setsid

## 34.4 Controlling Terminals and Controlling Processes
> 很迷
> 什么叫做 controlling Terminals 
> 什么只能打开一次

## 34.5 Foreground and Background Process Groups
**The controlling terminal maintains the notion of a foreground process group.**
**Within a session, only one process can be in the foreground at a particular moment;**
all of the other process groups in the session are background process groups. The
foreground process group is the only process group that can freely read and write
on the controlling terminal. When one of the signal-generating terminal characters
is typed on the controlling terminal, the terminal driver delivers the corresponding
signal to the members of the foreground process group. We describe further
details in Section 34.7.

The `tcgetpgrp()` and `tcsetpgrp()` functions respectively retrieve and
change the process group of a terminal. These functions are used primarily by **job-control shells**.

> 为什么 terninal 是使用 fd 描述的 ?

## 34.6 The SIGHUP Signal

When a controlling process loses its terminal connection, the kernel sends it a
`SIGHUP` signal to inform it of this fact. (A `SIGCONT` signal is also sent, to ensure that the
process is restarted in case it had been previously stopped by a signal.) Typically,
this may occur in two circumstances:
- When a “disconnect” is detected by the terminal driver, indicating a loss of signal
on a modem or terminal line.
- When a terminal window is closed on a workstation. This occurs because the
last open file descriptor for the master side of the pseudoterminal associated
with the terminal window is closed.


The delivery of SIGHUP to the controlling process can set off a kind of chain reaction,
resulting in the delivery of SIGHUP to many other processes. This may occur in two ways:
- The controlling process is typically a shell. The shell establishes a handler for
`SIGHUP`, so that, before terminating, it can send a `SIGHUP` to each of the jobs that it
has created. This signal terminates those jobs by default, but if instead they
catch the signal, then they are thus informed of the shell’s demise.
- Upon termination of the controlling process for a terminal,
the kernel disassociates all processes in the session from the controlling terminal, disassociates the
controlling terminal from the session (so that it may be acquired as the controlling
terminal by another session leader), and informs the members of the foreground
process group of the terminal of the loss of their controlling terminal by sending
them a `SIGHUP` signal.

#### 34.6.1 Handling of SIGHUP by the Shell
> 很酷，程序现在也看不懂了

#### 34.6.2 SIGHUP and Termination of the Controlling Process

## 34.7 Job Control

#### 34.7.1 Using Job Control Within the Shell

The various states of a job under job control, as well as the shell commands and terminal characters (and the accompanying signals) used to move a job between these
states, are summarized in **Figure 34-2**. This figure also includes a notional
terminated state for a job. This state can be reached by sending various signals to the
job, including SIGINT and SIGQUIT, which can be generated from the keyboard.
> Figure 34-2 的内容非常复杂，但是对于其理解一般

#### 34.7.2 Implementing Job Control
This support requires the following:
- The implementation must provide certain job-control signals: SIGTSTP, SIGSTOP,
SIGCONT, SIGTTOU, and SIGTTIN. In addition, the SIGCHLD signal (Section 26.3) is also
necessary, since it allows the shell (the parent of all jobs) to find out when one
of its children terminates or is stopped.

- The terminal driver must support generation of the job-control signals, so that
when certain characters are typed, or terminal I/O and certain other terminal
operations (described below) are performed from a background job, an appropriate signal (as shown in Figure 34-2) is sent to the relevant process group. In
order to be able to carry out these actions, the terminal driver must also record
the session ID (controlling process) and foreground process group ID associated with a terminal (Figure 34-1).

- The shell must support job control (most modern shells do so). This support is
provided in the form of the commands described earlier to move a job between
the foreground and background and monitor the state of jobs. Certain of these
commands send signals to a job (as shown in Figure 34-2). In addition, when
performing operations that move a job between the running in foreground and
any of the other states, the shell uses calls to `tcsetpgrp()` to adjust the terminal
driver’s record of the foreground process group.

* ***The SIGTTIN and SIGTTOU signals***

* ***Example program: demonstrating the operation of job control***

#### 34.7.3 Handling Job-Control Signals

## 34.8 Summary
Sessions and process groups (also known as jobs) form a two-level hierarchy of processes:
a session is a collection of process groups, and a process group is a collection
of processes. A session leader is the process that created the session using setsid().
Similarly, a process group leader is the process that created the group using
setpgid(). All of the members of a process group share the same process group ID
(which is the same as the process group ID of the process group leader), and all
processes in the process groups that constitute a session have the same session ID
(which is the same as the ID of the session leader). **Each session may have a controlling
terminal (/dev/tty), which is established when the session leader opens a terminal
device. Opening the controlling terminal also causes the session leader to become
the controlling process for the terminal.**
> controlling terminal 和 controlling process

Sessions and process groups were defined to support shell job control (although
occasionally they find other uses in applications). Under job control, **the shell is the
session leader and controlling process for the terminal on which it is running**. **Each
job (a simple command or a pipeline) executed by the shell is created as a separate
process group, and the shell provides commands to move a job between three
states: running in the foreground, running in the background, and stopped in the
background.**

To support job control, the terminal driver **maintains** a record of the foreground process group (job) for the controlling terminal.
The terminal driver **delivers** job-control signals to the foreground job when certain characters are typed. These
signals either terminate or stop the foreground job.

**The notion of the terminal’s foreground job is also used to arbitrate terminal
I/O requests.** Only processes in the foreground job may read from the controlling
terminal. Background jobs are prevented from reading by delivery of the `SIGTTIN`
signal, whose default action is to stop the job. If the terminal `TOSTOP` is set, then
background jobs are also prevented from writing to the controlling terminal by
delivery of a `SIGTTOU` signal, whose default action is to stop the job.

When a **terminal disconnect** occurs, the kernel delivers a `SIGHUP` signal to the
controlling process to inform it of the fact. Such an event may result in a chain reaction
whereby a `SIGHUP` signal is delivered to many other processes. First, if the controlling
process is a shell (as is typically the case), then, before terminating, the shell sends
`SIGHUP` to each of the process groups it has created. Second, if delivery of `SIGHUP` results
in termination of a controlling process, then the kernel also sends `SIGHUP` to all of the
members of the foreground process group of the controlling terminal.

In general, applications don’t need to be cognizant of job-control signals. One
exception is when a program performs screen-handling operations. Such programs
need to correctly handle the SIGTSTP signal, resetting terminal attributes to sane
values before the process is suspended, and restoring the correct (applicationspecific) terminal attributes when the application is once more resumed following
delivery of a `SIGCONT` signal.

A process group is considered to be **orphaned** if none of its member processes
has a parent in a different process group in the same session. **Orphaned process
groups** are significant because there is no process outside the group that can both
monitor the state of any stopped processes within the group and is always allowed
to send a SIGCONT signal to these stopped processes in order to restart them. This
could result in such stopped processes languishing forever on the system. **To avoid
this possibility, when a process group with stopped member processes becomes
orphaned, all members of the process group are sent a SIGHUP signal, followed by a
SIGCONT signal, to notify them that they have become orphaned and ensure that they
are restarted.**

## 补充资料
- [GNU](https://www.gnu.org/software/libc/manual/html_node/Job-Control.html#Job-Control)

So you can probably ignore the material in this chapter unless you are writing a shell or login program.



- [build your own shell](https://github.com/tokenrove/build-your-own-shell)
> 应该是最好的实战教程了吧!

- [](https://unix.stackexchange.com/questions/404555/what-is-the-purpose-of-the-controlling-terminal/404576#404576?newreg=d0984d34a8794abba7fac597ff4622c2)

## KeyNote
