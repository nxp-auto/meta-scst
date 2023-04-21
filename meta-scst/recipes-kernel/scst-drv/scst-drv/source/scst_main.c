 /*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/arm-smccc.h>
#include <linux/miscdevice.h>

#include "scst_types.h"

#define F_FILENAME              "scst_fw.bin"
#define DEVICE_NAME             "scst_drv"

#define F_BUF_SIZE              (1024UL)
#define MAX_BUF_SIZE            (0x100000UL)

/* Function ID */
#define SMCCC_LXOS_CMD          (0x8000000CUL)
#define SMCCC_SCST_CMD          (0x8000000DUL)

/* Command types */
#define LXOS_SCST_SETUP         (0x4C000001UL)
#define SCST_SETUP_DONE         (0x53000001UL)

#define LXOS_SCST_EXECUTE       (0x4C000002UL)
#define SCST_EXECUTE_DONE       (0x53000002UL)

#define LXOS_SCST_CLEAR         (0x4C000003UL)
#define SCST_CLEAR_DONE         (0x53000003UL)

void __iomem *scst_base;

static void scst_smc(unsigned long function_id,
        unsigned long arg0, unsigned long arg1,
        unsigned long arg2, struct arm_smccc_res *res)
{
    arm_smccc_smc(function_id, arg0, arg1, arg2, 0, 0, 0, 0, res);
}

static int fread(struct file *f,
            unsigned long long offset,
            unsigned char *buf,
            unsigned int count)
{
    mm_segment_t oldfs;
    int ret;

    oldfs = get_fs();
    set_fs(get_fs());
    ret = kernel_read(f, buf, count, &offset);
    set_fs(oldfs);
    return ret;
}

static void fclose(struct file *f)
{
    filp_close(f, NULL);
}

static struct file *fopen(const char *filename, int flags, int mode)
{
    struct file *f = NULL;
    mm_segment_t fs;

    fs = get_fs();
    set_fs(get_fs());
    f = filp_open(filename, flags, mode);
    if (IS_ERR(f))
        return NULL;

    set_fs(fs);

    return f;
}

static int scst_open(struct inode *inode, struct file *filp)
{
    int ret = 0;

    return ret;
}

static int scst_release(struct inode *inode, struct file *filp)
{
    int ret = 0;

    return ret;
}

static long scst_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long ret = 0;
    scst_args_t kbuf;
    void __user *ubuf = (void __user *)arg;
    struct arm_smccc_res scst_res;
    unsigned long flags;
    
    switch (cmd) {
    case SCST_KM_IOCTL_PRE:
        local_irq_save(flags);
        scst_smc(SMCCC_LXOS_CMD, LXOS_SCST_SETUP, 0, 0, &scst_res);
        local_irq_restore(flags);

        if (scst_res.a0 != SCST_SETUP_DONE) {
            pr_err("[ERROR] Setup scst env failed!\n");
            ret = -EFAULT;
        }
        break;
    case SCST_KM_IOCTL_RUN:
        ret = copy_from_user((void *)&kbuf, ubuf, sizeof(scst_args_t));
        if (unlikely(ret)) {
            pr_err("[ERROR] Unable to get parameter from user space!\n");
            ret = -EFAULT;
        } else {
            scst_args_t *args;

            args = (scst_args_t *)&kbuf;

            local_irq_save(flags);
            scst_smc(SMCCC_LXOS_CMD, LXOS_SCST_EXECUTE, args->scst_arg0, args->scst_arg1, &scst_res);
            local_irq_restore(flags);

            if (scst_res.a0 != SCST_EXECUTE_DONE) {
                pr_err("[ERROR] Perform scst: (%d, %d) error!\n", args->scst_arg0, args->scst_arg1);
                ret = -EFAULT;
            } else {

                kbuf.scst_arg0 = scst_res.a1;
                kbuf.scst_arg1 = 0;
                ret = copy_to_user(ubuf, (void *)&kbuf, sizeof(scst_args_t));
                
                if (unlikely(ret)) {
                    pr_err("[ERROR] Unable to return scst result to userspace!\n");
                    ret = -EFAULT;
                }
            }
        }
        break;

    default:
        break;
    }

    return ret;
}


