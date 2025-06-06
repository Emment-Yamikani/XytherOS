#include <arch/paging.h>
#include <bits/errno.h>
#include <boot/boot.h>
#include <dev/dev.h>
#include <dev/fb.h>
#include <dev/console.h>
#include <mm/kalloc.h>
#include <mm/mmap.h>
#include <string.h>
#include <sync/spinlock.h>

DECL_DEVOPS(static, fb);

static int fb_vmr_fault(vmr_t *region, vm_fault_t *fault);

static device_t            fbdev;
fb_fixinfo_t            fix_info       = {0};
fb_varinfo_t            var_info       = {0};
static framebuffer_t    fbs[NFBDEV]    = {0};

static vmr_ops_t fb_vmrops = {
    .io = NULL,
    .fault = fb_vmr_fault,
};

#define fblock(fb)      ({ spin_lock(&(fb)->lock); })
#define fbunlock(fb)    ({ spin_unlock(&(fb)->lock); })
#define fbislocked(fb)  ({ spin_islocked(&(fb)->lock); })


static DECL_DEVICE(fb, FS_CHR, FB_DEV_MAJOR, 0);

#define VESA_ID    "VESA-VBE3"

int framebuffer_gfx_init(void) {
    if (bootinfo.fb.type != 1)
        return -ENOENT;

    memset(fbs, 0, sizeof fbs);

    memset(&fix_info, 0, sizeof fix_info);

    memset(&var_info, 0, sizeof var_info);

    fix_info.addr       = bootinfo.fb.addr;
    fix_info.memsz      = bootinfo.fb.pitch * bootinfo.fb.height;

    strncpy(fix_info.id, VESA_ID, strlen(VESA_ID));

    fix_info.type       = bootinfo.fb.type;
    fix_info.line_length= bootinfo.fb.pitch;

    var_info.colorspace = 1;
    var_info.bpp        = bootinfo.fb.bpp;
    var_info.red        = bootinfo.fb.red;
    var_info.transp     = bootinfo.fb.resv;
    var_info.blue       = bootinfo.fb.blue;
    var_info.green      = bootinfo.fb.green;
    var_info.pitch      = bootinfo.fb.pitch;
    var_info.width      = bootinfo.fb.width;
    var_info.height     = bootinfo.fb.height;

    fbs[0].module       = NULL;
    fbs[0].dev          = &fbdev;
    fbs[0].fixinfo      = &fix_info;
    fbs[0].varinfo      = &var_info;
    fbs[0].priv         = &bootinfo.fb;
    return 0;
}

static int fb_init(void) {
    return device_register(&fbdev);
}

static int fb_probe(devid_t *) {
    return 0;
}

static int fb_fini(devid_t *) {
    return 0;
}

static int fb_close(devid_t *dd) {
    if (dd == NULL ||
        dd->major != FB_DEV_MAJOR ||
        dd->minor >= NFBDEV || dd->type != FS_CHR)
        return -EINVAL;
    
    return 0;
}

static int fb_getinfo(devid_t *dd, void *info __unused) {
    if (dd == NULL ||
        dd->major != FB_DEV_MAJOR ||
        dd->minor >= NFBDEV || dd->type != FS_CHR)
        return -EINVAL;
    
    return 0;
}

static int fb_open(devid_t *dd __unused, inode_t **pip __unused) {
    if (dd == NULL || dd->major != FB_DEV_MAJOR ||
        dd->minor >= NFBDEV || dd->type != FS_CHR)
        return -EINVAL;
    return 0;
}

static int fb_ioctl(devid_t *dd, int req, void *argp) {
    int err = 0;
    framebuffer_t *fb = NULL;

    if (dd == NULL ||
        dd->major != FB_DEV_MAJOR ||
        dd->minor >= NFBDEV || dd->type != FS_CHR)
        return -EINVAL;

    /// TODO: may need an explicit locking mechanism here.
    /// To protect fbs[] array.
    fb = &fbs[dd->minor];
    
    fblock(fb);

    switch (req) {
    case FBIOGET_FIX_INFO:
        if (fb->fixinfo == NULL) {
            err = -EINVAL;
            break;
        }

        memcpy(argp, fb->fixinfo, sizeof *fb->fixinfo);
        break;
    case FBIOGET_VAR_INFO:
        if (fb->varinfo == NULL) {
            err = -EINVAL;
            break;
        }

        memcpy(argp, fb->varinfo, sizeof *fb->varinfo);
        break;
    default:
        err = -EINVAL;
    }

    fbunlock(fb);

    return err;
}

