#define NEXT w = *ip++; goto **(void **) w
#define CELL_LEN(n) (((n) + sizeof(cell_t) - 1) / sizeof(cell_t))
#define FIND(name) find(name, sizeof(name) - 1)
#define LOWER(ch) ((ch) & 95)

static struct {
  const char *tib;
  cell_t ntib, tin, state, base;
  cell_t *heap, *last, tthrow;
  cell_t DOLIT_XT, DOEXIT_XT;
} g_sys;

static cell_t convert(const char *pos, cell_t n, cell_t *ret) {
  *ret = 0;
  cell_t negate = 0;
  cell_t base = g_sys.base;
  if (!n) { return 0; }
  if (pos[0] == '$') { base = 16; ++pos; --n; }
  if (pos[0] == '-') { negate = -1; ++pos; --n; }
  for (; n; --n) {
    uintptr_t d = pos[0] - '0';
    if (d > 9) {
      d = LOWER(d) - 7;
      if (d < 10) { return 0; }
    }
    if (d >= (uintptr_t) g_sys.base) { return 0; }
    *ret = *ret * base + d;
    ++pos;
  }
  if (negate) { *ret = -*ret; }
  return -1;
}

static cell_t same(const char *a, const char *b, cell_t len) {
  for (;len && LOWER(*a) == LOWER(*b); --len, ++a, ++b);
  return len;
}

static cell_t find(const char *name, cell_t len) {
  cell_t *pos = g_sys.last;
  cell_t clen = CELL_LEN(len);
  while (pos) {
    if (len == pos[-3] &&
        same(name, (const char *) &pos[-3 - clen], len) == 0) {
      return (cell_t) pos;
    }
    pos = (cell_t *) pos[-2];  // Follow link
  }
  return 0;
}

static void create(const char *name, cell_t length, cell_t flags, void *op) {
  memcpy(g_sys.heap, name, length);  // name
  g_sys.heap += CELL_LEN(length);
  *g_sys.heap++ = length;  // length
  *g_sys.heap++ = (cell_t) g_sys.last;  // link
  *g_sys.heap++ = flags;  // flags
  g_sys.last = g_sys.heap;
  *g_sys.heap++ = (cell_t) op;  // code
}

static cell_t parse(cell_t sep, cell_t *ret) {
  while (g_sys.tin < g_sys.ntib && g_sys.tib[g_sys.tin] == sep) { ++g_sys.tin; }
  *ret = (cell_t) (g_sys.tib + g_sys.tin);
  while (g_sys.tin < g_sys.ntib && g_sys.tib[g_sys.tin] != sep) { ++g_sys.tin; }
  cell_t len = g_sys.tin - (*ret - (cell_t) g_sys.tib);
  if (g_sys.tin < g_sys.ntib) { ++g_sys.tin; }
  return len;
}

static cell_t *eval1(cell_t *sp, cell_t *call) {
  *call = 0;
  cell_t name;
  cell_t len = parse(' ', &name);
  cell_t xt = find((const char *) name, len);
  if (xt) {
    if (g_sys.state && !(((cell_t *) xt)[-1] & 1)) {  // bit 0 of flags is immediate
      *g_sys.heap++ = xt;
    } else {
      *call = xt;
    }
  } else {
    cell_t n;
    cell_t ok = convert((const char *) name, len, &n);
    if (ok) {
      if (g_sys.state) {
        *g_sys.heap++ = g_sys.DOLIT_XT;
        *g_sys.heap++ = n;
      } else {
        *++sp = n;
      }
    } else {
      //fwrite((void *) name, 1, len, stderr);
      *++sp = -1;
      *call = g_sys.tthrow;
    }
  }
  return sp;
}

static void ueforth(void *heap, const char *src, cell_t src_len) {
  g_sys.heap = (cell_t *) heap;
  register cell_t *sp = g_sys.heap; g_sys.heap += STACK_SIZE;
  register cell_t *rp = g_sys.heap; g_sys.heap += STACK_SIZE;
  register cell_t tos = 0, *ip, t, w;
  dcell_t d;
  udcell_t ud;
  cell_t tmp;
#define X(name, op, code) create(name, sizeof(name) - 1, name[0] == ';', && op);
  PLATFORM_OPCODE_LIST
  OPCODE_LIST
#undef X
  g_sys.last[-1] = 1;  // Make ; IMMEDIATE
  g_sys.DOLIT_XT = FIND("DOLIT");
  g_sys.DOEXIT_XT = FIND("EXIT");
  g_sys.tthrow = FIND("DROP");
  ip = g_sys.heap;
  *g_sys.heap++ = FIND("EVAL1");
  *g_sys.heap++ = FIND("BRANCH");
  *g_sys.heap++ = (cell_t) ip;
  g_sys.base = 10;
  g_sys.tib = src;
  g_sys.ntib = src_len;
  NEXT;
#define X(name, op, code) op: code; NEXT;
  PLATFORM_OPCODE_LIST
  OPCODE_LIST
#undef X
  OP_DOCREATE: DUP; tos = w + sizeof(cell_t) * 2; NEXT;
  OP_DODOES: DUP; tos = w + sizeof(cell_t) * 2;
             *++rp = (cell_t) ip; ip = (cell_t *) *(cell_t *) (w + sizeof(cell_t)); NEXT;
  OP_DOCOL: *++rp = (cell_t) ip; ip = (cell_t *) (w + sizeof(cell_t)); NEXT;
}
