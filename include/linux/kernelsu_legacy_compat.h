/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_KERNELSU_LEGACY_COMPAT_H
#define _LINUX_KERNELSU_LEGACY_COMPAT_H

/* KernelSU-Next legacy uses helpers introduced after Linux 4.4. */
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, a) ((x) & ~((typeof(x))(a) - 1))
#endif

#ifndef __nocfi
#define __nocfi
#endif

/* full_name_hash() gained a 3rd "salt" arg upstream; 4.4 only has the old
 * 2-arg form, so dispatch on argument count to keep old call sites working. */
#define __ksu_full_name_hash_2(name, len) full_name_hash(name, len)
#define __ksu_full_name_hash_3(salt, name, len) full_name_hash(name, len)
#define __ksu_full_name_hash_pick(_1, _2, _3, NAME, ...) NAME
#define full_name_hash(...)                                        \
	__ksu_full_name_hash_pick(__VA_ARGS__, __ksu_full_name_hash_3, \
							  __ksu_full_name_hash_2)(__VA_ARGS__)

/* kvmalloc()/kvfree() don't exist before Linux 4.12; provide them tree-wide
 * for drivers/ files that don't already have a fallback. */
#ifndef kvmalloc
#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
static inline void *__ksu_compat_kvmalloc(size_t size, gfp_t flags)
{
	void *ret = kmalloc(size, flags | __GFP_NOWARN | __GFP_NORETRY);

	if (!ret)
		ret = vmalloc(size);
	return ret;
}
#define kvmalloc __ksu_compat_kvmalloc
#endif

#ifndef kvfree
static inline void __ksu_compat_kvfree(const void *addr)
{
	if (is_vmalloc_addr(addr))
		vfree((void *)addr);
	else
		kfree(addr);
}
#define kvfree __ksu_compat_kvfree
#endif

/* kernel_write()/kernel_read() gained a "pos by pointer" form upstream; pick
 * KSU's compat wrapper by pos's type so old by-value callers stay untouched. */
#include <linux/types.h>
struct file;
extern ssize_t ksu_kernel_write_compat(struct file *p, const void *buf,
									   size_t count, loff_t *pos);
extern ssize_t ksu_kernel_read_compat(struct file *p, void *buf, size_t count,
									  loff_t *pos);

#define kernel_write(file, buf, count, pos)                           \
	__builtin_choose_expr(                                            \
		__builtin_types_compatible_p(__typeof__(pos), loff_t *),      \
		ksu_kernel_write_compat(file,                                 \
								(const void *)(unsigned long)(buf),   \
								count,                                \
								(loff_t *)(unsigned long)(pos)),      \
		kernel_write(file, (const char *)(unsigned long)(buf), count, \
					 (loff_t)(unsigned long)(pos)))

#define kernel_read(file, a2, a3, a4)                             \
	__builtin_choose_expr(                                        \
		__builtin_types_compatible_p(__typeof__(a4), loff_t *),   \
		ksu_kernel_read_compat(file, (void *)(unsigned long)(a2), \
							   (size_t)(unsigned long)(a3),       \
							   (loff_t *)(unsigned long)(a4)),    \
		kernel_read(file, (loff_t)(unsigned long)(a2),            \
					(char *)(unsigned long)(a3),                  \
					(unsigned long)(a4)))

#endif /* _LINUX_KERNELSU_LEGACY_COMPAT_H */
