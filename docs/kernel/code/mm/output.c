# 0 "pageflags-expand.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/nix/store/4pqv2mwdn88h7xvsm7a5zplrd8sxzvw0-glibc-2.35-163-dev/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "pageflags-expand.c"
# 99 "pageflags-expand.c"
static __always_inline bool folio_test_lru(struct folio *folio) {
  return test_bit(PG_lru, folio_flags(folio, FOLIO_PF_HEAD));
}
static __always_inline int PageLRU(struct page *page) {
  return test_bit(PG_lru, &PF_HEAD(page, 0)->flags);
}
static __always_inline void folio_set_lru(struct folio *folio) {
  set_bit(PG_lru, folio_flags(folio, FOLIO_PF_HEAD));
}
static __always_inline void SetPageLRU(struct page *page) {
  set_bit(PG_lru, &PF_HEAD(page, 1)->flags);
}
static __always_inline void folio_clear_lru(struct folio *folio) {
  clear_bit(PG_lru, folio_flags(folio, FOLIO_PF_HEAD));
}
static __always_inline void ClearPageLRU(struct page *page) {
  clear_bit(PG_lru, &PF_HEAD(page, 1)->flags);
}
static __always_inline void __folio_clear_lru(struct folio *folio) {
  __clear_bit(PG_lru, folio_flags(folio, FOLIO_PF_HEAD));
}
static __always_inline void __ClearPageLRU(struct page *page) {
  __clear_bit(PG_lru, &PF_HEAD(page, 1)->flags);
}
static __always_inline bool folio_test_clear_lru(struct folio *folio) {
  return test_and_clear_bit(PG_lru, folio_flags(folio, FOLIO_PF_HEAD));
}
static __always_inline int TestClearPageLRU(struct page *page) {
  return test_and_clear_bit(PG_lru, &PF_HEAD(page, 1)->flags);
}
