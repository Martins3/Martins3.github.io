# nfs 的常识

## 如何使用
https://docs.fedoraproject.org/en-US/fedora-server/services/filesharing-nfs-installation/

参考 docs/kernel/fs-nfs.md

修改
showmount -e 10.0.0.2

### nixos
- https://nixos.wiki/wiki/NFS

- https://linux.die.net/man/5/exports
- https://man7.org/linux/man-pages/man5/nfs.5.html

https://unix.stackexchange.com/questions/122676/how-to-mount-an-remote-filesystem-with-specifying-a-port-number


```txt
sudo mount -t nfs 127.0.0.1:/home/martins3/nfs x
```

```txt
sunrpc on /var/lib/nfs/rpc_pipefs type rpc_pipefs (rw,relatime)
nfsd on /proc/fs/nfsd type nfsd (rw,relatime)
127.0.0.1:/home/martins3/nfs on /home/martins3/.dotfiles/x type nfs (rw,relatime,vers=3,rsize=1048576,wsize=1048576,namlen=255,hard,proto=tcp,timeo=600,retrans=2,sec=sys,mountaddr=127.0.0.1,mountvers=3,mountport=20048,mountproto=udp,local_lock=none,addr=127.0.0.1)
```

### 不知道为什么，nfs 的性能特别差

https://www.ibm.com/docs/en/aix/7.2?topic=management-nfs-performance

显然是被限制速度了:
```txt
➜  ~ fio test.fio
trash: (g=0): rw=write, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=libaio, iodepth=128
fio-3.34
Starting 1 process
trash: Laying out IO file (1 file / 10240MiB)
Jobs: 1 (f=1): [W(1)][0.6%][w=1944KiB/s][w=486 IOPS][eta 08h:17m:13s]
```
iops 限制了，但是拷贝大文件速度还可以

## 基本 io 流程
1. nfs 通过 mount 之后，将文件系统的类型修改了!
  - 或者说，server 这边是一个 bcachefs ，而 nfs 进行 copy 的时候，会利用上 bcachefs 的 cow 吗?
2. 不需要任何在 server 端的配置，通过重新 mount
```txt
submit_bio
iomap_writepages
xfs_vm_writepages
do_writepages
filemap_fdatawrite_wbc
file_write_and_wait_range
xfs_file_fsync
xfs_file_buffered_write
do_iter_readv_writev
vfs_iter_write
nfsd_vfs_write
nfsd_write
nfsd3_proc_write
nfsd_dispatch
svc_process_common
svc_process
svc_recv
nfsd
kthread
ret_from_fork
ret_from_fork_asm
```

```txt
submit_bio
iomap_writepages
xfs_vm_writepages
do_writepages
filemap_fdatawrite_wbc
file_write_and_wait_range
xfs_file_fsync
xfs_file_buffered_write
do_iter_readv_writev
vfs_iter_write
nfsd_vfs_write
nfsd4_write
nfsd4_proc_compound
nfsd_dispatch
svc_process_common
svc_process
svc_recv
nfsd
kthread
ret_from_fork
ret_from_fork_asm
```

如果在 nfs 中进行一个 link 的结果
```txt
xfs_link
xfs_vn_link
vfs_link
nfsd_link
nfsd4_link
nfsd4_proc_compound
nfsd_dispatch
svc_process_common
svc_process
svc_recv
nfsd
kthread
ret_from_fork
ret_from_fork_asm
```

看上去，就是使用了一个 vfs 作为中间层。

## nfs 协议定义的 opcode
<!-- 27b7f276-6393-499f-a6bb-60244f1e5a08 -->

include/linux/nfs4.h 中定义了整个 nfs 的操作
过程:

```c
enum nfs_opnum4 {
	OP_ACCESS = 3,
	OP_CLOSE = 4,
	OP_COMMIT = 5,
	OP_CREATE = 6,
	OP_DELEGPURGE = 7,
	OP_DELEGRETURN = 8,
	OP_GETATTR = 9,
	OP_GETFH = 10,
	OP_LINK = 11,
	OP_LOCK = 12,
	OP_LOCKT = 13,
	OP_LOCKU = 14,
	OP_LOOKUP = 15,
	OP_LOOKUPP = 16,
	OP_NVERIFY = 17,
	OP_OPEN = 18,
	OP_OPENATTR = 19,
	OP_OPEN_CONFIRM = 20,
	OP_OPEN_DOWNGRADE = 21,
	OP_PUTFH = 22,
	OP_PUTPUBFH = 23,
	OP_PUTROOTFH = 24,
	OP_READ = 25,
	OP_READDIR = 26,
	OP_READLINK = 27,
	OP_REMOVE = 28,
	OP_RENAME = 29,
	OP_RENEW = 30,
	OP_RESTOREFH = 31,
	OP_SAVEFH = 32,
	OP_SECINFO = 33,
	OP_SETATTR = 34,
	OP_SETCLIENTID = 35,
	OP_SETCLIENTID_CONFIRM = 36,
	OP_VERIFY = 37,
	OP_WRITE = 38,
	OP_RELEASE_LOCKOWNER = 39,

	/* nfs41 */
	OP_BACKCHANNEL_CTL = 40,
	OP_BIND_CONN_TO_SESSION = 41,
	OP_EXCHANGE_ID = 42,
	OP_CREATE_SESSION = 43,
	OP_DESTROY_SESSION = 44,
	OP_FREE_STATEID = 45,
	OP_GET_DIR_DELEGATION = 46,
	OP_GETDEVICEINFO = 47,
	OP_GETDEVICELIST = 48,
	OP_LAYOUTCOMMIT = 49,
	OP_LAYOUTGET = 50,
	OP_LAYOUTRETURN = 51,
	OP_SECINFO_NO_NAME = 52,
	OP_SEQUENCE = 53,
	OP_SET_SSV = 54,
	OP_TEST_STATEID = 55,
	OP_WANT_DELEGATION = 56,
	OP_DESTROY_CLIENTID = 57,
	OP_RECLAIM_COMPLETE = 58,

	/* nfs42 */
	OP_ALLOCATE = 59,
	OP_COPY = 60,
	OP_COPY_NOTIFY = 61,
	OP_DEALLOCATE = 62,
	OP_IO_ADVISE = 63,
	OP_LAYOUTERROR = 64,
	OP_LAYOUTSTATS = 65,
	OP_OFFLOAD_CANCEL = 66,
	OP_OFFLOAD_STATUS = 67,
	OP_READ_PLUS = 68,
	OP_SEEK = 69,
	OP_WRITE_SAME = 70,
	OP_CLONE = 71,

	/* xattr support (RFC8276) */
	OP_GETXATTR                = 72,
	OP_SETXATTR                = 73,
	OP_LISTXATTRS              = 74,
	OP_REMOVEXATTR             = 75,

	OP_ILLEGAL = 10044,
};
```


