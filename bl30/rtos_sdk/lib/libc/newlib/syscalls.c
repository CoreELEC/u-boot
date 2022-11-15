/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <sys/stat.h>
#include <sys/time.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#if CONFIG_VFS
#include <sys_vfs.h>
#else
#include <stdarg.h>
#endif
#if defined(CONFIG_LOG_BUFFER)
#include "logbuffer.h"
#endif
#if defined(CONFIG_ARM64) || defined(CONFIG_ARM)
#include <serial.h>
#else
#include <uart.h>
#endif

#define UNUSED(x) ((void)(x))

int _isatty(int);
int _stat(const char *, struct stat *);
int _fstat(int, struct stat *);
int _close(int);
int _open(const char *, int, ...);
int _write(int, const void *, size_t);
_off_t _lseek(int, _off_t, int);
int _read(int, void *, size_t);

extern void __sinit(struct _reent *);
#define CHECK_INIT(ptr)                          \
	do {                                     \
		if ((ptr) && !(ptr)->__sdidinit) \
			__sinit(ptr);            \
	} while (0)

#define FILE_TYPE_MASK 0xFF
#define FILE_TYPE_STDIO 0x00
#define FILE_TYPE_VFS 0x01
#define FILE_TYPE_SOCKET 0x02
#define FILE_VALID 0x80000000

struct poslog {
	int handle;
	unsigned int flg;
};

/* Modified the MAX_OPEN_FILES from 512 to 8 inorder to reduce size */
#define MAX_OPEN_FILES 512
static struct poslog openfiles[MAX_OPEN_FILES];

static inline int _is_stdio(int fh)
{
	if (fh == STDERR_FILENO || fh == STDOUT_FILENO || fh == STDIN_FILENO)
		return 1;
	return 0;
}

static inline unsigned int _file_type(int fh)
{
	return (openfiles[fh].flg & FILE_TYPE_MASK);
}

static inline int remap_handle(int fh)
{
	CHECK_INIT(_REENT);

	if (_is_stdio(fh))
		return fh;

	if (fh < 0 || fh >= MAX_OPEN_FILES)
		return -1;

	if ((openfiles[fh].flg & FILE_VALID) == 0)
		return -1;

	return openfiles[fh].handle;
}

static inline int remap_handle_r(int fh, unsigned int type)
{
	int i;
	if (_is_stdio(fh))
		return fh;
	for (i = 0; i < MAX_OPEN_FILES; i++) {
		if (_is_stdio(i))
			continue;
		if ((openfiles[i].flg & FILE_VALID) && _file_type(i) == type &&
		    (openfiles[i].handle == fh))
			return i;
	}
	return -1;
}

static inline int alloc_file(unsigned int type)
{
	int i = 0;
	unsigned long flags;
	portIRQ_SAVE(flags);
	for (i = 0; i < MAX_OPEN_FILES; i++) {
		if (_is_stdio(i))
			continue;
		if ((openfiles[i].flg & FILE_VALID) == 0) {
			openfiles[i].flg = (FILE_VALID | (type & FILE_TYPE_MASK));
			goto _exit_fun;
		}
	}
	i = -1;
_exit_fun:
	portIRQ_RESTORE(flags);
	return i;
}

static inline void free_file(int fh)
{
	unsigned long flags;
	portIRQ_SAVE(flags);
	if (fh > 0 && fh < MAX_OPEN_FILES) {
		openfiles[fh].flg = 0;
		openfiles[fh].handle = 0;
	}
	portIRQ_RESTORE(flags);
}

int _fstat(int file, struct stat *st)
{
	int myhan = remap_handle(file);

	if (myhan < 0) {
		errno = EBADF;
		return -1;
	}

	memset(st, 0, sizeof(*st));

	if (_isatty(file)) {
		st->st_mode = S_IFCHR;
		if (file == STDIN_FILENO)
			st->st_mode |= S_IRUSR;
		else
			st->st_mode |= S_IWUSR;
		st->st_blksize = 1024;
		return 0;
	}

#if CONFIG_VFS
	if (_file_type(file) == FILE_TYPE_VFS)
		return vfs_fstat(myhan, st);
#endif
	return -1;
}

