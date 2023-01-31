
static inline __attribute__((__gnu_inline__)) __attribute__((__unused__))
__attribute__((no_instrument_function)) __attribute__((__always_inline__)) void
queued_spin_lock(struct qspinlock *lock) {
  int val = 0;

  if (__builtin_expect(
          !!(atomic_try_cmpxchg_acquire(&lock->val, &val, (1U << 0))), 1))
    return;

  queued_spin_lock_slowpath(lock, val);
}

static inline __attribute__((__gnu_inline__)) __attribute__((__unused__))
__attribute__((no_instrument_function)) void
queued_spin_lock_slowpath(struct qspinlock *lock, u32 val) {
  pv_queued_spin_lock_slowpath(lock, val);
}

struct pv_lock_ops {
 void (*queued_spin_lock_slowpath)(struct qspinlock *lock, u32 val);
 struct paravirt_callee_save queued_spin_unlock;

 void (*wait)(u8 *ptr, u8 val);
 void (*kick)(int cpu);

 struct paravirt_callee_save vcpu_is_preempted;
} ;

static inline __attribute__((__gnu_inline__)) __attribute__((__unused__))
__attribute__((no_instrument_function)) __attribute__((__always_inline__)) void
pv_queued_spin_lock_slowpath(struct qspinlock *lock, u32 val) {
  (void)({
    unsigned long __edi = __edi, __esi = __esi, __edx = __edx, __ecx = __ecx,
                  __eax = __eax;
    ;
    do {
      if (__builtin_expect(
              !!(pv_ops.lock.queued_spin_lock_slowpath == ((void *)0)), 0))
        do {
          do {
          } while (0);
          do {
            asm __inline volatile("1:\t"
                                  ".byte 0x0f, 0x0b"
                                  "\n"
                                  ".pushsection __bug_table,\"aw\"\n"
                                  "2:\t"
                                  ".long "
                                  "1b"
                                  " - ."
                                  "\t# bug_entry::bug_addr\n"
                                  "\t"
                                  ".long "
                                  "%c0"
                                  " - ."
                                  "\t# bug_entry::file\n"
                                  "\t.word %c1"
                                  "\t# bug_entry::line\n"
                                  "\t.word %c2"
                                  "\t# bug_entry::flags\n"
                                  "\t.org 2b+%c3\n"
                                  ".popsection\n"
                                  ""
                                  :
                                  : "i"("arch/x86/include/asm/paravirt.h"),
                                    "i"(591), "i"(0),
                                    "i"(sizeof(struct bug_entry)));
          } while (0);
          __builtin_unreachable();
        } while (0);
    } while (0);
    asm volatile("771:\n\t"
                 "999:\n\t"
                 ".pushsection .discard.retpoline_safe\n\t"
                 " "
                 ".quad"
                 " "
                 " 999b\n\t"
                 ".popsection\n\t"
                 "call *%[paravirt_opptr];"
                 "\n"
                 "772:\n"
                 ".pushsection .parainstructions,\"a\"\n"
                 " "
                 ".balign 8"
                 " "
                 "\n"
                 " "
                 ".quad"
                 " "
                 " 771b\n"
                 "  .byte "
                 "%c[paravirt_typenum]"
                 "\n"
                 "  .byte 772b-771b\n"
                 " "
                 ".balign 8"
                 " "
                 "\n"
                 ".popsection\n"
                 : "=D"(__edi), "=S"(__esi), "=d"(__edx), "=c"(__ecx),
                   "+r"(current_stack_pointer)
                 : [paravirt_typenum] "i"(
                       (__builtin_offsetof(struct paravirt_patch_template,
                                           lock.queued_spin_lock_slowpath) /
                        sizeof(void *))),
                   [paravirt_opptr] "m"(pv_ops.lock.queued_spin_lock_slowpath),
                   "D"((unsigned long)(lock)), "S"((unsigned long)(val))
                 : "memory", "cc", "rax", "r8", "r9", "r10", "r11");
    ;
  });
}