## tracepoint

client :
```txt
                 0      nfs4:nfs4_commit
                 0      nfs4:nfs4_write
               252      nfs4:nfs4_read
                 0      nfs4:nfs4_map_gid_to_group
                 0      nfs4:nfs4_map_uid_to_name
             8,175      nfs4:nfs4_map_group_to_gid
             8,175      nfs4:nfs4_map_name_to_uid
                 0      nfs4:nfs4_match_stateid
                 0      nfs4:nfs41_match_stateid
                 0      nfs4:nfs4_cb_layoutrecall_file
                 0      nfs4:nfs4_cb_recall
                 0      nfs4:nfs4_cb_getattr
                 0      nfs4:nfs4_fsinfo
                 0      nfs4:nfs4_lookup_root
               472      nfs4:nfs4_getattr
                 0      nfs4:nfs4_close_stateid_update_wait
                 0      nfs4:nfs4_open_stateid_update_wait
               314      nfs4:nfs4_open_stateid_update
                 0      nfs4:nfs4_delegreturn
                 0      nfs4:nfs4_setattr
                 0      nfs4:nfs4_set_acl
                 0      nfs4:nfs4_get_acl
               501      nfs4:nfs4_readdir
                 0      nfs4:nfs4_readlink
               574      nfs4:nfs4_access
                 0      nfs4:nfs4_rename
                 0      nfs4:nfs4_lookupp
                 0      nfs4:nfs4_secinfo
                 0      nfs4:nfs4_get_fs_locations
                 0      nfs4:nfs4_remove
                 0      nfs4:nfs4_mknod
                 0      nfs4:nfs4_mkdir
                 0      nfs4:nfs4_symlink
             3,712      nfs4:nfs4_lookup
                 0      nfs4:nfs4_delegreturn_exit
                 0      nfs4:nfs_delegation_need_return
                 0      nfs4:nfs4_detach_delegation
                 0      nfs4:nfs4_reclaim_delegation
               154      nfs4:nfs4_set_delegation
                 0      nfs4:nfs4_state_lock_reclaim
                 0      nfs4:nfs4_set_lock
                 0      nfs4:nfs4_unlock
                 0      nfs4:nfs4_get_lock
               157      nfs4:nfs4_close
                56      nfs4:nfs4_cached_open
               225      nfs4:nfs4_open_file
                 0      nfs4:nfs4_open_expired
                 0      nfs4:nfs4_open_reclaim
                 0      nfs4:nfs_cb_badprinc
                 0      nfs4:nfs_cb_no_clp
                 0      nfs4:nfs4_xdr_bad_filehandle
                54      nfs4:nfs4_xdr_status
                 0      nfs4:nfs4_xdr_bad_operation
                 0      nfs4:nfs4_state_mgr_failed
                 0      nfs4:nfs4_state_mgr
             5,868      nfs4:nfs4_setup_sequence
                 0      nfs4:nfs4_renew_async
                 0      nfs4:nfs4_renew
                 0      nfs4:nfs4_setclientid_confirm
                 0      nfs4:nfs4_setclientid
```