int _stat(const char *fname, struct stat *st)
{
#if CONFIG_VFS
	return vfs_stat(fname, st);
#else
	UNUSED(fname);
	UNUSED(st);
	return -1;
#endif
}

off_t _lseek(int file, off_t ptr, int dir)
{
	int myhan = remap_handle(file);

	UNUSED(ptr);
	UNUSED(dir);

	if (myhan < 0) {
		errno = EBADF;
		return -1;
	}

	if (_isatty(file)) {
		return 0;
	}

#if CONFIG_VFS
	if (_file_type(file) == FILE_TYPE_VFS)
		return vfs_lseek(myhan, ptr, dir);
#endif

	return -1;
}

int _isatty(int fd)
{
	return _is_stdio(fd);
}

int _read(int file, void *ptr, size_t len)
{
	int myhan = remap_handle(file);

	UNUSED(ptr);
	UNUSED(len);

	if (myhan < 0) {
		errno = EBADF;
		return -1;
	}

	if (file == STDERR_FILENO || file == STDOUT_FILENO) {
		errno = EBADF;
		return -1;
	}

	if (file == STDIN_FILENO) {
		errno = EBADF;
		return -1;
	}

#if CONFIG_VFS
	if (_file_type(file) == FILE_TYPE_VFS)
		return vfs_read(myhan, ptr, len);
#endif

#if ENABLE_MODULE_LWIP
	if (_file_type(file) == FILE_TYPE_SOCKET)
		return lwip_read(myhan, ptr, len);
#endif
	return -1;
}

int _open(const char *path, int flags, ...)
{
	int _fhandle = -1;
	int i = alloc_file(FILE_TYPE_VFS);
	va_list args;

	UNUSED(path);
	UNUSED(flags);

	if (i < 0) {
		errno = ENFILE;
		return -1;
	}

	va_start(args, flags);
#if CONFIG_VFS
	_fhandle = vfs_open(path, flags, va_arg(args, int));
#endif
	va_end(args);

	if (_fhandle < 0) {
		free_file(i);
		return -1;
	}

	openfiles[i].handle = _fhandle;
	return i;
}

int _write(int file, const void *ptr, size_t len)
{
	int myhan = remap_handle(file);
	signed char *pxNext = NULL;
	size_t i = 0;
	unsigned long flags;

	if (myhan < 0) {
		errno = EBADF;
		return -1;
	}

	if (file == STDIN_FILENO) {
		errno = EBADF;
		return -1;
	}

	if (file == STDERR_FILENO || file == STDOUT_FILENO) {
#if CONFIG_LOG_BUFFER
		if (logbuf_is_enable()) {
			return logbuf_output_len(ptr, len);
		}
#endif
		/* Send each character in the string, one at a time. */
		portIRQ_SAVE(flags);
		pxNext = (signed char *)ptr;
		while (i < len && *pxNext) {
#if defined(CONFIG_ARM64) || defined(CONFIG_ARM)
			if (*pxNext == '\n') {
				vSerialPutChar(ConsoleSerial, '\r');
			}
			vSerialPutChar(ConsoleSerial, *pxNext);
#else
			if (*pxNext == '\n')
				vUartPutc('\r');
			vUartPutc(*pxNext);
#endif
			pxNext++;
			i++;
		}
		portIRQ_RESTORE(flags);
		return i;
	}

#if CONFIG_VFS
	if (_file_type(file) == FILE_TYPE_VFS)
		return vfs_write(myhan, ptr, len);
#endif
#if ENABLE_MODULE_LWIP
	if (_file_type(file) == FILE_TYPE_SOCKET)
		return lwip_write(myhan, ptr, len);
#endif
	return -1;
}

int _close(int file)
{
	int myhan = remap_handle(file);

	if (myhan < 0)
		return 0;

	if (_isatty(file)) {
		return 0;
	}

#if CONFIG_VFS
	if (_file_type(file) == FILE_TYPE_VFS)
		vfs_close(myhan);
#endif

#if ENABLE_MODULE_LWIP
	if (_file_type(file) == FILE_TYPE_SOCKET)
		lwip_close(myhan);
#endif

	free_file(file);
	return 0;
}

