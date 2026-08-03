#include "internal.h"
#include <linux/mm.h>

static struct folio *folio = NULL;
int test_mapping_init(void)
{

	folio = folio_alloc(GFP_USER, 2);
	return !folio;
}

int test_mapping_exit(void)
{
	folio_put(folio);
	return 0;
}

int test_mapping(long action)
{
	switch (action) {
	case 0:
		break;
	}
	return 0;
}
