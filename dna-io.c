#include <zlib.h>
#include <stdio.h>
#include "dna-io.h"
#include "kseq.h"
KSEQ_INIT2(, gzFile, gzread)

unsigned char seq_nt4_table[256] = {
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4 /*'-'*/, 4, 4,
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 0, 4, 1,  4, 4, 4, 2,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  3, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 0, 4, 1,  4, 4, 4, 2,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  3, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4
};

void dn_seq2vec_ds(int l, const uint8_t *seq4, float *x)
{
	int i, c;
	uint8_t *rev4;
	rev4 = (uint8_t*)malloc(l);
	for (i = 0; i < l; ++i) rev4[l-i-1] = seq4[i] > 3? 4 : 3 - seq4[i];
	memset(x, 0, 8 * l * sizeof(float));
	for (c = 0; c < 4; ++c) {
		float *x1 = &x[c * l];
		for (i = 0; i < l; ++i)
			if (seq4[i] == c) x1[i] = 1.0f;
		x1 = &x[(c + 4) * l];
		for (i = 0; i < l; ++i)
			if (rev4[i] == c) x1[i] = 1.0f;
	}
	free(rev4);
}

int dn_select_seq(const dn_seqs_t *tr, double r)
{
	double x = tr->sum_len[tr->n - 1] * r;
	int st = 0, en = tr->n - 1;
	if (tr->n <= 0) return -1;
	while (st < en) {
		int mid = st + ((en - st) >> 1);
		if (tr->sum_len[mid] < x) st = mid + 1;
		else if (mid == 0) return 0;
		else if (x >= tr->sum_len[mid - 1]) return mid;
		else en = mid - 1;
	}
	return st;
}

dn_seqs_t *dn_read(const char *fn)
{
	gzFile fp;
	kseq_t *ks;
	dn_seqs_t *s;
	int i, max_lbl = 0;

	fp = fn && strcmp(fn, "-")? gzopen(fn, "r") : gzdopen(fileno(stdin), "r");
	ks = kseq_init(fp);
	s = (dn_seqs_t*)calloc(1, sizeof(dn_seqs_t));
	while (kseq_read(ks) >= 0) {
		for (i = 0; i < ks->seq.l; ++i) {
			ks->seq.s[i] = seq_nt4_table[(int)ks->seq.s[i]];
			if (ks->qual.l == ks->seq.l)
				ks->qual.s[i] = ks->qual.s[i] - 33;
		}
		if (s->n == s->m) {
			s->m = s->m? s->m<<1 : 16;
			s->len = (int*)realloc(s->len, s->m * sizeof(int));
			s->sum_len = (uint64_t*)realloc(s->sum_len, s->m * 8);
			s->seq = (uint8_t**)realloc(s->seq, s->m * sizeof(uint8_t*));
			s->lbl = (uint8_t**)realloc(s->lbl, s->m * sizeof(uint8_t*));
		}
		s->sum_len[s->n] = s->n == 0? ks->seq.l : s->sum_len[s->n - 1] + ks->seq.l;
		s->len[s->n] = ks->seq.l;
		s->seq[s->n] = (uint8_t*)malloc(ks->seq.l);
		memcpy(s->seq[s->n], ks->seq.s, ks->seq.l);
		if (ks->qual.l == ks->seq.l) {
			s->lbl[s->n] = (uint8_t*)malloc(ks->qual.l);
			memcpy(s->lbl[s->n], ks->qual.s, ks->qual.l);
			for (i = 0; i < ks->qual.l; ++i)
				max_lbl = max_lbl > ks->qual.s[i]? max_lbl : ks->qual.s[i];
		}
		++s->n;
	}
	s->n_lbl = max_lbl + 1;
	kseq_destroy(ks);
	gzclose(fp);
	return s;
}

void dn_destroy(dn_seqs_t *s)
{
	int i;
	for (i = 0; i < s->n; ++i) {
		free(s->seq[i]);
		if (s->lbl) free(s->lbl[i]);
	}
	free(s->seq); free(s->lbl);
	free(s->len); free(s->sum_len);
	free(s);
}

int dn_bseq_read(void *ks_, dn_bseq_t *bs, int max)
{
	int n_bases = 0, ret;
	kseq_t *ks = (kseq_t*)ks_;
	while ((ret = kseq_read(ks)) >= 0) {
		if (bs->n == bs->m) {
			bs->m = bs->m? bs->m + (bs->m>>1) : 16;
			bs->a = (dn_bseq1_t*)realloc(bs->a, bs->m * sizeof(dn_bseq1_t));
		}
		bs->a[bs->n].seq  = strdup(ks->seq.s);
		bs->a[bs->n].qual = ks->qual.l > 0? strdup(ks->qual.s) : 0;
		bs->a[bs->n].name = strdup(ks->name.s);
		bs->a[bs->n].len  = ks->seq.l;
		bs->a[bs->n++].lbl  = 0;
		n_bases += ks->seq.l;
		if (n_bases >= max) break;
	}
	return ret >= -1? n_bases : ret;
}

void dn_bseq_reset(dn_bseq_t *bs)
{
	int i;
	for (i = 0; i < bs->n; ++i) {
		free(bs->a[i].seq);
		free(bs->a[i].qual);
		free(bs->a[i].name);
		free(bs->a[i].lbl);
	}
	free(bs->a);
	bs->n = bs->m = 0, bs->a = 0;
}
