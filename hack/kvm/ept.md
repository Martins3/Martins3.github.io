# ept in kvm

- [ ] EPT violation or an `EPT misconfiguration` encountered during that translation.

- [ ] intel manual chapter 28


- guest page fault will not cause vmexit

- [ ] libdune has to handle page fault in guest mode ?
  - [ ] because mapping full, so we will not page fault
- [ ] dune process malloc and access them ?


  - [ ] why we need `struct pages`, now that all the pages are page table, and unable to free