server 端:
```txt
                 0      nfsd:nfsd_vfs_statfs
            10,614      nfsd:nfsd_vfs_getattr
               502      nfsd:nfsd_vfs_readdir
                 0      nfsd:nfsd_vfs_rename
                 0      nfsd:nfsd_vfs_unlink
                 0      nfsd:nfsd_vfs_link
                 0      nfsd:nfsd_vfs_symlink
                 0      nfsd:nfsd_vfs_create
             4,085      nfsd:nfsd_vfs_lookup
                 0      nfsd:nfsd_vfs_setattr
                 0      nfsd:nfsd_copy_async_cancel
                 0      nfsd:nfsd_copy_async_done
                 0      nfsd:nfsd_copy_done
                 0      nfsd:nfsd_copy_async
                 0      nfsd:nfsd_copy_intra
                 0      nfsd:nfsd_copy_inter
                 0      nfsd:nfsd_end_grace
                 0      nfsd:nfsd_ctl_recoverydir
                 0      nfsd:nfsd_ctl_time
                 0      nfsd:nfsd_ctl_maxblksize
                 0      nfsd:nfsd_ctl_ports_addxprt
                 0      nfsd:nfsd_ctl_ports_addfd
                 0      nfsd:nfsd_ctl_version
                 0      nfsd:nfsd_ctl_pool_threads
                 0      nfsd:nfsd_ctl_threads
                 0      nfsd:nfsd_ctl_filehandle
                 0      nfsd:nfsd_ctl_unlock_fs
                 0      nfsd:nfsd_ctl_unlock_ip
                 0      nfsd:nfsd_cb_recall_any_done
                 0      nfsd:nfsd_cb_getattr_done
                 0      nfsd:nfsd_cb_offload_done
                 0      nfsd:nfsd_cb_layout_done
                 0      nfsd:nfsd_cb_notify_lock_done
                 0      nfsd:nfsd_cb_recall_done
                 0      nfsd:nfsd_cb_recall_any
                 0      nfsd:nfsd_cb_offload
                 0      nfsd:nfsd_cb_notify_lock
                 0      nfsd:nfsd_cb_recall
                 0      nfsd:nfsd_cb_free_slot
                 0      nfsd:nfsd_cb_seq_status
                 1      nfsd:nfsd_cb_bc_shutdown
                 3      nfsd:nfsd_cb_bc_update
                 0      nfsd:nfsd_cb_restart
                 1      nfsd:nfsd_cb_destroy
                 3      nfsd:nfsd_cb_queue
                 0      nfsd:nfsd_cb_setup_err
                 2      nfsd:nfsd_cb_setup
                 0      nfsd:nfsd_cb_rpc_release
                 0      nfsd:nfsd_cb_rpc_done
                 0      nfsd:nfsd_cb_rpc_prepare
                 0      nfsd:nfsd_cb_shutdown
                 0      nfsd:nfsd_cb_lost
                 2      nfsd:nfsd_cb_probe
                 1      nfsd:nfsd_cb_new_state
                 3      nfsd:nfsd_cb_start
                 0      nfsd:nfsd_cb_nodelegs
                 2      nfsd:nfsd_cb_args
                 0      nfsd:nfsd_drc_mismatch
                 0      nfsd:nfsd_drc_found
                 0      nfsd:nfsd_file_close
                 0      nfsd:nfsd_file_shrinker_removed
                 0      nfsd:nfsd_file_gc_removed
                 0      nfsd:nfsd_file_gc_disposed
                 0      nfsd:nfsd_file_gc_aged
                 0      nfsd:nfsd_file_gc_referenced
                 0      nfsd:nfsd_file_gc_writeback
                 0      nfsd:nfsd_file_gc_in_use
                 0      nfsd:nfsd_file_lru_del
                 0      nfsd:nfsd_file_lru_add
                 0      nfsd:nfsd_file_fsnotify_handle_event
                 0      nfsd:nfsd_file_is_cached
                 0      nfsd:nfsd_file_opened
               161      nfsd:nfsd_file_open
                 0      nfsd:nfsd_file_cons_err
                 0      nfsd:nfsd_file_insert_err
               161      nfsd:nfsd_file_acquire
               161      nfsd:nfsd_file_get_dio_attrs
               161      nfsd:nfsd_file_alloc
                 0      nfsd:nfsd_file_closing
               417      nfsd:nfsd_file_put
                 4      nfsd:nfsd_file_unhash
                 4      nfsd:nfsd_file_free
                 0      nfsd:nfsd_clid_confirmed_r
                 1      nfsd:nfsd_clid_fresh
                 0      nfsd:nfsd_clid_verf_mismatch
                 0      nfsd:nfsd_clid_cred_mismatch
                 0      nfsd:nfsd_writeverf_reset
                 0      nfsd:nfsd_grace_complete
                 0      nfsd:nfsd_grace_start
                 0      nfsd:nfsd_mark_client_expired
                 0      nfsd:nfsd_clid_stale
                 1      nfsd:nfsd_clid_renew
                 0      nfsd:nfsd_clid_purged
                 0      nfsd:nfsd_clid_replaced
                 0      nfsd:nfsd_clid_admin_expired
                 0      nfsd:nfsd_clid_destroyed
                 1      nfsd:nfsd_clid_confirmed
                 0      nfsd:nfsd_clid_reclaim_complete
                 0      nfsd:nfsd_clid_expire_unconf
                 0      nfsd:nfsd_slot_seqid_sequence
                 0      nfsd:nfsd_slot_seqid_unconf
                 0      nfsd:nfsd_slot_seqid_conf
                 0      nfsd:nfsd_seq4_status
                 0      nfsd:nfsd_stateowner_replay
                 0      nfsd:nfsd_stid_revoke
                 4      nfsd:nfsd_open_confirm
               165      nfsd:nfsd_preprocess
                 0      nfsd:nfsd_deleg_return
                 0      nfsd:nfsd_deleg_write
               157      nfsd:nfsd_deleg_read
               161      nfsd:nfsd_open
                 0      nfsd:nfsd_layout_recall_release
                 0      nfsd:nfsd_layout_recall_fail
                 0      nfsd:nfsd_layout_recall_done
                 0      nfsd:nfsd_layout_recall
                 0      nfsd:nfsd_layout_return_lookup_fail
                 0      nfsd:nfsd_layout_commit_lookup_fail
                 0      nfsd:nfsd_layout_get_lookup_fail
                 0      nfsd:nfsd_layoutstate_free
                 0      nfsd:nfsd_layoutstate_unhash
                 0      nfsd:nfsd_layoutstate_alloc
                 0      nfsd:nfsd_delegret_wakeup
                 0      nfsd:nfsd_clone_file_range_err
             4,763      nfsd:nfsd_dirent
                 0      nfsd:nfsd_write_err
                 0      nfsd:nfsd_read_err
                 0      nfsd:nfsd_commit_done
                 0      nfsd:nfsd_commit_start
                 0      nfsd:nfsd_write_done
                 0      nfsd:nfsd_write_io_done
                 0      nfsd:nfsd_write_opened
                 0      nfsd:nfsd_write_start
               256      nfsd:nfsd_read_done
               256      nfsd:nfsd_read_io_done
                 0      nfsd:nfsd_read_vector
               256      nfsd:nfsd_read_splice
               256      nfsd:nfsd_read_start
                 5      nfsd:nfsd_export_update
                 0      nfsd:nfsd_exp_get_by_name
                 0      nfsd:nfsd_expkey_update
                 0      nfsd:nfsd_exp_find_key
                 0      nfsd:nfsd_set_fh_dentry_badhandle
                 0      nfsd:nfsd_set_fh_dentry_badexport
                 0      nfsd:nfsd_fh_verify_err
            27,705      nfsd:nfsd_fh_verify
                 0      nfsd:nfsd_compound_encode_err
            31,679      nfsd:nfsd_compound_op_err
                 0      nfsd:nfsd_compound_decode_err
            31,679      nfsd:nfsd_compound_status
            11,596      nfsd:nfsd_compound
                 0      nfsd:nfsd_cant_encode_err
                 0      nfsd:nfsd_garbage_args_err
```

