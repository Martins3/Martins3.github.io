#!/usr/bin/env bash
set -E -e -u -o pipefail

# 运行所有的测试
cd tests/build
sudo ./simplefs_test

# 这个有什么区别?
sudo ./simplefs_test --gtest_brief=1
