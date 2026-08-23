/*
 SPDX-License-Identifier: AGPL-3.0-or-later

 Copyright (C) 2025 - 2026 emexlab

 This file is part of Nyxian.

 Nyxian is free software: you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Nyxian is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU Affero General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with Nyxian. If not, see <https://www.gnu.org/licenses/>.
*/

#include <LiveShim/shim.h>
#include <LiveShim/dyld.h>
#include <LiveShim/dyld_node_remap.h>
#include <LiveShim/cdhash.h>
#include <Frameworks/HWHook/HWHookThreadContext.h>
#include <mach-o/dyld_images.h>
#include <sys/mman.h>
#include <copyfile.h>

#if __has_include(<ksurface_config.h>)
#include <ksurface_config.h>
#else
#define KSURFACE_DYLD_HOOK_LOGGING_ENABLED 0
#define KSURFACE_DYLD_HARDENED_CDHASH_VERIFIER 1
#endif /* __has_include(<ksurface_config.h>) */

#if KSURFACE_DYLD_HOOK_LOGGING_ENABLED
#define dyld_hook_log(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define dyld_hook_log(fmt, ...)
#endif /* KSURFACE_DYLD_HOOK_LOGGING_ENABLED */

static const char openSig[] = {0xB0, 0x00, 0x80, 0xD2, 0x01, 0x10, 0x00, 0xD4};
static const char fcntlSig[] = {0x90, 0x0B, 0x80, 0xD2, 0x01, 0x10, 0x00, 0xD4};
static const char fstat64Sig[] = {0x70, 0x2A, 0x80, 0xD2, 0x01, 0x10, 0x00, 0xD4};
static const char stat64Sig[] = {0x50, 0x2A, 0x80, 0xD2, 0x01, 0x10, 0x00, 0xD4};

static int (*orig_dyld_open)(const char *path, int flags, mode_t mode);
static int (*orig_dyld_fcntl)(int fildes, int cmd, void *param);
static int (*orig_dyld_fstat64)(int fildes, struct stat *buf);
static int (*orig_dyld_stat64)(const char *path, struct stat *buf);

static struct dyld_all_image_infos *_alt_dyld_get_all_image_infos(void)
{
    static struct dyld_all_image_infos *result;
    if(result)
    {
        return result;
    }
    struct task_dyld_info dyld_info;
    mach_vm_address_t image_infos;
    mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
    kern_return_t ret;
    ret = task_info(mach_task_self(), TASK_DYLD_INFO, (task_info_t)&dyld_info, &count);
    if(ret != KERN_SUCCESS)
    {
        return NULL;
    }
    image_infos = dyld_info.all_image_info_addr;
    result = (struct dyld_all_image_infos *)image_infos;
    return result;
}

static char *searchDyldFunction(char *base,
                                char *signature,
                                int length)
{
    char *patchAddr = NULL;
    for(int i=0; i < 0x80000; i+=4)
    {
        if(base[i] == signature[0] && memcmp(base+i, signature, length) == 0)
        {
            patchAddr = base + i;
            break;
        }
    }
    return patchAddr;
}

static int hook_fcntl(int fildes,
                      int cmd,
                      void *param)
{
    dyld_hook_log("[hook_fcntl:args] (fildes = %d, cmd: %d, param: %p)\n", fildes, cmd, param);
    int ret = orig_dyld_fcntl(fildes, cmd, param);
#if KSURFACE_DYLD_HOOK_LOGGING_ENABLED
    char path[PATH_MAX];
    if(orig_dyld_fcntl(fildes, F_GETPATH, path) != -1)
    {
        dyld_hook_log("[hook_fcntl:return] (ret = %d, path: %s)\n", ret, path);
    }
    else
    {
        dyld_hook_log("[hook_fcntl:return] (ret = %d)\n", ret);
    }
#endif /* KSURFACE_DYLD_HOOK_LOGGING_ENABLED */
    
    if(cmd == F_GETPATH)
    {
        if(inode_bank_get_path(inode_for_fd(fildes), param, MAXPATHLEN))
        {
            dyld_hook_log("[hook_fcntl:fool] fooling da cutie dyld >:3\n");
        }
    }
    
    return ret;
}