## nfs
- https://about.gitlab.com/blog/2018/11/14/how-we-spent-two-weeks-hunting-an-nfs-bug/
- https://github.com/nfs-ganesha/nfs-ganesha
- [ ] https://zhuanlan.zhihu.com/p/295230549 : 简介


## nfs writeback page 需要小心的考虑
曾经专门统计过 nfs 的 writeback page 的:
```diff
commit 8d92890bd6b8502d6aee4b37430ae6444ade7a8c
Author: NeilBrown <neilb@suse.de>
Date:   Mon Jun 1 21:48:21 2020 -0700

    mm/writeback: discard NR_UNSTABLE_NFS, use NR_WRITEBACK instead

    After an NFS page has been written it is considered "unstable" until a
    COMMIT request succeeds.  If the COMMIT fails, the page will be
    re-written.

    These "unstable" pages are currently accounted as "reclaimable", either
    in WB_RECLAIMABLE, or in NR_UNSTABLE_NFS which is included in a
    'reclaimable' count.  This might have made sense when sending the COMMIT
    required a separate action by the VFS/MM (e.g.  releasepage() used to
    send a COMMIT).  However now that all writes generated by ->writepages()
    will automatically be followed by a COMMIT (since commit 919e3bd9a875
    ("NFS: Ensure we commit after writeback is complete")) it makes more
    sense to treat them as writeback pages.

    So this patch removes NR_UNSTABLE_NFS and accounts unstable pages in
    NR_WRITEBACK and WB_WRITEBACK.

    A particular effect of this change is that when
    wb_check_background_flush() calls wb_over_bg_threshold(), the latter
    will report 'true' a lot less often as the 'unstable' pages are no
    longer considered 'dirty' (as there is nothing that writeback can do
    about them anyway).

    Currently wb_check_background_flush() will trigger writeback to NFS even
    when there are relatively few dirty pages (if there are lots of unstable
    pages), this can result in small writes going to the server (10s of
    Kilobytes rather than a Megabyte) which hurts throughput.  With this
    patch, there are fewer writes which are each larger on average.

    Where the NR_UNSTABLE_NFS count was included in statistics
    virtual-files, the entry is retained, but the value is hard-coded as
    zero.  static trace points and warning printks which mentioned this
    counter no longer report it.

    [akpm@linux-foundation.org: re-layout comment]
    [akpm@linux-foundation.org: fix printk warning]
```

## 这里有 bug 可以挖掘一下

```c
static void nfs_check_dirty_writeback(struct folio *folio,
				bool *dirty, bool *writeback)
{
	struct nfs_inode *nfsi;
	struct address_space *mapping = folio->mapping;

	/*
	 * Check if an unstable folio is currently being committed and
	 * if so, have the VM treat it as if the folio is under writeback
	 * so it will not block due to folios that will shortly be freeable.
	 */
	nfsi = NFS_I(mapping->host);
	if (atomic_read(&nfsi->commit_info.rpcs_out)) {
		*writeback = true;
		return;
	}

	/*
	 * If the private flag is set, then the folio is not freeable
	 * and as the inode is not being committed, it's not going to
	 * be cleaned in the near future so treat it as dirty
	 */
	if (folio_test_private(folio))
		*dirty = true;
}
```
folio_check_dirty_writeback 一定会检查 folio_test_private 的
然后 folio_test_private 在这里重新检查了，这说明至少有一方没有理解清楚这个问题

