#!/usr/bin/env bash
check_dir=(docs hack)

for i in "${check_dir[@]}"; do
	if ! lint-md "$i/**/*" -c .lintmdrc.json --threads; then
		lint-md -f "$i/**/*" -c .lintmdrc.json --threads
		exit 1
	fi
done