static const char *mmap_sandbox_map_exec_allowed_path = NULL;

static _Thread_local bool cdhash_verified = false;
static _Thread_local bool cdhash_must_valid;
static _Thread_local bool open_hardlock;
static _Thread_local const char *cdhash_data_container_match;
static _Thread_local dlopen_cdhash_verifier_failed_callback_t cdhash_verifier_failed_callback;
static int hook_open(const char *path,
                     int flags,
                     mode_t mode)
{
    dyld_hook_log("[hook_open:args] (path = %s, flags = %d, mode = %d)\n", path, flags, mode);
    if(open_hardlock)
    {
        dyld_hook_log("[hook_open:args] [error: hard locked]\n");
        errno = EACCES;
        return -1;
    }
    
    int fd = orig_dyld_open(path, flags, mode);
    if(fd < 0)
    {
        goto just_return;
    }
    
    char actualPath[PATH_MAX];
    if(orig_dyld_fcntl(fd, F_GETPATH, actualPath) != -1)
    {
        dyld_hook_log("[hook_open:path] %s\n", actualPath);
        
        const char prefix[] = "/private/var/mobile/Containers/Data";
        if(strncmp(actualPath, prefix, sizeof(prefix) - 1) == 0)
        {
            /* need a new path */
            char newTmpPath[PATH_MAX];
            void *random;
            arc4random_buf(&random, sizeof(void*));
            snprintf(newTmpPath, sizeof(newTmpPath),  "%s/tmp/%016llx.dylib", mmap_sandbox_map_exec_allowed_path, (unsigned long long)random);  /* use tmp so iOS clears it automatically in LP home */
            
            int copyfd = open(newTmpPath, O_RDWR | O_CREAT | O_TRUNC, 0777);
            if(copyfd < 0)
            {
                goto skip_inode_setup;
            }
            
            if(fcopyfile(fd, copyfd, NULL, COPYFILE_DATA) < 0)
            {
                close(copyfd);
                unlink(newTmpPath);
            }
            
            close(copyfd);
            copyfd = open(newTmpPath, flags);
            if(copyfd < 0)
            {
                goto skip_inode_setup;
            }
            
            /* this to orient or selfs */
            ino_t inode = inode_for_fd(copyfd);
            inode_bank_put(inode, newTmpPath);
            inode_bank_set_redirect(inode, actualPath);
            
            close(fd);
            dup2(copyfd, fd);
            
        skip_inode_setup:
            
            if(cdhash_must_valid && !cdhash_verified)
            {
                /* no matter what this is not reentrant */
                cdhash_must_valid = false;
                cdhash_verified = false;
                
                lseek(fd, 0, SEEK_SET);
                /* need to get cdhash and then reset it's position */
                
                char *cdhash = cdhash_of_fd(fd);
                dyld_hook_log("[hook_open:cdhash] [nyxian cdhash verifier] (foundCdhash = %p, cdhash = %p)\n", cdhash, cdhash_data_container_match);
                
                /* match */
                if(cdhash == NULL ||
                   cdhash_data_container_match == NULL ||
                   memcmp(cdhash_data_container_match, cdhash, USER_FSIGNATURES_CDHASH_LEN) != 0)
                {
                    cdhash_verified = false;
                    dyld_hook_log("[hook_open:cdhash] [nyxian cdhash verifier] cdhash does not match, calling callback if givven\n");
                    
#if KSURFACE_DYLD_HARDENED_CDHASH_VERIFIER
                    open_hardlock = true;
#else
                    if(cdhash_verifier_failed_callback != NULL)
                    {
                        cdhash_verifier_failed_callback(fd, &open_hardlock);
                    }
#endif /* !KSURFACE_DYLD_HARDENED_CDHASH_VERIFIER */
                    
                    /* callback can set open hardlock */
                    if(open_hardlock)
                    {
                        dyld_hook_log("[hook_open:args] [error: hard locked]\n");
                        errno = EACCES;
                        close(fd);
                        fd = -1;
                    }
                }
                else
                {
                    dyld_hook_log("[hook_open:cdhash] [nyxian cdhash verifier] cdhash valid!\n");
                    cdhash_verified = true;
                    lseek(fd, 0, SEEK_SET);
                }
                
                /* reset position */
                free(cdhash);   /* free on macOS/iOS is NULL safe */
            }
        }
    }
    
just_return:
    dyld_hook_log("[hook_open:return] (fd = %d)\n", fd);
    return fd;
}