int _gettimeofday(struct timeval *tv, struct timezone *tz)
{
	TickType_t tick_count;

	if (0 == configTICK_RATE_HZ)
		return -1;
	if ((NULL == tv) && (NULL == tz))
		return -1;

	if (NULL != tv) {
		tick_count = xTaskGetTickCount();
		tv->tv_sec = tick_count / configTICK_RATE_HZ;
		tv->tv_usec = (tick_count  % 1000) * configTICK_RATE_HZ;
	}

	if (NULL != tz) {
		tz->tz_dsttime = DST_NONE;
		tz->tz_minuteswest = 0;
	}

	return 0;
}

int __wrap__kill_r(int pid, int sig)
{
  UNUSED(pid);
  UNUSED(sig);
  errno = ENOTSUP;
  return -1;
}

int fcntl(int fd, int cmd, ...)
{
	int ret = -1;
	int myhan = remap_handle(fd);
	va_list args;

	if (myhan < 0) {
		errno = EBADF;
		return -1;
	}

	if (_isatty(fd)) {
		errno = EBADF;
		return -1;
	}

	va_start(args, cmd);
#if CONFIG_VFS
	if (_file_type(fd) == FILE_TYPE_VFS)
		ret = vfs_vfcntl(myhan, cmd, args);
#endif
#if ENABLE_MODULE_LWIP
	if (_file_type(fd) == FILE_TYPE_SOCKET)
		ret = lwip_fcntl(myhan, cmd, va_arg(args, int));
#endif
	va_end(args);
	return ret;
}

pid_t __wrap__getpid_r(void)
{
  return (pid_t)1;
}

int ioctl(int fd, int cmd, ...)
{
	int ret = -1;
	int myhan = remap_handle(fd);
	va_list args;

	if (myhan < 0) {
		errno = EBADF;
		return -1;
	}

	if (_isatty(fd)) {
		errno = EBADF;
		return -1;
	}

	va_start(args, cmd);
#if CONFIG_VFS
	if (_file_type(fd) == FILE_TYPE_VFS)
		ret = vfs_vioctl(myhan, cmd, args);
#endif
#if ENABLE_MODULE_LWIP
	if (_file_type(fd) == FILE_TYPE_SOCKET)
		ret = lwip_ioctl(myhan, cmd, va_arg(args, void *));
#endif
	va_end(args);
	return ret;
}

int fsync(int fd)
{
	int ret = -1;
	int myhan = remap_handle(fd);

	if (myhan < 0) {
		errno = EBADF;
		return -1;
	}

	if (_isatty(fd))
		return 0;

#if CONFIG_VFS
	if (_file_type(fd) == FILE_TYPE_VFS)
		ret = vfs_fsync(myhan);
#endif
	return ret;
}

#if CONFIG_VFS
int statfs(const char *path, struct statfs *buf)
{
	return vfs_statfs(path, buf);
}
#endif

pid_t _getpid(void)
{
	return (pid_t)1;
}

void *__wrap__malloc_r(struct _reent *reent, size_t size)
{
	UNUSED(reent);
	return pvPortMalloc(size);
}

void *__wrap__calloc_r(struct _reent *reent, size_t nmemb, size_t size)
{
	size_t total = nmemb * size;
	void *p;
	UNUSED(reent);
	if (total <= 0)
		return NULL;
	p = pvPortMalloc(total);
	if (!p)
		return NULL;
	return memset(p, 0, total);
}

void *__wrap__realloc_r(struct _reent *reent, void *ptr, size_t size)
{
	UNUSED(reent);
	return xPortRealloc(ptr, size);
}

void __wrap__free_r(struct _reent *reent, void *ptr)
{
	UNUSED(reent);
	vPortFree(ptr);
}

int _unlink(const char *__path)
{
#if CONFIG_VFS
	return vfs_remove(__path);
#else
	errno = ENOENT;
	return -1;
#endif
}

