void dummy(unsigned int a, unsigned int b, unsigned int c);

void siftmain() {
	unsigned int count = 50;
	while (1) {
		count++;
		if (count > 100)
		    dummy(192, count, 42);
	}
}