- is_dirty_writeback : b45972265f823ed01eae0867a176320071665787
- nfs_check_dirty_writeback : f919b19614f06711cba300c1bb1e3d94c9ca21b0

不知道为什么现在也就 buffer_check_dirty_writeback 和 nfs 在使用


执行 “ dd if=/dev/nvme0n1 of=/dev/null bs=512 count=100000000000” 的时候，

这个函数的确是仅仅被 block 注册的
```txt
#0  buffer_check_dirty_writeback (folio=0xffffea0000bdbe00, dirty=0xffffc9000094fb1b, writeback=0xffffc9000094fb1c) at fs/buffer.c:89
#1  0xffffffff81360eb4 in folio_check_dirty_writeback (writeback=0xffffc9000094fb1c, dirty=0xffffc9000094fb1b, folio=0xffffea0000bdbe00) at mm/vmscan.c:1611
#2  shrink_folio_list (folio_list=folio_list@entry=0xffffc9000094fc08, pgdat=pgdat@entry=0xffff88813fff9000, sc=sc@entry=0xffffc9000094fdb0, stat=stat@entry=0xffffc9000094fc90, ignore_references=ignore_references@entry=false) at mm/vmscan.c:1760
#3  0xffffffff81363225 in shrink_inactive_list (lru=<optimized out>, sc=0xffffc9000094fdb0, lruvec=<optimized out>, nr_to_scan=<optimized out>) at mm/vmscan.c:2614
#4  shrink_list (sc=0xffffc9000094fdb0, lruvec=<optimized out>, nr_to_scan=<optimized out>, lru=<optimized out>) at mm/vmscan.c:2855 #5  shrink_lruvec (lruvec=lruvec@entry=0xffff888108dd3800, sc=sc@entry=0xffffc9000094fdb0) at mm/vmscan.c:6303
```


## NFS_SWAP 打开或者关闭意味着什么?

- 为什么会依赖 CONFIG_SUNRPC_SWAP
- 到底预留的是什么?

```c
	.swap_activate = nfs_swap_activate,
	.swap_deactivate = nfs_swap_deactivate,
	.swap_rw = nfs_swap_rw,
```
注册的 nfs_swap_rw 只有 nfs 一个人使用

```txt
🧀  ag "\.swap_rw"
fs/nfs/file.c
573:    .swap_rw = nfs_swap_rw,
```

就这两个位置:
- swap_write_unplug
- `__swap_read_unplug`


这两个 commit 增加了 page-io.c 来实现的，看上去专门是为了支持 nfs 的
```diff
 History:   #0
 Commit:    2282679fb20bf036a714ed49fadd0230c278a203
 Author:    NeilBrown <neilb@suse.de>
 Committer: Andrew Morton <akpm@linux-foundation.org>
 Date:      Tue 10 May 2022 09:20:49 AM CST

 mm: submit multipage write for SWP_FS_OPS swap-space

 swap_writepage() is given one page at a time, but may be called repeatedly
 in succession.

 For block-device swapspace, the blk_plug functionality allows the multiple
 pages to be combined together at lower layers.  That cannot be used for
 SWP_FS_OPS as blk_plug may not exist - it is only active when
 CONFIG_BLOCK=y.  Consequently all swap reads over NFS are single page
 reads.

 With this patch we pass a pointer-to-pointer via the wbc.  swap_writepage
 can store state between calls - much like the pointer passed explicitly to
 swap_readpage.  After calling swap_writepage() some number of times, the
 state will be passed to swap_write_unplug() which can submit the combined
 request.
```

```diff
 History:   #0
 Commit:    e1209d3a7a67c281260ba9989621060ba7328b8c
 Author:    NeilBrown <neilb@suse.de>
 Committer: Andrew Morton <akpm@linux-foundation.org>
 Date:      Tue 10 May 2022 09:20:48 AM CST

 mm: introduce ->swap_rw and use it for reads from SWP_FS_OPS swap-space

 swap currently uses ->readpage to read swap pages.  This can only request
 one page at a time from the filesystem, which is not most efficient.

 swap uses ->direct_IO for writes which while this is adequate is an
 inappropriate over-loading.  ->direct_IO may need to had handle allocate
 space for holes or other details that are not relevant for swap.

 So this patch introduces a new address_space operation: ->swap_rw.  In
 this patch it is used for reads, and a subsequent patch will switch writes
 to use it.

 No filesystem yet supports ->swap_rw, but that is not a problem because
 no filesystem actually works with filesystem-based swap.
 Only two filesystems set SWP_FS_OPS:
 - cifs sets the flag, but ->direct_IO always fails so swap cannot work.
 - nfs sets the flag, but ->direct_IO calls generic_write_checks()
   which has failed on swap files for several releases.

 To ensure that a NULL ->swap_rw isn't called, ->activate_swap() for both
 NFS and cifs are changed to fail if ->swap_rw is not set.  This can be
 removed if/when the function is added.

 Future patches will restore swap-over-NFS functionality.

 To submit an async read with ->swap_rw() we need to allocate a structure
 to hold the kiocb and other details.  swap_readpage() cannot handle
 transient failure, so we create a mempool to provide the structures.
```

## nfs swap cache 触发一下

## 居然在 nfs 中不会使用了，也许搞一个新的 openEuler 测试一下
- https://ubuntu.com/server/docs/service-nfs

