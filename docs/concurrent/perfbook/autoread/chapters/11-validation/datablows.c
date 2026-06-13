/*
 * datablows.c: Statistical elimination of interference in benchmark data.
 *
 * Based on Listing 11.2 from perfbook Chapter 11 (Validation).
 *
 * Input: lines with x-value followed by y-values.
 * Output: x-value, avg, min, max, good_count, total_count.
 *
 * Algorithm:
 * 1. Sort y-values.
 * 2. Assume first 1/divisor of sorted values are good.
 * 3. Project max acceptable value from trusted region.
 * 4. Reject values that exceed projected max and break trend.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_VALUES 1024

static int compare_double(const void *a, const void *b)
{
	double da = *(const double *)a;
	double db = *(const double *)b;
	if (da < db) return -1;
	if (da > db) return 1;
	return 0;
}

static void process_line(const char *line, int divisor,
			 double relerr, double trendbreak)
{
	double vals[MAX_VALUES];
	double x;
	int n = 0;
	char *tok;
	char *buf;
	char *saveptr;
	int i, j, k;
	double delta, maxdelta, maxdelta1;
	double maxdiff;
	double sum = 0.0;
	double avg, min, max;
	int good;

	buf = strdup(line);
	if (!buf)
		return;

	tok = strtok_r(buf, " \t\n", &saveptr);
	if (!tok) {
		free(buf);
		return;
	}
	x = atof(tok);

	while ((tok = strtok_r(NULL, " \t\n", &saveptr)) != NULL) {
		if (n >= MAX_VALUES)
			break;
		vals[n++] = atof(tok);
	}
	free(buf);

	if (n == 0)
		return;

	qsort(vals, n, sizeof(double), compare_double);

	/* Number of trusted values */
	i = (n + divisor - 1) / divisor;
	if (i < 1)
		i = 1;
	if (i > n)
		i = n;

	delta = vals[i - 1] - vals[0];
	maxdelta = delta * divisor;
	maxdelta1 = delta + vals[i - 1] * relerr;
	if (maxdelta1 > maxdelta)
		maxdelta = maxdelta1;

	/* Attempt to accept more values */
	for (j = i; j < n; j++) {
		if (j <= 1) {
			maxdiff = vals[n - 1] - vals[0];
		} else {
			maxdiff = trendbreak * (vals[j - 1] - vals[0]) / (j - 1);
		}
		if (vals[j] - vals[0] > maxdelta &&
		    vals[j] - vals[j - 1] > maxdiff) {
			break;
		}
	}

	good = j;
	for (k = 0; k < good; k++)
		sum += vals[k];
	avg = sum / good;
	min = vals[0];
	max = vals[good - 1];

	printf("%.6f %.6f %.6f %.6f %d %d\n", x, avg, min, max, good, n);
}

int main(int argc, char *argv[])
{
	char line[65536];
	int divisor = 3;
	double relerr = 0.01;
	double trendbreak = 2.0;
	int i;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--divisor") == 0 && i + 1 < argc) {
			divisor = atoi(argv[++i]);
		} else if (strcmp(argv[i], "--relerr") == 0 && i + 1 < argc) {
			relerr = atof(argv[++i]);
		} else if (strcmp(argv[i], "--trendbreak") == 0 && i + 1 < argc) {
			trendbreak = atof(argv[++i]);
		}
	}

	while (fgets(line, sizeof(line), stdin)) {
		process_line(line, divisor, relerr, trendbreak);
	}

	return 0;
}
