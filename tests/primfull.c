typedef struct Fibheap Fibheap;
typedef struct Fibnode Fibnode;
typedef int (*Fibcmp)(Fibnode*, Fibnode*);

struct Fibheap {
	Fibcmp cmp;
	Fibnode *min;
	Fibnode **arr;
	int arrlen;
};

struct Fibnode {
	Fibnode *p, *c, *next, *prev;
	int rank;
	char mark;
};

#include <stdlib.h>
#include <string.h>

Fibheap*
fibinit(Fibheap *heap, Fibcmp cmp)
{
	heap->arr = calloc(10, sizeof(*heap->arr));
	if(heap->arr == NULL) {
		return NULL;
	}
	heap->arrlen = 10;

	heap->cmp = cmp;
	heap->min = NULL;

	return heap;
}

Fibheap*
fibfree(Fibheap *h)
{
	free(h->arr);
	return h;
}

static Fibnode*
concat(Fibnode *h1, Fibnode *h2)
{
	Fibnode *prev;

	if(h1 == NULL)
		return h2;
	if(h2 == NULL)
		return h1;

	h1->prev->next = h2;
	h2->prev->next = h1;

	prev = h1->prev;
	h1->prev = h2->prev;
	h2->prev = prev;
	return h1;
}

static Fibnode*
meld(Fibnode *h1, Fibnode *h2, Fibcmp cmp)
{
	if(h1 == NULL)
		return h2;
	if(h2 == NULL)
		return h1;

	concat(h1, h2);
	return cmp(h1, h2) <= 0 ? h1 : h2;
}

Fibheap*
fibmeld(Fibheap *h1, Fibheap *h2)
{
	h1->min = meld(h1->min, h2->min, h1->cmp);
	return h1;
}

static Fibnode*
initnode(Fibnode *n)
{
	n->p = NULL;
	n->c = NULL;
	n->next = n;
	n->prev = n;
	n->rank = 0;
	n->mark = 0;
	return n;
}

void
fibinsert(Fibheap *h, Fibnode *n)
{
	h->min = meld(h->min, initnode(n), h->cmp);
}

static Fibnode*
link1(Fibnode *x, Fibnode *y)
{
	x->c = concat(x->c, y);
	y->p = x;
	y->mark = 0;
	x->rank += y->rank + 1;
	return x;
}

static Fibnode*
link(Fibnode *x, Fibnode *y, Fibcmp cmp)
{
	if(cmp(x, y) <= 0)
		return link1(x, y);
	else
		return link1(y, x);
}

static int
resizearr(Fibheap *h, int rank)
{
	Fibnode **a;
	int alen;

	a = h->arr;
	alen = h->arrlen;
	h->arrlen = 2 * rank * sizeof(*h->arr);
	h->arr = calloc(h->arrlen, sizeof(*h->arr));
	if(h->arr == NULL)
		return -1;
	memcpy(h->arr, a, alen*sizeof(*h->arr));
	return 0;
}

static int
arraylink(Fibheap *h, Fibnode *n)
{
	Fibnode *m;

	for(;;) {
		if(h->arrlen <= n->rank) {
			if(resizearr(h, n->rank) == -1)
				return -1;
		}

		m = h->arr[n->rank];
		if(m == NULL) {
			h->arr[n->rank] = n;
			return n->rank;
		}
		h->arr[n->rank] = NULL;
		n = link(m, n, h->cmp);
	}
}

static int
linkheaps(Fibheap *h, Fibnode *head)
{
	Fibnode *n, *next;
	int rank, maxrank;

	memset(h->arr, 0, sizeof(*h->arr) * h->arrlen);

	maxrank = 0;
	n = head;
	do {
		next = n->next;
		n->next = n;
		n->prev = n;
		rank = arraylink(h, n);
		if(rank == -1)
			return -1;
		if(maxrank < rank)
			maxrank = rank;
		n = next;
	} while(n != head);

	return maxrank;
}

static void
meldheaps(Fibheap *h, int maxrank)
{
	Fibnode **ni;

	h->min = NULL;
	for(ni = h->arr; ni <= h->arr + maxrank; ni++) {
		if(*ni != NULL)
			h->min = meld(h->min, *ni, h->cmp);
	}
}

static int
linkstep(Fibheap *h, Fibnode *head)
{
	int maxrank;

	maxrank = linkheaps(h, head);
	if(maxrank == -1)
		return -1;
	meldheaps(h, maxrank);
	return 0;
}

static Fibnode*
removenode(Fibnode *n)
{
	Fibnode *next;

	n->p = NULL;
	if(n->next == n)
		return NULL;

	next = n->next;
	n->next->prev = n->prev;
	n->prev->next = n->next;

	n->next = n;
	n->prev = n;

	return next;
}

static Fibnode*
detachchildren(Fibnode *p)
{
	Fibnode *c;

	c = p->c;
	if(c != NULL) do {
		c->p = NULL;
		c = c->next;
	} while(c != p->c);
	p->c = NULL;
	return c;
}