static const uint64_t fake_ino = 0x30a43;   /* some random inode i picked from a valid systems library */
static const time_t fake_time = 1700000000;

static int hook_fstat64(int fd,
                        struct stat *buf)
{
    dyld_hook_log("[hook_fstat64:args] (fd = %d, buf = %p)\n", fd, buf);
    int ret = orig_dyld_fstat64(fd, buf);
    if(ret == 0)
    {
        dyld_hook_log("[hook_fstat64] changing inode: 0x%llx -> 0x%llx\n", buf->st_ino, fake_ino);
        buf->st_ino = 0x30a43;  /* some inode */
        
        dyld_hook_log("[hook_fstat64] playing a bit with the clock =3\n");
        buf->st_mtimespec.tv_sec = fake_time;
        buf->st_mtimespec.tv_nsec = 0;
        buf->st_ctimespec.tv_sec = fake_time;
        buf->st_ctimespec.tv_nsec = 0;
        buf->st_birthtimespec.tv_sec = fake_time;
        buf->st_birthtimespec.tv_nsec = 0;
    }
    dyld_hook_log("[hook_fstat64:return] (ret = %d)\n", ret);
    return ret;
}

static int hook_stat64(const char *path,
                       struct stat *buf)
{
    dyld_hook_log("[hook_stat64:args] (path = %s, buf = %p)\n", path, buf);
    int ret = orig_dyld_stat64(path, buf);
    if(ret == 0)
    {
        dyld_hook_log("[hook_stat64] changing inode: 0x%llx -> 0x%llx\n", buf->st_ino, fake_ino);
        buf->st_ino = 0x30a43;  /* some inode */
        
        dyld_hook_log("[hook_stat64] playing a bit with the clock so DYLD thinks the file never changed =3 (1700000000)\n");
        buf->st_mtimespec.tv_sec = fake_time;
        buf->st_mtimespec.tv_nsec = 0;
        buf->st_ctimespec.tv_sec = fake_time;
        buf->st_ctimespec.tv_nsec = 0;
        buf->st_birthtimespec.tv_sec = fake_time;
        buf->st_birthtimespec.tv_nsec = 0;
    }
    dyld_hook_log("[hook_stat64:return] (ret = %d)\n", ret);
    return ret;
}

