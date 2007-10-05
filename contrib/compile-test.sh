#!/bin/bash
# Try to compile eix with various version of gcc.

shopt -s extglob

get_avail_gcc() {
	echo /etc/env.d/gcc/!(*-hardened*|!(i?86-pc-linux-gnu-*)) \
	| xargs -n1 basename
}

reset_gcc() {
	gcc-config "${original_gcc}" >/dev/null &
	exit
}

set_status() {
	gcc_faild[$1]="$2"
}

gcc=($(get_avail_gcc))
gcc_faild=(${gcc[@]})
original_gcc=$(gcc-config -c)
trap reset_gcc EXIT INT


for ((gcc_n=0;gcc_n<"${#gcc[@]}";++gcc_n)); do
	make maintainer-clean &>/dev/null
	echo "trying ${gcc[$gcc_n]}"

	if ! gcc-config "${gcc[$gcc_n]}"; then
		set_status $gcc_n "(1 of 5) gcc-config failed"
		continue
	fi

	source /etc/profile

	if ! ./autogen.sh; then
		set_status $gcc_n "(2 of 5) autogen.sh failed"
		continue
	fi

	if ! ./configure CXXFLAGS="-Wall -W ${CXXFLAGS:- -g3 -ggdb3}"; then
		set_status $gcc_n "(3 of 5) configure failed"
		continue
	fi

	if ! make clean all; then
		set_status $gcc_n "(4 of 5) make clean all"
		continue
	fi

	if ! make check; then
		set_status $gcc_n "(5 of 5) make check"
		continue
	fi

	set_status $gcc_n "ok"
done

echo
echo "== all done =="
echo

for ((gcc_n=0;gcc_n<"${#gcc[@]}";++gcc_n)); do
	echo "${gcc[$gcc_n]}: ${gcc_faild[$gcc_n]}"
done
