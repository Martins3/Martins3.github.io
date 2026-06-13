/*
 * binomial_test.c: Compute number of test runs required for confidence.
 *
 * Based on Section 11.5.1 "Statistics for Discrete Testing" from perfbook.
 *
 * Formula: n = log(1 - F_n) / log(1 - f)
 *   f  = probability of failure in a single test run
 *   F_n = desired probability of at least one failure in n runs
 *   n  = number of test runs required
 *
 * To be 99%% confident that a fix helped, we run n tests with the fix.
 * If none fail, there is only 1%% chance this is due to dumb luck.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(int argc, char *argv[])
{
	double f;        /* single-test failure probability */
	double F_n;      /* desired cumulative failure probability */
	double n;        /* required number of test runs */

	if (argc > 1)
		f = atof(argv[1]);
	else
		f = 0.1;

	if (argc > 2)
		F_n = atof(argv[2]);
	else
		F_n = 0.99;

	if (f <= 0.0 || f >= 1.0) {
		fprintf(stderr, "failure rate f must be in (0, 1)\n");
		return 1;
	}
	if (F_n <= 0.0 || F_n >= 1.0) {
		fprintf(stderr, "confidence F_n must be in (0, 1)\n");
		return 1;
	}

	n = log(1.0 - F_n) / log(1.0 - f);

	printf("Single-test failure rate: %.4f (%.2f%%)\n", f, f * 100.0);
	printf("Desired confidence:       %.4f (%.2f%%)\n", F_n, F_n * 100.0);
	printf("Required test runs:       %.1f (round up to %d)\n",
	       n, (int)ceil(n));

	/* Print a table for common values */
	printf("\n--- Table: Required runs for 99%% confidence ---\n");
	printf("Failure Rate  Required Runs\n");
	printf("------------  -------------\n");
	double rates[] = { 0.5, 0.3, 0.1, 0.05, 0.03, 0.01, 0.005, 0.001 };
	int i;
	for (i = 0; i < (int)(sizeof(rates) / sizeof(rates[0])); i++) {
		double r = rates[i];
		double needed = log(1.0 - 0.99) / log(1.0 - r);
		printf("  %.3f         %d\n", r, (int)ceil(needed));
	}

	return 0;
}
