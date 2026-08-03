/*
 * AI 生成的，有待优化
 */
#include "internal.h"
#include <linux/bitmap.h>
#include <linux/kernel.h>
#include <linux/slab.h>

int test_bitmap_test_init(void)
{
	return 0;
}

int test_bitmap_test_exit(void)
{
	return 0;
}

int test_bitmap_test(long action)
{
	unsigned long *bitmap;
	DECLARE_BITMAP(test_bm, 128);
	int i, result = 0;

	switch (action) {
	case 0: // Basic initialization and zeroing
		printk(KERN_INFO "Testing basic bitmap operations\n");

		// Test bitmap_zero
		bitmap_zero(test_bm, 128);
		for (i = 0; i < 128; i++) {
			if (test_bit(i, test_bm)) {
				printk(KERN_ERR "bitmap_zero failed at bit %d\n", i);
				result = -1;
				break;
			}
		}

		// Test bitmap_fill
		bitmap_fill(test_bm, 128);
		for (i = 0; i < 128; i++) {
			if (!test_bit(i, test_bm)) {
				printk(KERN_ERR "bitmap_fill failed at bit %d\n", i);
				result = -1;
				break;
			}
		}

		// Test bitmap_set and bitmap_clear
		bitmap_zero(test_bm, 128);
		bitmap_set(test_bm, 10, 5); // Set 5 bits starting from bit 10
		for (i = 10; i < 15; i++) {
			if (!test_bit(i, test_bm)) {
				printk(KERN_ERR "bitmap_set failed at bit %d\n", i);
				result = -1;
			}
		}
		for (i = 0; i < 10; i++) {
			if (test_bit(i, test_bm)) {
				printk(KERN_ERR "bitmap_set wrongly set bit %d\n", i);
				result = -1;
			}
		}
		for (i = 15; i < 128; i++) {
			if (test_bit(i, test_bm)) {
				printk(KERN_ERR "bitmap_set wrongly set bit %d\n", i);
				result = -1;
			}
		}

		// Test bitmap_clear
		bitmap_clear(test_bm, 12, 2); // Clear 2 bits starting from bit 12
		if (test_bit(10, test_bm) && test_bit(11, test_bm) &&
			!test_bit(12, test_bm) && !test_bit(13, test_bm) && test_bit(14, test_bm)) {
			printk(KERN_INFO "bitmap_set and bitmap_clear work correctly\n");
		} else {
			printk(KERN_ERR "bitmap_set/bitmap_clear incorrect\n");
			result = -1;
		}

		// Test bitmap allocation functions
		bitmap = bitmap_alloc(64, GFP_KERNEL);
		if (!bitmap) {
			printk(KERN_ERR "bitmap_alloc failed\n");
			result = -1;
		} else {
			bitmap_zero(bitmap, 64);

			// Test basic operations with allocated bitmap
			bitmap_set(bitmap, 5, 3); // Set 3 bits starting from bit 5
			if (bitmap_weight(bitmap, 64) == 3) {
				printk(KERN_INFO "bitmap_alloc and bitmap_weight work correctly\n");
			} else {
				printk(KERN_ERR "bitmap_weight incorrect\n");
				result = -1;
			}

			bitmap_free(bitmap);
		}

		if (result == 0) {
			printk(KERN_INFO "Bitmap tests passed\n");
		} else {
			printk(KERN_ERR "Bitmap tests failed\n");
		}
		break;

	default:
		printk(KERN_WARNING "Unknown action %ld for bitmap test\n", action);
		break;
	}

	return result;
}
