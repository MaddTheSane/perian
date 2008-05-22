#include <stdio.h>
#include <ctype.h>
#include <math.h>

static unsigned l1c[256] = {0}, l2c[256] = {0};
static unsigned long long l1ct=0, l2ct=0;
static unsigned l1_change[256][256] = {0}, l2_change[256][256] = {0};
static unsigned long long l1cht=0, l2cht=0;

static int usable(unsigned char c)
{
	return isalpha(c) || (c == ' ') || (c >= 0x80);
}

static void count(FILE *f, unsigned one_d[256], unsigned long long *t1, unsigned two_d[256][256], unsigned long long *t2)
{
	int last_c = 0, c = 0;
	
	while ((c = fgetc(f)) != EOF) {
		if (usable(c)) {
			one_d[c]++;
			(*t1)++;
		}
		
		if (usable(last_c) || usable(c)) {
			two_d[last_c][c]++;
			(*t2)++;
		}
		
		last_c = c;
	}
}

static int diff(unsigned fr1_, unsigned long long fr1t, unsigned fr2_, unsigned long long fr2t)
{
	double fr1 = ((double)fr1_) / ((double)fr1t),
		   fr2 = ((double)fr2_) / ((double)fr2t);
	
	double diff = sqrt(fr2) - sqrt(fr1);
	
	return diff * 65536. + .5;
}

int main (int argc, const char * argv[]) {
    if (argc < 3) return 1;
	FILE *t1 = fopen(argv[1], "rb"), *t2 = fopen(argv[2], "rb");
	FILE *data = fopen("chardet.h", "w");
	int i, j;
	
	count(t1, l1c, &l1ct, l1_change, &l1cht);
	count(t2, l2c, &l2ct, l2_change, &l2cht);
	
	fprintf(data, "static const short frequencies[] = {");
	
	for (i = 0; i < 256; i++) {
		if (!(i % 16)) fprintf(data,"\n\t");
		fprintf(data, "%d", diff(l1c[i], l1ct, l2c[i], l2ct));
		if (i < 255) fprintf(data, ", ");
	}
	
	fprintf(data, "};\n\n");
	
	fprintf(data, "static const short transitions[256][256] = {\n");
	
	for (i = 0; i < 256; i++) {
		fprintf(data,"\t{ ");
		for (j = 0; j < 256; j++) {
			if (j && !(j % 16)) fprintf(data, "\n\t");
			fprintf(data, "%d", diff(l1_change[i][j], l1cht, l2_change[i][j], l2cht));
			if (j < 255) fprintf(data, ",");
			fprintf(data, " ");
		}
		fprintf(data, "}");
		if (i < 255) fprintf(data, ",\n");
	}
	
	fprintf(data, "};\n");
	
    return 0;
}