修改 nfs 之后，使用这个来生效:

```sh
sudo exportfs -a
```

使用 `*` 来到底谁可以访问:
```txt
/home/martins3/nfs         *(rw,fsid=0,no_subtree_check)
```

## 直接分析下 nfs set private 的含义吧

```txt
@[
    nfs_update_folio+5
    nfs_write_end+224
    generic_perform_write+283
    nfs_file_write+425
    vfs_write+555
    ksys_write+111
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+114
]: 1
```

- nfs_update_folio
  - nfs_writepage_setup
    - nfs_setup_write_request
      - nfs_inode_add_request

其设置的 private 是:

## 这个权限到底是怎么看的
```txt
sudo mount  192.168.26.132:/home/martins3/nfs /mnt
```

最后反而 root 不能用
```txt
drwxr-xr-x - root     11 Aug 08:45  bin
drwxr-xr-x - root      1 Jan  1970  boot
drwxr-xr-x - root     10 Aug 20:22  dev
drwxr-xr-x - root     11 Aug 08:45  etc
drwxr-xr-x - root     26 Nov  2022  home
drwx------ - root     26 Nov  2022  lost+found
drwxrwxr-x - martins3 11 Aug 17:28  mnt
drwxr-xr-x - root     26 Nov  2022  nix
drwx--x--x - root     26 Nov  2022  opt
dr-xr-xr-x - root     11 Aug 04:22  proc
drwx------ - root     10 Aug 09:21  root
drwxr-xr-x - root     11 Aug 08:45  run
drwxr-xr-x - root     26 Nov  2022  srv
dr-xr-xr-x - root     11 Aug 04:22  sys
drwxrwxrwt - root     11 Aug 17:34  tmp
drwxr-xr-x - root     26 Nov  2022  usr
drwxr-xr-x - root     11 Feb 14:59  var
```

## 简单的进行 fio ，然后 kill 掉 fio ，几乎必然导致 hang task

和网络也没有关系，就算是走本地的，也会出现这样的场景，几乎必出现
```txt
[global]
time_based
runtime=30000
ioengine=libaio
iodepth=128
direct=1


[trash]
ioengine=sync
iodepth=1
direct=0

bs=4k
size=30G
rw=write
filename=/mnt
```

## 为什么 NFS_SWAP 依赖 SUNRPC_SWAP 啊

看似这个 NFS_SWAP 完全找不到其依赖，实则不然
```txt
config NFS_SWAP
	bool "Provide swap over NFS support"
	default n
	depends on NFS_FS && SWAP
	select SUNRPC_SWAP
	help
	  This option enables swapon to work on files located on NFS mounts.
```
因为这个 config 打开了 SUNRPC_SWAP

## TODO
- https://unix.stackexchange.com/questions/234154/exactly-what-does-rpcbind-do

## 有个 nfs cilent 的库
https://github.com/sahlberg/libnfs


## 记录一个 backtrace
```txt
[<0>] __switch_to+0xf0/0x158
[<0>] rpc_wait_bit_killable+0x2c/0xc0 [sunrpc]
[<0>] __rpc_execute+0x134/0x4f0 [sunrpc]
[<0>] rpc_execute+0x68/0xe8 [sunrpc]
[<0>] rpc_run_task+0x138/0x190 [sunrpc]
[<0>] rpc_call_sync+0x70/0xc8 [sunrpc]
[<0>] nlmclnt_call+0xb8/0x2c8 [lockd]
[<0>] nlmclnt_proc+0x4d4/0x770 [lockd]
[<0>] nfs3_proc_lock+0x4c/0xe8 [nfsv3]
[<0>] nfs_lock+0x1ec/0x2c0 [nfs]
[<0>] vfs_test_lock+0x38/0x60
[<0>] fcntl_getlk+0xac/0x158
[<0>] do_fcntl+0x384/0x950
[<0>] __arm64_sys_fcntl+0x94/0xc0
[<0>] el0_svc_common+0x78/0x178
[<0>] el0_svc_handler+0x38/0x78
[<0>] el0_svc+0x8/0x640
[<0>] 0xffffffffffffffff
```

https://docs.oracle.com/cd/E19253-01/816-4555/netmonitor-12/index.html


## 原来 nfs 的状态这么复杂啊
include/linux/nfs4.h

```c
enum nfsstat4 {
	NFS4_OK = 0,
	NFS4ERR_PERM = 1,
```

## 使用说明比较清楚了
https://wiki.gentoo.org/wiki/Nfs-utils

## nfs 的 server 重启之后，是如何连接的?

## nfs 可以被挂在到多个 client 中，如何保持数据同步 ?

## nfs over RDMA
https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/7/html/storage_administration_guide/nfs-serverconfig

## 如何理解

在客户端无法删除，只有到 host 上才可以删除。
```txt
.rw-r--r--  221k martins3  3 Jun 15:44   .nfs000000018031e00f00000010
.rw-r--r--  221k martins3  3 Jun 15:44   .nfs000000018031e0040000000e
.rw-r--r--  221k martins3  3 Jun 15:44   .nfs000000018031e0050000000f
.rw-r--r--  221k martins3  3 Jun 15:44   .nfs000000018031e01000000011
.rw-r--r--  221k martins3  3 Jun 15:44   .nfs000000018031e01400000012
.rw-r--r--  221k martins3  3 Jun 15:44   .nfs000000018031e01500000013
.rw-r--r--  221k martins3  3 Jun 15:44   .nfs000000018031e01600000014
.rw-r--r--  221k martins3  3 Jun 15:44   .nfs000000018031e01700000015
```