int
fibdeletemin(Fibheap *h)
{
	Fibnode *head, *min;

	min = h->min;
	if(min == NULL)
		return 0;

	head = concat(removenode(min), detachchildren(min));
	if(head == NULL) {
		h->min = NULL;
		return 0;
	}

	return linkstep(h, head);
}

static void
cut(Fibheap *h, Fibnode *n)
{
	Fibnode *p;

	p = n->p;
	p->rank -= n->rank + 1;
	p->c = removenode(n);
	h->min = meld(h->min, n, h->cmp);
}

static void
cascadingcut(Fibheap *h, Fibnode *n)
{
	Fibnode *p;

Loop:
	p = n->p;
	cut(h, n);
	if(p->p == NULL)
		return;

	if(p->mark) {
		n = p;
		goto Loop;
	}

	p->mark = 1;
}

void
fibdecreasekey(Fibheap *h, Fibnode *n)
{
	if(n->p == NULL) {
		h->min = h->cmp(h->min, n) <= 0 ? h->min : n;
		return;
	}

	if(h->cmp(n->p, n) < 0)
		return;

	cascadingcut(h, n);
}

int
fibdelete(Fibheap *h, Fibnode *n)
{
	if(h->min == n)
		return fibdeletemin(h);

	if(n->p != NULL)
		cascadingcut(h, n);

	removenode(n);
	concat(h->min, detachchildren(n));
	return 0;
}

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
sysfatal(char *fmt, ...)
{
	char buf[1024];
	int w;
	va_list va;

	w = snprintf(buf, sizeof(buf), "dijkstra: ");
	va_start(va, fmt);
	w += vsnprintf(buf+w, sizeof(buf)-w, fmt, va);
	va_end(va);
	snprintf(buf+w, sizeof(buf)-w, ": %s", strerror(errno));

	fprintf(stderr, "%s\n", buf);
	exit(1);
}

typedef struct Edge Edge;
typedef struct Nodedata Nodedata;

struct Edge {
	Fibnode fibnode;
	int node, dist;
	Edge *next;
};

struct Nodedata {
	Edge *edges, **etail;
	int intree, node;
};

struct {
	Nodedata *a;
	int len;
} nodes;

Edge *edgepool;

Edge*
edgealloc(void)
{
	Edge *e;

	if(edgepool == NULL)
		return malloc(sizeof(Edge));

	e = edgepool;
	edgepool = edgepool->next;
	return e;
}

Nodedata*
nodedata(int n)
{
	return nodes.a + n-1;
}

int
edgecmp(Fibnode *a, Fibnode *b)
{
	Edge *e, *f;

	e = (Edge*)a;
	f = (Edge*)b;

	if(e->dist < f->dist)
		return -1;
	if(e->dist > f->dist)
		return 1;
	return 0;
}

void
insertedges(Fibheap *pq, Nodedata *s)
{
	Edge *e;
	Nodedata *d;

	for(e = s->edges; e != NULL; e = e->next) {
		d = nodedata(e->node);
		if(d->intree)
			continue;
		fibinsert(pq, &e->fibnode);
	}
}

int
prim(int start)
{
	Fibheap pq;
	Nodedata *n;
	Edge *e;
	int primsum;

	fibinit(&pq, edgecmp);
	n = nodedata(start);
	n->intree = 1;
	primsum = 0;
	insertedges(&pq, n);
	while(pq.min != NULL) {
		e = (Edge*)pq.min;
		if(fibdeletemin(&pq) < 0)
			sysfatal("deletion failed");
		n = nodedata(e->node);
		if(n->intree)
			continue;
		n->intree = 1;
		primsum += e->dist;
		insertedges(&pq, n);
	}
	return primsum;
}

void
addedge(int s, int d, int dist)
{
	Edge *e;
	Nodedata *nd;

	e = edgealloc();
	e->node = d;
	e->dist = dist;
	e->next = NULL;
	nd = nodedata(s);
	*nd->etail = e;
	nd->etail = &e->next;
}

void
initnodedata(Nodedata *nd, int n)
{
	*nd->etail = edgepool;
	edgepool = nd->edges;
	nd->edges = NULL;
	nd->etail = &nd->edges;
	nd->intree = 0;
	nd->node = n;
}

void
reallocnodes(int nnodes)
{
	Nodedata *ndi;

	nodes.len = 2*nnodes;
	free(nodes.a);
	nodes.a = calloc(nodes.len, sizeof(*nodes.a));
	for(ndi = nodes.a; ndi < nodes.a+nodes.len; ndi++)
		ndi->etail = &ndi->edges;
}

int
main(void)
{
	int nnodes, edges, s, d, dist, start, i;

	scanf("%d %d", &nnodes, &edges);
	if(nodes.len < nnodes)
		reallocnodes(nnodes);
	for(i = 0; i < nnodes; i++)
		initnodedata(nodes.a + i, i+1);

	while(edges-- > 0) {
		scanf("%d %d %d", &s, &d, &dist);
		addedge(s, d, dist);
		addedge(d, s, dist);
	}

	scanf("%d", &start);
	printf("%d\n", prim(start));
}