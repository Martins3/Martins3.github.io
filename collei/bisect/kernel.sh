#!/usr/bin/env bash
set -E -e -u -o pipefail

set -x

function notify() {
	notify-send "$*"
}

function setup_scripts_for_docker() {
	local main_repo
	pushd "$(readlink -m "$(dirname "$0")")"
	main_repo=$(git rev-parse --show-toplevel)
	popd

	cp "$main_repo"/code/build/build.tar.sh .
	cp "$main_repo"/code/build/run.sh .
	cat <<_EOF_ >./run.sh
#!/usr/bin/env bash
set -E -e -u -o pipefail
set -e
make olddefconfig
if [[ \$(whoami) == root ]]; then
  git config --global --add safe.directory /root
fi
scmversion=\$(git rev-parse --short HEAD)
sed -i "s/CONFIG_LOCALVERSION=.*/CONFIG_LOCALVERSION=\"\$scmversion\"/" .config
make -j128
./build.tar.sh
_EOF_
	chmod +x run.sh
}

function wait_for_build() {
	ready_file=./.ready.txt
	WAIT=600
	for ((i = 0; i < WAIT; i = i + 1)); do
		printf '%s\n' "$i"
		sleep 1
		if [[ -f $ready_file ]]; then
			f=$(cat $ready_file)
			rm -f $ready_file
			break
		fi
	done
	echo $i
	if [[ $i == "$WAIT" ]]; then
		echo "build failed , please check"
		exit 1
	fi
}

function install_driver() {
	scp "$f" $user@$ip:
	dir=${f%%.tar.gz}
	# TODO 如果是 kexec 可以么?
	cmd="tar -xvf $f && cd install-$dir && ./run.sh && sudo reboot"
	# 由于 reboot ， 这里必然会失败，
	set +e
	# shellcheck disable=2029
	ssh $user@$ip "$cmd"
	set -e
}

# 测试虚拟机需要去掉 user 的密码
ip=172.20.251.143
user=martins3
cd /home/martins3/data/kernel/linux

setup_scripts_for_docker
wait_for_build
install_driver
sleep 60
while true; do
	if /usr/sbin/ping -c 1 "$ip"; then
		echo "$(date '+%Y-%m-%d %H:%M:%S') 服务器已响应，重启完成！"
		# 使用完整的 shell 登录，不然环境变量有问题
		ssh $user@$ip "bash -l -c './guest.sh'"
		break
	else
		sleep 1
	fi
done

notify "ready"
