#include <stdio.h>

int main(int argc, char** argv) {
	printf("Huhu\n");
	for (int i = 0; i < argc; i++) {
		printf("\t%s\n", argv[i]);
	}
	return 0;
}
