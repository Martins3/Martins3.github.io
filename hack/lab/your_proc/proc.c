// copy from  http://krishnamohanlinux.blogspot.com/2015/02/getuserpages-example.html
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#include <linux/sched.h>
#include <linux/slab.h>

static struct class *proc_class;

static int proc_open(struct inode *inode, struct file *file) {
  printk(KERN_INFO "%s\n", __FUNCTION__);
  return (0);
}

static int proc_release(struct inode *inode, struct file *file) {
  printk(KERN_INFO "%s\n", __FUNCTION__);
  return (0);
}

static ssize_t proc_write(struct file *file, const char __user *buf,
                            size_t count, loff_t *off) {
  
  printk(KERN_INFO "%s\n", __FUNCTION__);
  return (0);
}

ssize_t proc_read(struct file * file , char __user * buf, size_t count , loff_t * off){
  // TODO
  // 将 current : proc id 等等
  return 0;
}

static struct file_operations proc_ops = {.owner = THIS_MODULE,
                                            .open = proc_open,
                                            .read = proc_read,
                                            .release = proc_release,
                                            .write = proc_write};

static int __init proc_init(void) {
  int ret;
  ret = register_chrdev(42, "MyProc", &proc_ops);
  proc_class = class_create(THIS_MODULE, "MyProc");
  device_create(proc_class, NULL, MKDEV(42, 0), NULL, "MyProc");
  return (ret);
}

static void __exit proc_exit(void) {
  device_destroy(proc_class, MKDEV(42, 0));
  class_destroy(proc_class);
  unregister_chrdev(42, "MyProc");
}

module_init(proc_init);
module_exit(proc_exit);

MODULE_LICENSE("GPL");