/* File operations */
static const struct file_operations scst_fops = {
    .owner = THIS_MODULE,
    .open = scst_open,
    .release = scst_release,
    .unlocked_ioctl = scst_ioctl
};

static struct miscdevice scst_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &scst_fops
};

static int __init scst_init(void)
{
    struct device_node *node, *rmem_node;
    int ret;
    int cnt;
    struct file *f;
    void *buffer;
    unsigned long long offset = 0U, baddr;
    struct resource res;

    /* Find the reserved memory from dts */
    rmem_node = of_find_node_by_path("/reserved-memory");
    for_each_child_of_node(rmem_node, node) {
        if (of_node_name_eq(node, "scst-mem"))
            break;
    }

    if (!node) {
        pr_err("[ERROR] No reserved memory for scst-mem\n");
        ret = -ENOTSUPP;
        goto L_exit;
    }

    scst_base = of_iomap(node, 0);
    of_node_put(node);

    if (scst_base) {
        ret = of_address_to_resource(node, 0, &res);
        if (ret) {
            ret = -ENOMEM;
            goto L_unmap;
        }
        /* Initialize the memory */
        ret = (res.end != res.start) ? (res.end - res.start + 1) : 0;
        memset_io(scst_base, 0, ret);
    } else {
        ret = -ENOMEM;
        goto L_exit;
    }

    pr_info("[INFO] scst reserved memory mapped @0x%llx, size: 0x%llx\n", (unsigned long long)scst_base, res.end - res.start + 1);

    /* Register misc device */
    ret = misc_register(&scst_misc);
    if (unlikely(ret)) {
        pr_err("[ERROR] Register misc dev failed\n");
        goto L_unregister;
    }

    /* Allocate a buffer to load scst_app.bin file */
    buffer = kmalloc(F_BUF_SIZE, GFP_KERNEL);
    if (unlikely(buffer == NULL)) {
        ret = -ENOMEM;
        goto L_unregister;
    }

    /* Open the .bin file */
    f = fopen(F_FILENAME, O_RDONLY, 0);
    if (unlikely(f == NULL)) {
        pr_err("[ERROR] Open %s file failed\n", F_FILENAME);
        ret = -ENOENT;
        goto L_kfree;
    }

    baddr = (unsigned long long)(scst_base);

    /* Check file and buffer size */
    if (unlikely(f->f_inode->i_size > MAX_BUF_SIZE)) {
        pr_err("[ERROR] File %s size 0x%llx over than 0x%llx\n", F_FILENAME,
        (unsigned long long)f->f_inode->i_size, (unsigned long long)MAX_BUF_SIZE);
        ret = -EFBIG;
        goto L_cfile;
    } else {
        do {
            cnt = fread(f, offset, (unsigned char *)buffer, F_BUF_SIZE);
            memcpy_toio((void *)(baddr + offset), buffer, cnt);
            offset += cnt;
        } while (cnt >= F_BUF_SIZE);

        pr_info("[INFO] %lld bytes from %s written @%llx\n", offset, F_FILENAME, (unsigned long long)baddr);

        fclose(f);
        kfree(buffer);
    }

    return 0;

L_cfile:
    fclose(f);
L_kfree:
    kfree(buffer);
L_unregister:
    misc_deregister(&scst_misc);
L_unmap:
    iounmap(scst_base);
L_exit:
    return ret;
}

static void scst_clear_cpu_ctx(void)
{
    struct arm_smccc_res scst_res;
    unsigned long flags;

    local_irq_save(flags);
    scst_smc(SMCCC_LXOS_CMD, LXOS_SCST_CLEAR, 0, 0, &scst_res);
    local_irq_restore(flags);
    
    if (scst_res.a0 != SCST_CLEAR_DONE) {
        pr_err("[ERROR] Clear scst cpu ctx failed!\n");
    }
}

static void __exit scst_exit(void)
{
    scst_clear_cpu_ctx();
    misc_deregister(&scst_misc);
    iounmap(scst_base);
}

module_init(scst_init);
module_exit(scst_exit);

MODULE_AUTHOR("NXP Ltd");
MODULE_DESCRIPTION("SCST driver");
MODULE_LICENSE("GPL");
