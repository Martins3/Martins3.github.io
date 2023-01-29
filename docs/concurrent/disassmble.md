# disassable the code



## kernel low level interface


### why
```c
typedef struct {
	int counter;
} atomic_t;

#define ATOMIC_INIT(i) { (i) }
```

### `_raw_spin_lock_irqsave`
