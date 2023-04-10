## copy_to_user
然后简单的分析下如何处理用户态的蛇皮指针之类的问题即可。

最后到达此处
```c
static __always_inline __must_check unsigned long
copy_user_generic(void *to, const void *from, unsigned len)
{
	unsigned ret;

	/*
	 * If CPU has ERMS feature, use copy_user_enhanced_fast_string.
	 * Otherwise, if CPU has rep_good feature, use copy_user_generic_string.
	 * Otherwise, use copy_user_generic_unrolled.
	 */
	alternative_call_2(copy_user_generic_unrolled,
			 copy_user_generic_string,
			 X86_FEATURE_REP_GOOD,
			 copy_user_enhanced_fast_string,
			 X86_FEATURE_ERMS,
			 ASM_OUTPUT2("=a" (ret), "=D" (to), "=S" (from),
				     "=d" (len)),
			 "1" (to), "2" (from), "3" (len)
			 : "memory", "rcx", "r8", "r9", "r10", "r11");
	return ret;
}
```
```c
/*
 * Some CPUs are adding enhanced REP MOVSB/STOSB instructions.
 * It's recommended to use enhanced REP MOVSB/STOSB if it's enabled.
 *
 * Input:
 * rdi destination
 * rsi source
 * rdx count
 *
 * Output:
 * eax uncopied bytes or 0 if successful.
 */
SYM_FUNC_START(copy_user_enhanced_fast_string)
	ASM_STAC
	/* CPUs without FSRM should avoid rep movsb for short copies */
	ALTERNATIVE "cmpl $64, %edx; jb copy_user_short_string", "", X86_FEATURE_FSRM
	movl %edx,%ecx
1:	rep movsb
	xorl %eax,%eax
	ASM_CLAC
	RET

12:	movl %ecx,%edx		/* ecx is zerorest also */
	jmp .Lcopy_user_handle_tail

	_ASM_EXTABLE_CPY(1b, 12b)
SYM_FUNC_END(copy_user_enhanced_fast_string)
```

## fsrm
