# Linux Programming Interface: Chapter 20

# 20 Gignals: Fundamental Concepts
> P389 : 不知道如何使用ps 实现查询: The same information can also be obtained using various options to the ps(1) command.




If pid equals –1, the signal is sent to every process for which the calling process
has permission to send a signal, except init (process ID 1) and the calling process. If a *privileged process* makes this call, then all processes on the system will
be signaled, except for these last two. For obvious reasons, signals sent in this
way are sometimes called broadcast signals. (SUSv3 doesn’t require that the calling process be excluded from receiving the signal; Linux follows the BSD
semantics in this regard.)

An unprivileged process can send a signal to another process if the real or
effective user ID of the sending process matches the *real user ID* or *saved set-user-ID* of the receiving process, as shown in Figure 20-2. This rule allows users
to send signals to set-user-ID programs that they have started, regardless of the
current setting of the target process’s effective user ID. Excluding the effective
user ID of the target from the check serves a complementary purpose: it prevents
one user from sending signals to another user’s process that is running a setuser-ID program belonging to the user trying to send the signal. (SUSv3 mandates
the rules shown in Figure 20-2, but Linux followed slightly different rules in
kernel versions before 2.0, as described in the kill(2) manual page.)
# Linux Programming Interface: Chapter 20