在 guest 端可以观察到:
```txt
[16496.676743] RPC: Could not send backchannel reply error: -110
[16607.584223] RPC: Could not send backchannel reply error: -110
[16741.592622] RPC: Could not send backchannel reply error: -110
[16798.552039] RPC: Could not send backchannel reply error: -110
[16907.462384] RPC: Could not send backchannel reply error: -110
[17062.002912] RPC: Could not send backchannel reply error: -110
[17113.691267] RPC: Could not send backchannel reply error: -110
[17139.964266] RPC: Could not send backchannel reply error: -110
[17625.863635] RPC: Could not send backchannel reply error: -110
[17675.436696] RPC: Could not send backchannel reply error: -110
[17745.853723] RPC: Could not send backchannel reply error: -110
[17787.492862] RPC: Could not send backchannel reply error: -110
[17796.810013] RPC: Could not send backchannel reply error: -110
[17911.380998] RPC: Could not send backchannel reply error: -110
[17929.780625] RPC: Could not send backchannel reply error: -110
[17947.799319] RPC: Could not send backchannel reply error: -110
[18049.365500] RPC: Could not send backchannel reply error: -110
[18123.644695] RPC: Could not send backchannel reply error: -110
[18154.117711] RPC: Could not send backchannel reply error: -110
[18164.455044] RPC: Could not send backchannel reply error: -110
[18173.642013] RPC: Could not send backchannel reply error: -110
[18231.024324] RPC: Could not send backchannel reply error: -110
[18257.306630] RPC: Could not send backchannel reply error: -110
[18275.940133] RPC: Could not send backchannel reply error: -110
[18298.040613] RPC: Could not send backchannel reply error: -110
[18320.020667] RPC: Could not send backchannel reply error: -110
[18336.255744] RPC: Could not send backchannel reply error: -110
[18400.005076] RPC: Could not send backchannel reply error: -110
[18409.161450] RPC: Could not send backchannel reply error: -110
[18435.546669] RPC: Could not send backchannel reply error: -110
```

即便是 guest os 和 host 直接的共享，还是可以观察到。
这些文件还可以最后自动被删除掉。

## 为什么将 linux source trace 放到 nfs 上之后，我的龟龟
那个性能，为什么会这么差啊

git status 需要 18s

性能瓶颈是什么，感觉是哪里配置的问题吧，带宽只有 1 ~ 2M 的样子

## 虚拟机中发现这个错误
```txt
[  477.242028] NFS: 10.0.0.2: lost 1 locks
```
这是用 nfs 提供文件给 qemu ，qemu 在 nfs 上使用 file lock 的时候，
就会有这个问题


## 收藏
- https://blogsystem5.substack.com/p/demystifying-secure-nfs

## netapp 和 nfs 是什么关系?
https://bluexp.netapp.com/blog/azure-anf-blg-linux-nfs-server-how-to-set-up-server-and-client

他自己有一个后端吗?

## 有趣
https://nluug.nl/bestanden/presentaties/2024-05-21-tom-lyon-why-nfs-must-die-and-how-to-get-beyond-file-sharing-in-the-cloud.pdf
https://news.ycombinator.com/item?id=40723541

## 似乎使用 rsync 同步之后，大写的文件名称会有问题
目前看，只有 nfs 是无懈可击的

## 为什么 l2 虚拟机会卡一下
2025-01-08
```txt
[root@bogon ~]# cat /proc/2063/stack
[<0>] rpc_wait_bit_killable+0x11/0x70
[<0>] __rpc_execute+0x11f/0x480
[<0>] rpc_execute+0xca/0xf0
[<0>] rpc_run_task+0x125/0x180
[<0>] nfs4_do_call_sync+0x6d/0xb0
[<0>] _nfs4_proc_getattr+0x13e/0x170
[<0>] nfs4_proc_getattr+0x76/0x100
[<0>] __nfs_revalidate_inode+0xa5/0x2b0
[<0>] nfs_access_get_cached+0x1ce/0x270
[<0>] nfs_do_access+0x67/0x290
[<0>] nfs_permission+0x92/0x170
[<0>] inode_permission+0xd8/0x190
[<0>] link_path_walk.part.0.constprop.0+0x2ff/0x390
[<0>] path_lookupat+0x3e/0x1a0
[<0>] filename_lookup+0xdc/0x1a0
[<0>] user_path_at+0x37/0x50
[<0>] do_faccessat+0x101/0x2e0
[<0>] do_syscall_64+0xbc/0x210
[<0>] entry_SYSCALL_64_after_hwframe+0x77/0x7f
```

## l1 虚拟机中发现了这个
```txt
[    3.061513] FS-Cache: Loaded
[    3.065951] FS-Cache: Netfs 'nfs' registered for caching
[    3.068553] Key type dns_resolver registered
[    3.072146] NFS: Registering the id_resolver key type
[    3.072349] Key type id_resolver registered
[    3.072580] Key type id_legacy registered
[    3.075889] FS-Cache: Duplicate cookie detected
[    3.076164] FS-Cache: O-cookie c=00000000fe226755 [p=0000000004c89372 fl=222 nc=0 na=1]
[    3.076439] FS-Cache: O-cookie d=000000005ec14964 n=000000003b484475
[    3.076718] FS-Cache: O-key=[16] '0400000002000000020008010a000002'
[    3.076897] FS-Cache: N-cookie c=00000000e3eb3689 [p=0000000004c89372 fl=2 nc=0 na=1]
[    3.077081] FS-Cache: N-cookie d=000000005ec14964 n=000000004f9d0ef1
[    3.077260] FS-Cache: N-key=[16] '0400000002000000020008010a000002'
```