static HWHookThreadContextRef HWHookDlopenThreadContext(void)
{
    static HWHookThreadContextRef context = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        char *dyldBase = (char *)_alt_dyld_get_all_image_infos()->dyldImageLoadAddress;
        orig_dyld_fcntl = (void *)searchDyldFunction(dyldBase, (char*)fcntlSig, sizeof(fcntlSig));
        orig_dyld_open = (void *)searchDyldFunction(dyldBase, (char*)openSig, sizeof(openSig));
        orig_dyld_fstat64 = (void *)searchDyldFunction(dyldBase, (char*)fstat64Sig, sizeof(fstat64Sig));
        orig_dyld_stat64 = (void *)searchDyldFunction(dyldBase, (char*)stat64Sig, sizeof(stat64Sig));
        if(orig_dyld_fcntl == NULL || orig_dyld_open == NULL || orig_dyld_fstat64 == NULL || orig_dyld_stat64 == NULL)
        {
            return;
        }
        
        HWHookRef fcntlHook = HWHookCreateWithPointerToSymbol(kCFAllocatorDefault, orig_dyld_fcntl, hook_fcntl);
        if(fcntlHook == NULL)
        {
            return;
        }
        
        HWHookRef openHook = HWHookCreateWithPointerToSymbol(kCFAllocatorDefault, orig_dyld_open, hook_open);
        if(openHook == NULL)
        {
            CFRelease(fcntlHook);
            return;
        }
        
        HWHookRef fstat64Hook = HWHookCreateWithPointerToSymbol(kCFAllocatorDefault, orig_dyld_fstat64, hook_fstat64);
        if(fstat64Hook == NULL)
        {
            CFRelease(fcntlHook);
            CFRelease(openHook);
            return;
        }
        
        HWHookRef stat64Hook = HWHookCreateWithPointerToSymbol(kCFAllocatorDefault, orig_dyld_stat64, hook_stat64);
        if(fstat64Hook == NULL)
        {
            CFRelease(fcntlHook);
            CFRelease(openHook);
            CFRelease(fstat64Hook);
            return;
        }
        
        HWHookSetDisableContextHooksInFrame(fcntlHook, true);
        HWHookSetDisableContextHooksInFrame(openHook, true);
        HWHookSetDisableContextHooksInFrame(fstat64Hook, true);
        HWHookSetDisableContextHooksInFrame(stat64Hook, true);
        
        context = HWHookThreadContextCreate(kCFAllocatorDefault);
        if(context == NULL)
        {
            goto release_hooks;
        }
        
        if(!HWHookThreadContextAppendHook(context, fcntlHook) ||
           !HWHookThreadContextAppendHook(context, openHook) ||
           !HWHookThreadContextAppendHook(context, fstat64Hook) ||
           !HWHookThreadContextAppendHook(context, stat64Hook))
        {
            CFRelease(context);
        release_hooks:
            CFRelease(fcntlHook);
            CFRelease(openHook);
            CFRelease(fstat64Hook);
            CFRelease(stat64Hook);
            return;
        }
    });
    return context;
}

void *hook_dlopen(const char *path, int mode);

INTERPOSE(hook_dlopen, dlopen);

void *hook_dlopen(const char *path, int mode)
{
    inode_bank_init();
    
    void *(*darwin_dlopen)(const char *path, int mode) = _interpose_dlopen.replacee;
    dyld_hook_log("[hook_dlopen] %s\n", path);
    
    open_hardlock = false;
    HWHookThreadContextRef context = HWHookDlopenThreadContext();
    HWHookThreadContextEnter(context);  /* is nil safe, so it shall work anyways */
    void *ret = darwin_dlopen(path, mode);
    HWHookThreadContextExit(context);
    
    
    
    return ret;
}

void *dlopen_cdhash_verified(const char *path,
                             int flags,
                             const char *cdhash,
                             dlopen_cdhash_verifier_failed_callback_t callback)
{
    cdhash_verified = false;
    cdhash_must_valid = true;
    cdhash_data_container_match = cdhash;
    cdhash_verifier_failed_callback = callback;
    void *ret = hook_dlopen(path, flags);
    cdhash_verifier_failed_callback = NULL;
    cdhash_data_container_match = NULL;
    cdhash_must_valid = false;
    return ret;
}

__attribute__((constructor))
void LiveShimDlopenHookInit(void)
{
    const char *home = getenv("HOME");
    if(home == NULL)
    {
        return;
    }
    
    char *home_copy = strndup(home, MAXPATHLEN);
    if(home_copy == NULL)
    {
        return;
    }
    
    mmap_sandbox_map_exec_allowed_path = home_copy;
}

const char *dyld_get_mmap_sandbox_map_exec_allowed_path(void)
{
    return mmap_sandbox_map_exec_allowed_path;
}
