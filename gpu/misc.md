# GPU 压测

git clone https://github.com/wilicc/gpu-burn

很小的一个程序，但是在 nix 上运行需要
```diff
diff --git a/Makefile b/Makefile
index 8235e8508f69..885d23649076 100644
--- a/Makefile
+++ b/Makefile
@@ -8,6 +8,20 @@ IS_JETSON   ?= $(shell if grep -Fwq "Jetson" /proc/device-tree/model 2>/dev/null
 NVCC        :=  ${CUDAPATH}/bin/nvcc
 CCPATH      ?=

+# Compiler selection:
+# GPP is used for compiling .cpp to .o.
+# HOST_GPP is used for the final link to avoid Nix glibc dependencies.
+GPP         := g++
+ifeq ($(shell which g++ 2>/dev/null | grep -qE '(/nix/|\.nix-profile)' ; echo $$?),0)
+    HOST_GPP := $(shell test -x /usr/bin/g++ && echo /usr/bin/g++ || echo /usr/local/bin/g++)
+    ifeq ($(wildcard $(HOST_GPP)),)
+        $(warning Nix g++ detected but no host g++ found; linking may produce Nix-dependent binary)
+        HOST_GPP := $(GPP)
+    endif
+else
+    HOST_GPP := $(GPP)
+endif
+
 override CFLAGS   ?=
 override CFLAGS   += -O3
 override CFLAGS   += -Wno-unused-result
@@ -32,6 +46,7 @@ IMAGE_DISTRO ?= ubi8

 override NVCCFLAGS ?=
 override NVCCFLAGS += -I${CUDAPATH}/include
+override NVCCFLAGS += -allow-unsupported-compiler
 ifneq ($(strip $(COMPUTE)),)
 override NVCCFLAGS += -arch=compute_$(subst .,,${COMPUTE})
 endif
@@ -41,10 +56,10 @@ IMAGE_NAME ?= gpu-burn
 .PHONY: clean

 gpu_burn: gpu_burn-drv.o compare.fatbin
-	g++ -o $@ $< -O3 ${LDFLAGS}
+	$(HOST_GPP) -o $@ $< -O3 ${LDFLAGS}

 %.o: %.cpp
-	g++ ${CFLAGS} -c $<
+	$(GPP) ${CFLAGS} -c $<

 %.fatbin: %.cu
 	PATH="${PATH}:${CCPATH}:." ${NVCC} ${NVCCFLAGS} -fatbin $< -o $@
```
