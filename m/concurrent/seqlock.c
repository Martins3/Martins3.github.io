#include "internal.h"
#include <linux/seqlock.h>

/*
 * seqlock 适用场景: 写者很少，读者很多，读者绝对不可以阻塞写者。
 * 读者通过 sequence 号检查发现读到了撕裂的数据，然后重试。
 *
 * 这里的 demo 维护一个不变量: data_a == data_b 永远成立，
 * 读者每次读到一致的数据都验证这个不变量。
 */
static DEFINE_SEQLOCK(seqlock_demo);

static long data_a;
static long data_b;
static bool writer_done;

#define SEQLOCK_WRITER_LOOP 1000000

static void seqlock_worker(struct work_struct *work)
{
	struct work *test = (struct work *)work;

	if (test->id == 0) {
		/* writer */
		ktime_t start = ktime_get();
		for (long i = 0; i < SEQLOCK_WRITER_LOOP; i++) {
			write_seqlock(&seqlock_demo);
			data_a++;
			data_b++;
			write_sequnlock(&seqlock_demo);
		}
		writer_done = true;
		pr_info("writer cost %lld ns\n", ktime_get() - start);
	} else {
		/* reader : 读到的数据要么是一致的，要么重试，永远不会阻塞 writer */
		unsigned int seq;
		long a, b;
		long reads = 0, retries = 0;

		while (!writer_done) {
			do {
				seq = read_seqbegin(&seqlock_demo);
				a = data_a;
				b = data_b;
				retries++;
			} while (read_seqretry(&seqlock_demo, seq));

			if (a != b)
				pr_err("reader %d saw torn data: a=%ld b=%ld\n",
				       test->id, a, b);
			reads++;
		}
		pr_info("reader %d : %ld consistent reads, %ld retries\n",
			test->id, reads, retries - reads);
	}
}

int test_seqlock(long action)
{
	switch (action) {
	case 0:
		/* 1 个 writer + 3 个 reader */
		data_a = 0;
		data_b = 0;
		writer_done = false;
		batch_queue_works(seqlock_worker, 4, sizeof(struct work));
		pr_info("final : data_a=%ld data_b=%ld (expect %d)\n", data_a,
			data_b, SEQLOCK_WRITER_LOOP);
		break;
	}
	return 0;
}
