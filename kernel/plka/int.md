# https://elinux.org/images/8/8c/Zyngier.pdf

Chained interrupt controllers : 参考ldd 以及 https://stackoverflow.com/questions/34377846/what-is-chained-irq-in-linux-when-are-they-need-to-used

Generic MSIs : https://en.wikipedia.org/wiki/Message_Signaled_Interrupts

Most systems have tens, hundreds of interrupt signal, an interrupt controller allows them to be multiplexed.
> device need attension : cpu check some pin after finished one instruction
> exception : this instruction worked abnormally
> int n : go to idt , execute  number n slot : 如何这样，似乎只有int 0x80 有意义啊!

> 仲裁器 还是　multiplexed , 如何体现的 multiplexed 的形式的 ?

IRQ line 到底是什么?

