#!/usr/bin/env bash
set -E -e -u -o pipefail
cd "$(dirname "$0")"
set -x

remove=false
while getopts "hr" opt; do
	case $opt in
		r) remove=true ;;
		h) echo "help" ;;
		*)
			exit 1
			;;
	esac
done
shift $((OPTIND - 1))

testcase=${1:-}
if [[ -z $testcase ]]; then
	echo "./ktest.sh testcase"
	echo "./ktest.sh -r testcase"
	exit 0
fi
upper_testcase=$(echo "$testcase" | tr '[:lower:]' '[:upper:]')

function create_tempalte_file() {

	if [[ $need_resource == true ]]; then
		cat <<_EOF_ >"$testcase.c"
#include "internal.h"
int test_${testcase}_init(void)
{
	return 0;
}

int test_${testcase}_exit(void)
{
	return 0;
}

int test_${testcase}(long action)
{
	switch (action) {
	case 0:
		break;
	}
	return 0;
}
_EOF_
	else
		cat <<_EOF_ >>"$testcase.c"
#include "internal.h"
int test_${testcase}(long action)
{
	switch (action) {
	case 0:
		break;
	}
	return 0;
}
_EOF_
	fi

}

main=main.c
function add_sysfs_entry() {
	sed -i -E "/DECLARE_TESTER\(watchdog\)/a #ifdef CONFIG_TEST_$upper_testcase\n\tDECLARE_TESTER\($testcase\)\n#endif" internal.h
	if [[ $need_resource == true ]]; then
		sed -i -E "/start of definition/i #ifdef CONFIG_TEST_$upper_testcase\n\tDEFINE_TESTER_RESOURCE\($testcase\)\n#endif" ${main}
	else
		sed -i -E "/start of definition/i #ifdef CONFIG_TEST_$upper_testcase\n\tDEFINE_TESTER\($testcase\)\n#endif" ${main}
	fi
	sed -i "/static struct attribute /a #ifdef CONFIG_TEST_$upper_testcase\n\t&${testcase}_attribute.attr,\n#endif" ${main}
	sed -i -E "/#define CONFIG_H_PB2UMYTB/a #define CONFIG_TEST_$upper_testcase 1" config.h
}

function remove_sysfs_entry() {
	sed -i -E "/\($testcase\)/d" internal.h
	sed -i -E "/^#ifdef CONFIG_TEST_$upper_testcase$/,/^#endif$/d" $main
	sed -i -E "/CONFIG_TEST_$upper_testcase/d" config.h
}

function create_user() {

	cat <<_EOF_ >>"$testcase-user.c"
#include <stdio.h>
#include <stdlib.h>
#include "user/lib.h"
#include "user/sysfs.h"

int main(int argc, char *argv[])
{
	int test = atoi(argv[1]);
	switch (test) {
	case 0:
		break;
	}
	return 0;
}

_EOF_

}

if [[ $remove == true ]]; then
	remove_sysfs_entry
	rm -f "$testcase.c"
	rm -f "$testcase-user.c"
else
	need_resource=false

	if gum confirm --default=No "need resource ?"; then
		need_resource=true
	fi

	if gum confirm --default=No "need user program ?"; then
		create_user
	fi

	create_tempalte_file
	add_sysfs_entry
fi

# 刷新 Makefile 中构建的 object
./config.sh
