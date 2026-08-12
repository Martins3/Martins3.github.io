# 0 "pageflags-expand.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/nix/store/bl5b4ywrn1ls6fwc0cvsvwmr9hynr3gk-glibc-2.39-52-dev/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "pageflags-expand.c"
# 99 "pageflags-expand.c"
static __always_inline bool folio_test_swapbacked(struct folio *folio) {
  return test_bit(PG_swapbacked, folio_flags(folio, FOLIO_PF_NO_TAIL));
}
static __always_inline int PageSwapBacked(struct page *page) {
  return test_bit(PG_swapbacked, &PF_NO_TAIL(page, 0)->flags);
}
static __always_inline void folio_set_swapbacked(struct folio *folio) {
  set_bit(PG_swapbacked, folio_flags(folio, FOLIO_PF_NO_TAIL));
}
static __always_inline void SetPageSwapBacked(struct page *page) {
  set_bit(PG_swapbacked, &PF_NO_TAIL(page, 1)->flags);
}
static __always_inline void folio_clear_swapbacked(struct folio *folio) {
  clear_bit(PG_swapbacked, folio_flags(folio, FOLIO_PF_NO_TAIL));
}
static __always_inline void ClearPageSwapBacked(struct page *page) {
  clear_bit(PG_swapbacked, &PF_NO_TAIL(page, 1)->flags);
}
static __always_inline void __folio_clear_swapbacked(struct folio *folio) {
  __clear_bit(PG_swapbacked, folio_flags(folio, FOLIO_PF_NO_TAIL));
}
static __always_inline void __ClearPageSwapBacked(struct page *page) {
  __clear_bit(PG_swapbacked, &PF_NO_TAIL(page, 1)->flags);
}
static __always_inline void __folio_set_swapbacked(struct folio *folio) {
  __set_bit(PG_swapbacked, folio_flags(folio, FOLIO_PF_NO_TAIL));
}
static __always_inline void __SetPageSwapBacked(struct page *page) {
  __set_bit(PG_swapbacked, &PF_NO_TAIL(page, 1)->flags);
}