## 虽然 nfs 已经很完美了，但是还是有这个问题
时间戳有时候不对
```txt
make[3]: warning:  Clock skew detected.  Your build may be incomplete.
```

## 这个 .nfs 文件似乎容易出现在

host l1 l2 ，然后在 l2 中编辑文件:
```txt
🧀  git status
On branch master
Your branch is ahead of 'origin/master' by 6 commits.
  (use "git push" to publish your local commits)

Changes to be committed:
  (use "git restore --staged <file>..." to unstage)
        modified:   code/src/c/sched/sched.c
        modified:   docs/kernel/sched-ext.md
        modified:   docs/kernel/storage-lvm.md

Untracked files:
  (use "git add <file>..." to include in what will be committed)
        code/src/c/sched/.nfs000000000374003400000002
```

在 nfs 中有报错如下:
```txt
localhost login: [  505.081003] NFSv4: state recovery failed for open file sched/.nfs000000000374002000000001, error = -116
[  600.436815] bpftrace[2166]: segfault at 50 ip 00007f88766c71e0 sp 00007ffe8c583868 error 4 in libbpf.so.1.4.7[131e0,7f88766c0000+3e000] likely on CPU 0 (core 0, socket 0)
[  600.438981] Code: 74 15 31 c0 48 83 c4 08 31 d2 31 c9 31 f6 31 ff 45 31 c0 45 31 c9 c3 b8 f4 ff ff ff eb e6 66 66 2e 0f 1f 84 00 00 00 00 00 90 <8b> 47 50 03 47 40 31 ff c3 0f 1f 80 00 00 00 00 55 be 78 00 00 00
```

## 思考一个问题
如果一个 client 的 ca 和 cb 通过 nfs map 到 host 的两个目录 a 和 b ，a 和 b 在一个设备上
现在在 client 中操作 ca 和 cb 的拷贝，nfs 支持瞬间完成吗? 还是必须过一次网络

## 有趣的问题

kunpeng 机器用自己构建的内核之后，在虚拟机中无法自动挂载 nfs :
```txt
  sudo mount.nfs 10.0.0.2://home/martins3/core/vn /mnt/
mount.nfs: Protocol not supported for 10.0.0.2://home/martins3/core/vn on /mnt

rpcinfo 10.0.0.2 |egrep "service|nfs"
   program version netid     address                service    owner
    100003    3    tcp       0.0.0.0.8.1            nfs        superuser
    100003    3    tcp6      ::.8.1                 nfs        superuser
```

给机器添加上 CONFIG_NFSD_V4=y 之后，再次在虚拟机中执行，可以看到:
```txt
🧀  rpcinfo 10.0.0.2 |egrep "service|nfs"
   program version netid     address                service    owner
    100003    3    tcp       0.0.0.0.8.1            nfs        superuser
    100003    4    tcp       0.0.0.0.8.1            nfs        superuser
    100003    3    tcp6      ::.8.1                 nfs        superuser
    100003    4    tcp6      ::.8.1                 nfs        superuser
```

## https://lwn.net/Articles/879027/
这里解释为什么将 swap_set_page_dirty 去掉了，其中 __set_page_dirty_no_writeback

## nfs.recover_lost_locks=1

## 新鲜出炉的东西 2025-06-18
Documentation/filesystems/netfs_library.rst

## pNFS
https://blogs.oracle.com/linux/post/parallelize-nfs-with-pnfs

https://docs.netapp.com/us-en/ontap/nfs-admin/enable-disable-pnfs-task.html

https://www.ietf.org/archive/id/draft-haynes-nfsv4-flexfiles-v2-00.html
(ietf 是什么机构)

https://wiki.linux-nfs.org/wiki/index.php?title=Configuring_pNFS/spnfsd&oldid=5160

https://www.hpcwire.com/2024/02/29/pnfs-provides-performance-and-new-possibilities/


## 提供一个 iscsi 设备，然后在上面构建创建文件系统
和提供 nfs ，哪一个效率更加高

哦，原来是这个样子的:
```txt
obj-$(CONFIG_PNFS_FILE_LAYOUT) += filelayout/
obj-$(CONFIG_PNFS_BLOCK) += blocklayout/
obj-$(CONFIG_PNFS_FLEXFILE_LAYOUT) += flexfilelayout/
```

## 既然有 nfs ，为什么还需要有 fuse

意思是，无论 fuse 还是 nfs ，都是需要经过
guets os 的 vfs ，nfs 的后端也是在用户态实现的。

## 哦，原来这么牛啊
https://techcrunch.com/2025/04/16/hammerspace-an-unstructured-data-wrangler-100m/

## 阅读
- https://lwn.net/Articles/897917/

## 原来有自己的专业测试的东西
https://git.linux-nfs.org/?p=cdmackay/pynfs.git;

## pnfs
https://mp.weixin.qq.com/s/AJy6ghvBRaE4RGuTAikRBw

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
