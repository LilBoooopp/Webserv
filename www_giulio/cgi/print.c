#include "stdio.h"
#include "unistd.h"

#include <stdio.h>
#include <stdlib.h>

int main(void) {
	const char *qs = getenv("QUERY_STRING");
	if (!qs)
		qs = "";

	printf("Content-Type: text/html\r\n\r\n");
	printf("<html><body>\n");
	printf("<p>QUERY_STRING = '%s'</p>\n", qs);
	printf("</body></html>\n");
	return 0;
}
