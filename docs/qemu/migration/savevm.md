# save vm

- [ ] 思考一个问题，既然可以让 snapshot 和 migration 两个功能类似的功能放到一起，那么是不是 upgrade 的功能可以类似的放到一起的。

一生之敌
- aio context
    - `aio_context_acquire`
    - `aio_context_release`
- `cpu_synchronize_all_post_init`


- [ ] `vmstate_register_ram`
    - 本来以为是放到某一个 list 中的，但是并没有


savevm 中 `load_snapshot` 和 `save_snapshot` 可以和 migration 共用
- `hmp_loadvm`
    - `load_snapshot` ：似乎是从一个 block driver 中 load 的 snapshot，TODO 为什么不是直接读取一个文件，而是搞的这么复杂啊
        - `bdrv_all_has_snapshot`
        - [ ] 类似的还有各种操作，但是最后的结果其实只是为了获取一个 QEMUFile
        - `qemu_loadvm_state`
            - `qemu_loadvm_state_header`
                - 检查 vmfile `QEMU_VM_FILE_MAGIC`
        - `qemu_loadvm_state_main`

- `hmp_savevm`
    - `save_snapshot`
        - `qemu_savevm_state` ：TODO 在 load 中，这个函数就是开始和 migration 开始共用，但是 save 是下一个函数才开始的
            - `qemu_savevm_state_iterate`

## [ ]  为什么 savevm 和 postcopy 有关系哇
似乎主要是 postcopy 的一些通信的操作
