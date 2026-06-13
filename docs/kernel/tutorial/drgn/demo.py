#!/usr/bin/env drgn
from drgn.helpers.linux import list_for_each_entry
for mod in list_for_each_entry("struct module", prog["modules"].address_of_(), "list"):
    if mod.refcnt.counter > 1:
        print(mod.name)
