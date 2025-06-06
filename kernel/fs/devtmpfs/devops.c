#include <dev/dev.h>
#include <fs/dentry.h>
#include <fs/devtmpfs.h>
#include <fs/file.h>
#include <fs/inode.h>
#include <fs/stat.h>
#include <sys/thread.h>

iops_t dev_iops = {
    .iopen      = dev_iopen,
    .isync      = dev_isync,
    .iclose     = dev_iclose,
    .iunlink    = dev_iunlink,
    .itruncate  = dev_itruncate,
    .igetattr   = dev_igetattr,
    .isetattr   = dev_isetattr,
    .ifcntl     = dev_ifcntl,
    .iioctl     = dev_iioctl,
    .iread      = dev_iread,
    .iwrite     = dev_iwrite,
    .imkdir     = dev_imkdir,
    .icreate    = dev_icreate,
    .ibind      = dev_ibind,
    .ilookup    = dev_ilookup,
    .isymlink   = dev_isymlink,
    .imknod     = dev_imknod,
    .ireaddir   = dev_ireaddir,
    .ilink      = dev_ilink,
    .irename    = dev_irename,
};

int dev_iopen(inode_t *idev, inode_t **pip) {
    int     err = 0;
    inode_t *ip = NULL;

    if (idev == NULL)
        return -EINVAL;
    
    if ((err = device_open(IDEVID(idev), &ip)))
        return err;
    
    if (pip) *pip = ip;
    return 0;
}

int dev_isync(inode_t *idev) {
    if (idev == NULL)
        return -EINVAL;
    
    return -EOPNOTSUPP;
}

int dev_iclose(inode_t *idev) {
    if (idev == NULL)
        return -EINVAL;
    
    return device_close(IDEVID(idev));
}

int dev_iunlink(inode_t *idev) {
    if (idev == NULL)
        return -EINVAL;
    
    return -EOPNOTSUPP;
}

int dev_itruncate(inode_t *idev) {
    if (idev == NULL)
        return -EINVAL;
    
    return -EOPNOTSUPP;
}

int dev_igetattr(inode_t *idev, void *attr) {
    if (idev == NULL)
        return -EINVAL;

    return device_getinfo(IDEVID(idev), attr);
}

int dev_isetattr(inode_t *idev, void *attr __unused) {
    if (idev == NULL)
        return -EINVAL;
    
    return -EOPNOTSUPP;
}

int dev_ifcntl(inode_t *idev, int cmd __unused, void *argp __unused) {
    if (idev == NULL)
        return -EINVAL;
    
    return -EOPNOTSUPP;
}

int dev_iioctl(inode_t *idev, int req, void *argp) {
    if (idev == NULL)
        return -EINVAL;

    return device_ioctl(IDEVID(idev), req, argp);
}

isize dev_iread(inode_t *idev, off_t off, void *buf, usize nb) {
    if (idev == NULL)
        return -EINVAL;

    return device_read(IDEVID(idev), off, buf, nb);
}

isize dev_iwrite(inode_t *idev, off_t off, void *buf, usize nb) {
    if (idev == NULL)
        return -EINVAL;
    // printk("%s:%d: %s->rdev[%d:%d]: %p\n",
        //    __FILE__, __LINE__, itype_strings[idev->i_type], idev->i_rdev & 0xff, idev->i_rdev >> 8, IDEVID(idev));
    return device_write(IDEVID(idev), off, buf, nb);
}

int dev_imkdir(inode_t *didev __unused, const char *fname __unused, mode_t mode __unused) {
    return -EOPNOTSUPP;
}

int dev_icreate(inode_t *didev __unused, const char *fname __unused, mode_t mode __unused) {
    return -EOPNOTSUPP;
}

int dev_ibind(inode_t *didev __unused, struct dentry *dentry __unused, inode_t *ip __unused) {
    return -EOPNOTSUPP;
}

int dev_ilookup(inode_t *didev __unused, const char *fname __unused, inode_t **pipp __unused) {
    return -EOPNOTSUPP;
}

int dev_isymlink(inode_t *idev __unused, inode_t *atdir __unused, const char *symname __unused) {
    return -EOPNOTSUPP;
}

int dev_imknod(inode_t *didev __unused, const char *name __unused, mode_t mode __unused, int devid __unused) {
    return -EOPNOTSUPP;
}

isize dev_ireaddir(inode_t *didev __unused, off_t off __unused, struct dirent *buf __unused, usize count __unused) {
    return -EOPNOTSUPP;
}

int dev_ilink(const char *oldname __unused, inode_t *dir __unused, const char *newname __unused) {
    return -EOPNOTSUPP;
}

int dev_irename(inode_t *didev __unused, const char *old __unused, inode_t *newdir __unused, const char *new __unused) {
    return -EOPNOTSUPP;
}