static off_t fb_lseek(devid_t *dd, off_t off __unused, int whence __unused) {
    if (dd == NULL ||
        dd->major != FB_DEV_MAJOR ||
        dd->minor >= NFBDEV || dd->type != FS_CHR)
        return -EINVAL;
    
    return -EINVAL;
}

static ssize_t fb_read(devid_t *dd, off_t off, void *buf, size_t sz) {
    ssize_t         size = 0;
    framebuffer_t   *fb  = NULL;

    if (dd == NULL ||
        dd->major != FB_DEV_MAJOR ||
        dd->minor >= NFBDEV || dd->type != FS_CHR)
        return -EINVAL;

    /// TODO: may need an explicit locking mechanism here.
    /// To protect fbs[] array.
    fb = &fbs[dd->minor];
    

    // TODO: Do i need to lock fb???

    if (fb->fixinfo == NULL)
        return -EINVAL;

    if (off > fb->fixinfo->memsz)
        return -ERANGE; // Out of range not allowed.

    size = MIN(sz, fb->fixinfo->memsz - off);
    size = (size_t)memcpy(buf, (void *)(fb->fixinfo->addr + off), size) - 
        (size_t)buf;

    return size;
}

static ssize_t fb_write(devid_t *dd, off_t off, void *buf, size_t sz) {
    ssize_t         size = 0;
    framebuffer_t   *fb  = NULL;

    if (dd == NULL ||
        dd->major != FB_DEV_MAJOR ||
        dd->minor >= NFBDEV || dd->type != FS_CHR)
        return -EINVAL;

    /// TODO: may need an explicit locking mechanism here.
    /// To protect fbs[] array.
    fb = &fbs[dd->minor];
    

    // TODO: Do i need to lock fb???

    if (fb->fixinfo == NULL)
        return -EINVAL;

    if (off > fb->fixinfo->memsz)
        return -ERANGE; // Out of range not allowed.

    size = MIN(sz, fb->fixinfo->memsz - off);
    size = (size_t)memcpy((void *)(fb->fixinfo->addr + off), buf, size) - 
        (fb->fixinfo->addr + off);
    return size;
}

static int fb_vmr_fault(vmr_t *region, vm_fault_t *fault) {
    uintptr_t   fbaddr = 0;
    framebuffer_t *fb  = NULL;

    if (region == NULL || fault == NULL)
        return -EINVAL;

    if (__vmr_exec(region) || __vmr_dontexpand(region))
        return -EINVAL;

    if (!__vmr_read(region) && !__vmr_write(region))
        return -EACCES;
    
    /**
     * @brief In a twist of fate, region->priv has a purpose.
     * I never imagened it would workout this way! ;).
     */
    fb = (framebuffer_t *)region->priv;

    if (fb == NULL)
        return -EFAULT;

    // TODO: Do i need to lock fb here.

    if (fb->fixinfo == NULL)
        return -EFAULT;

    if (__vmr_filepos(region) > fb->fixinfo->memsz)
        return -ERANGE; // Out of range not allowed.

    fbaddr = PGROUND(fb->fixinfo->addr + __vmr_filepos(region));
    return arch_map_i(fault->addr, fbaddr, PGSZ, region->vflags);
}

static int fb_mmap(devid_t *dd, vmr_t *region) {
    framebuffer_t *fb = NULL;

    if (dd == NULL || dd->major != FB_DEV_MAJOR ||
        dd->minor >= NFBDEV || dd->type != FS_CHR)
        return -EINVAL;

    if (region == NULL)
        return -EINVAL;

    /// TODO: may need an explicit locking mechanism here.
    /// To protect fbs[] array.
    fb = &fbs[dd->minor];

    if (fb == NULL)
        return -EFAULT;

    // TODO: Do i need to lock fb here.

    if (fb->fixinfo == NULL)
        return -EFAULT;

    if (__vmr_filepos(region) > fb->fixinfo->memsz)
        return -ERANGE; // Out of range not allowed.

    if (__vmr_exec(region) || __vmr_dontexpand(region))
        return -EINVAL;
    
    if (!__vmr_read(region) && !__vmr_write(region))
        return -EINVAL;

    region->priv  = fb;
    region->vmops = &fb_vmrops;
    return 0;
}

BUILTIN_DEVICE(fbdev, fb_init, NULL, NULL);