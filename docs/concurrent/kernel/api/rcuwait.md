## rcuwait
<!-- ec2048b7-c0df-4e6f-a652-a23ec752d7e0 -->

通过 kvm 为什么使用 kvm_vcpu::wait 可以看的很清楚，
那就是，如果只有一个等待者，那么就使用 rcuwait ，其实也就是最简单的等待机制了:

如果存在多个等待者，那么这些 waiter 就放到 queue 上，也就是 wait queue 了。

(似乎有的地方也是用这种简单的同步，但是没有用 rcuwait ，是没有及时修改 api 吗?)
```txt
History:        #0
Commit:         da4ad88cab5867ee240dfd0585e9d115a8cc47db
Author:         Davidlohr Bueso <dave@stgolabs.net>
Committer:      Paolo Bonzini <pbonzini@redhat.com>
Author Date:    Fri 24 Apr 2020 01:48:37 PM CST
Committer Date: Thu 14 May 2020 12:14:56 AM CST

kvm: Replace vcpu->swait with rcuwait

The use of any sort of waitqueue (simple or regular) for
wait/waking vcpus has always been an overkill and semantically
wrong. Because this is per-vcpu (which is blocked) there is
only ever a single waiting vcpu, thus no need for any sort of
queue.

As such, make use of the rcuwait primitive, with the following
considerations:

  - rcuwait already provides the proper barriers that serialize
  concurrent waiter and waker.

  - Task wakeup is done in rcu read critical region, with a
  stable task pointer.

  - Because there is no concurrency among waiters, we need
  not worry about rcuwait_wait_event() calls corrupting
  the wait->task. As a consequence, this saves the locking
  done in swait when modifying the queue. This also applies
  to per-vcore wait for powerpc kvm-hv.

The x86 tscdeadline_latency test mentioned in 8577370fb0cb
("KVM: Use simple waitqueue for vcpu->wq") shows that, on avg,
latency is reduced by around 15-20% with this change.

Cc: Paul Mackerras <paulus@ozlabs.org>
Cc: kvmarm@lists.cs.columbia.edu
Cc: linux-mips@vger.kernel.org
Reviewed-by: Marc Zyngier <maz@kernel.org>
Signed-off-by: Davidlohr Bueso <dbueso@suse.de>
Message-Id: <20200424054837.5138-6-dave@stgolabs.net>
[Avoid extra logic changes. - Paolo]
Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
```


```txt
History:        #0
Commit:         510958e997217e39a16b47afb5a44dfa39013964
Author:         Sean Christopherson <seanjc@google.com>
Committer:      Paolo Bonzini <pbonzini@redhat.com>
Author Date:    Sat 09 Oct 2021 10:11:57 AM CST
Committer Date: Wed 08 Dec 2021 05:24:46 PM CST

KVM: Force PPC to define its own rcuwait object

Do not define/reference kvm_vcpu.wait if __KVM_HAVE_ARCH_WQP is true, and
instead force the architecture (PPC) to define its own rcuwait object.
Allowing common KVM to directly access vcpu->wait without a guard makes
it all too easy to introduce potential bugs, e.g. kvm_vcpu_block(),
kvm_vcpu_on_spin(), and async_pf_execute() all operate on vcpu->wait, not
the result of kvm_arch_vcpu_get_wait(), and so may do the wrong thing for
PPC.

Due to PPC's shenanigans with respect to callbacks and waits (it switches
to the virtual core's wait object at KVM_RUN!?!?), it's not clear whether
or not this fixes any bugs.

Signed-off-by: Sean Christopherson <seanjc@google.com>
Message-Id: <20211009021236.4122790-5-seanjc@google.com>
Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
```

但是，问题是，为什么叫做 rcuwait ，其中的 rcu 体现在什么地方?

## 资料

first commit :

```txt
commit 8f95c90ceb541a38ac16fec48c05142ef1450c25
Author: Davidlohr Bueso <dave@stgolabs.net>
Date:   Wed Jan 11 07:22:25 2017 -0800

    sched/wait, RCU: Introduce rcuwait machinery

    rcuwait provides support for (single) RCU-safe task wait/wake functionality,
    with the caveat that it must not be called after exit_notify(), such that
    we avoid racing with rcu delayed_put_task_struct callbacks, task_struct
    being rcu unaware in this context -- for which we similarly have
    task_rcu_dereference() magic, but with different return semantics, which
    can conflict with the wakeup side.

    The interfaces are quite straightforward:

      rcuwait_wait_event()
      rcuwait_wake_up()

    More details are in the comments, but it's perhaps worth mentioning at least,
    that users must provide proper serialization when waiting on a condition, and
    avoid corrupting a concurrent waiter. Also care must be taken between the task
    and the condition for when calling the wakeup -- we cannot miss wakeups. When
    porting users, this is for example, a given when using waitqueues in that
    everything is done under the q->lock. As such, it can remove sources of non
    preemptable unbounded work for realtime.
```

## kernel/locking/percpu-rwsem.c 居然使用了 rcuwait

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
