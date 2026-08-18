让 Qwen 写的 demo ，但是运行会出错，在
```txt
freeglut (./triangle):  ERROR:  Internal error <FBConfig with necessary capabilities not found> in function fgOpenWindow
make: *** [Makefile:3: all] Error 1
```

这里链接了 X11 ，什么时候再调试下吧
```txt
 ldd triangle
        linux-vdso.so.1 (0x0000fffee05dc000)
        libGL.so.1 => /nix/store/7dwj6nqfl32cdzc1sd4q1dh5ik3mch29-libglvnd-1.7.0/lib/libGL.so.1 (0x0000fffee04d0000)
        libGLU.so.1 => /nix/store/1fb4kdqm1lzv1jn1gjnglzzz3f07a9zn-glu-9.0.3/lib/libGLU.so.1 (0x0000fffee0440000)
        libglut.so.3 => /nix/store/k6s7qgrq9ai8f5vqsf3m5yr8q4y7pi82-freeglut-3.6.0/lib/libglut.so.3 (0x0000fffee0360000)
        libc.so.6 => /nix/store/r0pjdp81mmd7dvk5pv1ch75hrbbw60xb-glibc-2.40-66/lib/libc.so.6 (0x0000fffee0170000)
        libGLX.so.0 => /nix/store/7dwj6nqfl32cdzc1sd4q1dh5ik3mch29-libglvnd-1.7.0/lib/libGLX.so.0 (0x0000fffee0100000)
        libX11.so.6 => /nix/store/ypkf3vajkprw0hmx6bxq83821gbhw1g1-libX11-1.8.12/lib/libX11.so.6 (0x0000fffedff90000)
        libXext.so.6 => /nix/store/n2yldzzax9srwz15s5qlrmx5j307qzzl-libXext-1.3.6/lib/libXext.so.6 (0x0000fffedff50000)
        libGLdispatch.so.0 => /nix/store/7dwj6nqfl32cdzc1sd4q1dh5ik3mch29-libglvnd-1.7.0/lib/libGLdispatch.so.0 (0x0000fffedfdc0000)
        /nix/store/r0pjdp81mmd7dvk5pv1ch75hrbbw60xb-glibc-2.40-66/lib/ld-linux-aarch64.so.1 (0x0000fffee05e0000)
        libOpenGL.so.0 => /nix/store/7dwj6nqfl32cdzc1sd4q1dh5ik3mch29-libglvnd-1.7.0/lib/libOpenGL.so.0 (0x0000fffedfd40000)
        libstdc++.so.6 => /nix/store/wffgswxkp55xi14jy63rjsnfvl2qvmxy-gcc-14.3.0-lib/lib/libstdc++.so.6 (0x0000fffedfad0000)
        libm.so.6 => /nix/store/r0pjdp81mmd7dvk5pv1ch75hrbbw60xb-glibc-2.40-66/lib/libm.so.6 (0x0000fffedfa20000)
        libgcc_s.so.1 => /nix/store/wffgswxkp55xi14jy63rjsnfvl2qvmxy-gcc-14.3.0-lib/lib/libgcc_s.so.1 (0x0000fffedf9e0000)
        libXrandr.so.2 => /nix/store/j8q8gm06w7hdr5x783znjp56zy4114z4-libXrandr-1.5.4/lib/libXrandr.so.2 (0x0000fffedf9b0000)
        libXxf86vm.so.1 => /nix/store/rlk4qc0iyva2jrj82493pgd2gbzlh7jv-libXxf86vm-1.1.6/lib/libXxf86vm.so.1 (0x0000fffedf980000)
        libXi.so.6 => /nix/store/jp86vrfrj05ycb2vnyp97jvfyicwlbkw-libXi-1.8.2/lib/libXi.so.6 (0x0000fffedf940000)
        libpthread.so.0 => /nix/store/r0pjdp81mmd7dvk5pv1ch75hrbbw60xb-glibc-2.40-66/lib/libpthread.so.0 (0x0000fffedf910000)
        libxcb.so.1 => /nix/store/8zaww51ff754j1j34hhy3l6l1fkn15mh-libxcb-1.17.0/lib/libxcb.so.1 (0x0000fffedf8c0000)
        libXrender.so.1 => /nix/store/46aqb1zshh3xssbclkqbq0yxbjynv47m-libXrender-0.9.12/lib/libXrender.so.1 (0x0000fffedf890000)
        libXau.so.6 => /nix/store/fxj8zn8q7sdvkh4pyv5y65s9s1ymq9jp-libXau-1.0.12/lib/libXau.so.6 (0x0000fffedf860000)
        libXdmcp.so.6 => /nix/store/yx58nfrwndz1zij8frkhl377hgin674d-libXdmcp-1.1.5/lib/libXdmcp.so.6 (
```
