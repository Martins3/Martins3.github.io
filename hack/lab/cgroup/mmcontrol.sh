#!/usr/bin/env bash

cgcreate -g memory:mem
watch --interval 1 cgget -g memory:mem

# stress --vm-bytes 150M --vm-keep -m 1
