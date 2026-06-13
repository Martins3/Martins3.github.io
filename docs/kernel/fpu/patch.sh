#!/usr/bin/env bash
set -E -e -u -o pipefail

pushd ~/data/kernel/linux-build


#   0001-x86-kvm-fpu-Limit-guest-user_xfeatures-to-supported-.patch
#   0001-x86-kvm-fpu-Remove-kvm_vcpu_arch.guest_supported_xcr.patch
#   0003-KVM-x86-Reinstate-kvm_vcpu_arch.guest_supported_xcr0.patch
#   0004-KVM-x86-Always-enable-legacy-FP-SSE-in-allowed-user-.patch
#   0005-x86-fpu-Allow-caller-to-constrain-xfeatures-when-cop.patch
#   0006-KVM-x86-Constrain-guest-supported-xfeatures-only-at-.patch

commits=(
	# key fix
	# commit 988896bb6182 ("x86/kvm/fpu: Remove kvm_vcpu_arch.guest_supported_xcr0")
	988896bb6182
	# commit ad856280ddea ("x86/kvm/fpu: Limit guest user_xfeatures to supported bits of XCR0")
	ad856280ddea

	# 1 fix for fix
	a1020a25e697
	# commit a1020a25e697 ("KVM: x86: Always enable legacy FP/SSE in allowed user XFEATURES")
	ee519b3a2ae3
	# commit ee519b3a2ae3 ("KVM: x86: Reinstate kvm_vcpu_arch.guest_supported_xcr0")
	8647c52e9504
	# commit 8647c52e9504 ("KVM: x86: Constrain guest-supported xfeatures only at KVM_GET_XSAVE{2}")

	# 2 fix for fix
	# commit 18164f66e6c5 ("x86/fpu: Allow caller to constrain xfeatures when copying to uabi buffer")
	18164f66e6c5
)

for i in "${commits[@]}"; do
	# echo "$i"
	git kernel "$i"
	# PROGDIR=$(readlink -m "$(dirname "$0")")
	# git format-patch -1 "$i" -o "$PROGDIR"
done
