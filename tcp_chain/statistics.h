#include <stdio.h>

#define RESULT_LINE	"-------------------------------------------------------------\n"
#define RESULT_FMT	"#byte iterations   t_min[nsec]   t_max[nsec]  t_typical[nsec]\n"
#define REPORT_FMT	"%5d%5d         %8ld      %8ld       %8ld\n"

static int compare(const void* ptr0, const void* ptr1) {
	const long *a = ptr0;
	const long *b = ptr1;
	if (*a > *b) return 1;
	else if (*a < *b) return -1;
	else return 0;
}

void print_report_lat(int size, int iters, long *rec) {
	qsort(rec, iters, sizeof(long), compare);
	printf(REPORT_FMT, size, iters, rec[0], rec[iters-2], rec[(iters-2)>>1]);
}
