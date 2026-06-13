#!/usr/bin/env bash
set -E -e -u -o pipefail

# deepseek 生成的，没有测试过

# 检查是否以 root 权限运行
if [ "$(id -u)" != "0" ]; then
	echo "This script must be run as root" 1>&2
	exit 1
fi

# 定义变量
TEST_DIR="/test_quota"
TEST_USER="quota_test_user"
QUOTA_LIMIT="50M"  # 软限制 50MB
QUOTA_HARD="60M"   # 硬限制 60MB
DEVICE="/dev/sdb1" # 替换为实际的测试设备
MOUNT_POINT="/mnt/test"

# 1. 创建测试用户
echo "Creating test user: $TEST_USER"
useradd -m $TEST_USER
if [ $? -ne 0 ]; then
	echo "Failed to create user"
	exit 1
fi

# 2. 准备测试设备和文件系统
echo "Preparing test device and filesystem"
# 确保设备存在
if [ ! -b $DEVICE ]; then
	echo "Device $DEVICE does not exist"
	exit 1
fi

# 创建挂载点
mkdir -p $MOUNT_POINT

# 格式化设备为 ext4（如果尚未格式化）
mkfs.ext4 -F $DEVICE
if [ $? -ne 0 ]; then
	echo "Failed to format device"
	exit 1
fi

# 挂载文件系统并启用 quota
mount -o usrquota $DEVICE $MOUNT_POINT
if [ $? -ne 0 ]; then
	echo "Failed to mount device"
	exit 1
fi

# 3. 初始化 quota
echo "Initializing quota"
quotacheck -cum $MOUNT_POINT
if [ $? -ne 0 ]; then
	echo "Failed to initialize quota"
	exit 1
fi

quotaon $MOUNT_POINT
if [ $? -ne 0 ]; then
	echo "Failed to enable quota"
	exit 1
fi

# 4. 设置用户配额
echo "Setting quota for $TEST_USER"
setquota -u $TEST_USER $QUOTA_LIMIT $QUOTA_HARD 0 0 $MOUNT_POINT
if [ $? -ne 0 ]; then
	echo "Failed to set quota"
	exit 1
fi

# 5. 创建测试目录并更改权限
echo "Creating test directory"
mkdir -p $MOUNT_POINT/$TEST_DIR
chown $TEST_USER:$TEST_USER $MOUNT_POINT/$TEST_DIR

# 6. 测试配额限制
echo "Testing quota by creating files as $TEST_USER"
su - $TEST_USER -c "dd if=/dev/zero of=$MOUNT_POINT/$TEST_DIR/testfile1 bs=1M count=40"
if [ $? -eq 0 ]; then
	echo "Successfully created 40MB file (within soft limit)"
else
	echo "Failed to create 40MB file"
fi

su - $TEST_USER -c "dd if=/dev/zero of=$MOUNT_POINT/$TEST_DIR/testfile2 bs=1M count=30"
if [ $? -ne 0 ]; then
	echo "Failed to create 30MB file (exceeds hard limit)"
else
	echo "Unexpected: Created file beyond hard limit"
fi

# 7. 检查配额使用情况
echo "Checking quota usage"
repquota $MOUNT_POINT

# 8. 清理
echo "Cleaning up"
umount $MOUNT_POINT
userdel -r $TEST_USER
rm -rf $MOUNT_POINT

echo "Quota test completed"
