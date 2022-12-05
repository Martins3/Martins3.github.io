# IncludeOS

<!-- vim-markdown-toc GitLab -->

* [env setup](#env-setup)
* [想法](#想法)
* [design](#design)
  * [questions](#questions)
  * [crt](#crt)
  * [calling route](#calling-route)
* [how printf works](#how-printf-works)
    * [start up](#start-up)
    * [TCP_FD::write](#tcp_fdwrite)
    * [syscall_SYS_read](#syscall_sys_read)
* [reading material](#reading-material)
* [Some cpp skills I learn](#some-cpp-skills-i-learn)
* [Something else I learn](#something-else-i-learn)

<!-- vim-markdown-toc -->

in `your_build_dir`, `run.sh` `source activete.sh` `boot hello` to run the data.
```
➜  incOs ls
 hello_world   IncludeOS   your_build_dir

➜  your_build_dir cat run.sh
cur_dir=`pwd`

inc=/home/maritns3/core/incOs/IncludeOS
cd $inc
cmake --build build

cd $cur_dir
cmake --build .
```

## env setup
in `IncludeOS`:
```
# init operation
# clang-6.0-linux-x86_64

mkdir build
conan editable add . includeos/$(conan inspect -a version . | cut -d " " -f 2)@includeos/latest --layout=etc/layout.txt
conan install -if build . -pr <conan_profile>
conan build -bf build .

# build every time
cmake --build build
```

In `hello_world`: follow readme's instructors

- [ ] 仔细阅读一下 Building with IncludeOS in editable mode 内容来描述吧

## 想法

## design
> Virtio and vmxnet3 Network drivers with DMA. Virtio provides a highly efficient and widely supported I/O virtualization. vmxnet3 is the VMWare equivalent.

- [ ] vmxnet3 是什么?

### questions
- [ ] how could it possible to simulate a kernel in 40000 lines of code.
- [ ] Inspired by virtio, kvm and user space kernel, maybe we can invent new kernel without devices driver.

- paging ?
- network ?
- from stdout -> musl -> write syscall -> ... ? trace it !

- from kernel start up and the userspace applications

- how to project compiled
  - run userspace / test

- why /home/maritns3/core/incOs/IncludeOS/src/posix/pthread.cpp is compiled and included ?

- single address space, but with clone, but no kernel scheduler.

- [ ] posix_memalign ==> musl, what's the purpose of src/musl

- [ ] memdisk ?

- [ ] src/chainload
  - something related with service

- [ ] I need run the test
  - /home/maritns3/core/incOs/IncludeOS/test/hw/integration/serial/run.sh

- [ ] /home/maritns3/core/incOs/IncludeOS/src/hw/ ? seems useless

- [ ] from kernel init to hello world ?

### crt
src/crt provide C runtime support

### calling route


## how printf works
Evne in function src/platform/x86_pc/kernel_start.cpp::kernel_start, we can call printf.

where is the package ?
/home/maritns3/.conan/data/musl/1.1.18/includeos/stable/package/b6ca6a0ffff110bf17b843d4258482a94281eb43/include/stdio.h
this is standard musl interface.

**It seems musl in IncludeOS is never called**, and we can't call exit()

#### start up
- kernel_start
  - x86::init_libc
    - `__libc_start_main`
      - kernel_main : executed as a hook by `__libc_start_main`
        - kernel::start
          - os::add_stdout(&kernel::default_stdout);
          - `__arch_init_paging`
          - kernel::multiboot
          - `__platform_init` : hardware setup
        - kernel::post_start()
          - Service::start();

#### TCP_FD::write
- TCP_FD::write
  - TCP_FD::send
    - TCP_FD_Conn::send
      - Connection::write(buffer_t buffer)
        - writeq.push_back(std::move(buffer)) // add to queue
          - TCP::request_offe
            - Connection::offer
              - Connection::create_outgoing_packet
              - Connection::fill_packet
              - Connection::transmit
                - TCP::transmit
                  - IP4::transmit
                    - IP4::ship
                      - Arp::transmit
                        - TODO : got lost out of cpp syntax, but I will

#### syscall_SYS_read
- syscall_SYS_read
  - sys_read
    - Dirent::read
      - File_system::read
        - FAT:read // use mem as disk or virtio, read and read_sync are different, virtio only implement async read
          - MemDisk::read_sync
          - VirtioBlk::read

## reading material
- [ ] https://ma.ttias.be/what-is-a-unikernel/
- [ ] https://github.com/cetic/unikernels

## Some cpp skills I learn
- [ ] https://stackoverflow.com/questions/1041866/what-is-the-effect-of-extern-c-in-c
- [ ] https://en.cppreference.com/w/cpp/container/array
  - https://stackoverflow.com/questions/4424579/stdvector-versus-stdarray-in-c
    - > std::vector is a template class that encapsulate a dynamic array1, stored in the heap, that grows and shrinks automatically if elements are added or removed.
    - > std::array is a template class that encapsulate a statically-sized array, stored inside the object itself, which means that, if you instantiate the class on the stack, the array itself will be on the stack.
- https://stackoverflow.com/questions/18198314/what-is-the-override-keyword-in-c-used-for

```c
#pragma once
noexcept
```

- Let me understand how `std::move` works ?

- [ ] template programming

```cpp
template <class T>
class Link_layer : public hw::Nic {
public:
  using Protocol    = T;

// ...

private:
  Protocol link_;
};

template <class Protocol>
Link_layer<Protocol>::Link_layer(Protocol&& protocol)
  : hw::Nic(),
    link_{std::forward<Protocol>(protocol)}
{
}


/** Virtio-net device driver.  */
class VirtioNet : Virtio, public net::Link_layer<net::Ethernet> {

};
```

## Something else I learn
- [ ] cpp package manage
- cmake
