# tmpfs 是如何工作的

- [ ] https://www.jamescoyle.net/knowledge/951-the-difference-between-a-tmpfs-and-ramfs-ram-disk
  - 好家伙，原来 tmpfs 是受限制的

希望记得区分 shm_fault 和 shmem_fault
```c
static vm_fault_t shm_fault(struct vm_fault *vmf)
{
	struct file *file = vmf->vma->vm_file;
	struct shm_file_data *sfd = shm_file_data(file);

	return sfd->vm_ops->fault(vmf);
}
```
