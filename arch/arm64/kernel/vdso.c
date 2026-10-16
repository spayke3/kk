// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2016 MediaTek Inc.
 */

#include <linux/cache.h>
#include <linux/clocksource.h>
#include <linux/elf.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/gfp.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/timekeeper_internal.h>
#include <linux/vmalloc.h>

#include <asm/cacheflush.h>
#include <asm/signal32.h>
#include <asm/vdso.h>
#include <asm/vdso_datapage.h>

extern char vdso_start[], vdso_end[];
static unsigned long vdso_pages __ro_after_init;

/*
 * The vDSO data page.
 */
static union {
	struct vdso_data	data;
	u8			page[PAGE_SIZE];
} vdso_data_store __page_aligned_data;
struct vdso_data *vdso_data = &vdso_data_store.data;

#ifdef CONFIG_COMPAT
/*
 * Create and map the vectors page for AArch32 tasks.
 */
static struct page *vectors_page[1] __ro_after_init;

static int __init alloc_vectors_page(void)
{
	extern char __kuser_helper_start[], __kuser_helper_end[];
	extern char __aarch32_sigret_code_start[], __aarch32_sigret_code_end[];

	int kuser_sz = __kuser_helper_end - __kuser_helper_start;
	int sigret_sz = __aarch32_sigret_code_end - __aarch32_sigret_code_start;
	unsigned long vpage;

	vpage = get_zeroed_page(GFP_ATOMIC);

	if (!vpage)
		return -ENOMEM;

	/* kuser helpers */
	memcpy((void *)vpage + 0x1000 - kuser_sz, __kuser_helper_start,
		kuser_sz);

	/* sigreturn code */
	memcpy((void *)vpage + AARCH32_KERN_SIGRET_CODE_OFFSET,
               __aarch32_sigret_code_start, sigret_sz);

	flush_icache_range(vpage, vpage + PAGE_SIZE);
	vectors_page[0] = virt_to_page(vpage);

	return 0;
}
arch_initcall(alloc_vectors_page);

int aarch32_setup_vectors_page(struct linux_binprm *bprm, int uses_interp)
{
	struct mm_struct *mm = current->mm;
	unsigned long addr = AARCH32_VECTORS_BASE;
	static const struct vm_special_mapping spec = {
		.name	= "[vectors]",
		.pages	= vectors_page,

	};
	void *ret;

	if (mmap_write_lock_killable(mm))
		return -EINTR;
	current->mm->context.vdso = (void *)addr;

	/* Map vectors page at the high address. */
	ret = _install_special_mapping(mm, addr, PAGE_SIZE,
				       VM_READ|VM_EXEC|VM_MAYREAD|VM_MAYEXEC,
				       &spec);

	mmap_write_unlock(mm);

	return PTR_ERR_OR_ZERO(ret);
}
#endif /* CONFIG_COMPAT */

static int vdso_mremap(const struct vm_special_mapping *sm,
		struct vm_area_struct *new_vma)
{
	unsigned long new_size = new_vma->vm_end - new_vma->vm_start;
	unsigned long vdso_size = vdso_end - vdso_start;

	if (vdso_size != new_size)
		return -EINVAL;

	current->mm->context.vdso = (void *)new_vma->vm_start;

	return 0;
}

static int __vdso_init(enum arch_vdso_type arch_index)
{
	int i;
	struct page **vdso_pagelist;
	unsigned long pfn;

	if (memcmp(vdso_lookup[arch_index].vdso_code_start, "\177ELF", 4)) {
		pr_err("vDSO is not a valid ELF object!\n");
		return -EINVAL;
	}

	vdso_lookup[arch_index].vdso_pages = (
			vdso_lookup[arch_index].vdso_code_end -
			vdso_lookup[arch_index].vdso_code_start) >>
			PAGE_SHIFT;

	/* Allocate the vDSO pagelist, plus a page for the data. */
	vdso_pagelist = kcalloc(vdso_lookup[arch_index].vdso_pages + 1,
				sizeof(struct page *),
				GFP_KERNEL);
	if (vdso_pagelist == NULL)
		return -ENOMEM;

	/* Grab the vDSO data page. */
	vdso_pagelist[0] = phys_to_page(__pa_symbol(vdso_data));


	/* Grab the vDSO code pages. */
	pfn = sym_to_pfn(vdso_lookup[arch_index].vdso_code_start);

	for (i = 0; i < vdso_lookup[arch_index].vdso_pages; i++)
		vdso_pagelist[i + 1] = pfn_to_page(pfn + i);

	vdso_lookup[arch_index].dm->pages = &vdso_pagelist[0];
	vdso_lookup[arch_index].cm->pages = &vdso_pagelist[1];

	return 0;
}

static int __setup_additional_pages(enum arch_vdso_type arch_index,
				    struct mm_struct *mm,
				    struct linux_binprm *bprm,
				    int uses_interp)
{
	unsigned long vdso_base, vdso_text_len, vdso_mapping_len;
	void *ret;

	vdso_text_len = vdso_lookup[arch_index].vdso_pages << PAGE_SHIFT;
	/* Be sure to map the data page */
	vdso_mapping_len = vdso_text_len + PAGE_SIZE;

	vdso_base = get_unmapped_area(NULL, 0, vdso_mapping_len, 0, 0);
	if (IS_ERR_VALUE(vdso_base)) {
		ret = ERR_PTR(vdso_base);
		goto up_fail;
	}

	ret = _install_special_mapping(mm, vdso_base, PAGE_SIZE,
				       VM_READ|VM_MAYREAD,
				       vdso_lookup[arch_index].dm);
	if (IS_ERR(ret))
		goto up_fail;

	vdso_base += PAGE_SIZE;
	mm->context.vdso = (void *)vdso_base;
	ret = _install_special_mapping(mm, vdso_base, vdso_text_len,
				       VM_READ|VM_EXEC|
				       VM_MAYREAD|VM_MAYWRITE|VM_MAYEXEC,
				       vdso_lookup[arch_index].cm);
	if (IS_ERR(ret))
		goto up_fail;

	return 0;

up_fail:
	mm->context.vdso = NULL;
	return PTR_ERR(ret);
}

#ifdef CONFIG_COMPAT
/*
 * Create and map the vectors page for AArch32 tasks.
 */
#ifdef CONFIG_COMPAT_VDSO
static int aarch32_vdso_mremap(const struct vm_special_mapping *sm,
		struct vm_area_struct *new_vma)
{
	return __vdso_remap(ARM64_VDSO32, sm, new_vma);
}
#endif /* CONFIG_COMPAT_VDSO */

/*
 * aarch32_vdso_pages:
 * 0 - kuser helpers
 * 1 - sigreturn code
 * or (CONFIG_COMPAT_VDSO):
 * 0 - kuser helpers
 * 1 - vdso data
 * 2 - vdso code
 */
#define C_VECTORS	0
#ifdef CONFIG_COMPAT_VDSO
#define C_VVAR		1
#define C_VDSO		2
#define C_PAGES		(C_VDSO + 1)
#else
#define C_SIGPAGE	1
#define C_PAGES		(C_SIGPAGE + 1)
#endif /* CONFIG_COMPAT_VDSO */
static struct page *aarch32_vdso_pages[C_PAGES] __ro_after_init;
static struct vm_special_mapping aarch32_vdso_spec[C_PAGES] = {
	{
		.name	= "[vectors]", /* ABI */
		.pages	= &aarch32_vdso_pages[C_VECTORS],
	},
#ifdef CONFIG_COMPAT_VDSO
	{
		.name = "[vvar]",
	},
	{
		.name = "[vdso]",
		.mremap = aarch32_vdso_mremap,
	},
#else
	{
		.name	= "[sigpage]", /* ABI */
		.pages	= &aarch32_vdso_pages[C_SIGPAGE],
	},
#endif /* CONFIG_COMPAT_VDSO */
};

static int aarch32_alloc_kuser_vdso_page(void)
{
	extern char __kuser_helper_start[], __kuser_helper_end[];
	int kuser_sz = __kuser_helper_end - __kuser_helper_start;
	unsigned long vdso_page;

	if (!IS_ENABLED(CONFIG_KUSER_HELPERS))
		return 0;

	vdso_page = get_zeroed_page(GFP_ATOMIC);
	if (!vdso_page)
		return -ENOMEM;

	memcpy((void *)(vdso_page + 0x1000 - kuser_sz), __kuser_helper_start,
	       kuser_sz);
	aarch32_vdso_pages[C_VECTORS] = virt_to_page(vdso_page);
	flush_dcache_page(aarch32_vdso_pages[C_VECTORS]);
	return 0;
}

#ifdef CONFIG_COMPAT_VDSO
static int __aarch32_alloc_vdso_pages(void)
{
	int ret;

	vdso_lookup[ARM64_VDSO32].dm = &aarch32_vdso_spec[C_VVAR];
	vdso_lookup[ARM64_VDSO32].cm = &aarch32_vdso_spec[C_VDSO];

	ret = __vdso_init(ARM64_VDSO32);
	if (ret)
		return ret;

	return aarch32_alloc_kuser_vdso_page();
}
#else
static int __aarch32_alloc_vdso_pages(void)
{
	extern char __aarch32_sigret_code_start[], __aarch32_sigret_code_end[];
	int sigret_sz = __aarch32_sigret_code_end - __aarch32_sigret_code_start;
	unsigned long sigpage;
	int ret;

	sigpage = get_zeroed_page(GFP_ATOMIC);
	if (!sigpage)
		return -ENOMEM;

	memcpy((void *)sigpage, __aarch32_sigret_code_start, sigret_sz);
	aarch32_vdso_pages[C_SIGPAGE] = virt_to_page(sigpage);
	flush_dcache_page(aarch32_vdso_pages[C_SIGPAGE]);

	ret = aarch32_alloc_kuser_vdso_page();
	if (ret)
		free_page(sigpage);

	return ret;
}
#endif /* CONFIG_COMPAT_VDSO */

static int __init aarch32_alloc_vdso_pages(void)
{
	return __aarch32_alloc_vdso_pages();
}
arch_initcall(aarch32_alloc_vdso_pages);

static int aarch32_kuser_helpers_setup(struct mm_struct *mm)
{
	void *ret;

	if (!IS_ENABLED(CONFIG_KUSER_HELPERS))
		return 0;

	/*
	 * Avoid VM_MAYWRITE for compatibility with arch/arm/, where it's
	 * not safe to CoW the page containing the CPU exception vectors.
	 */
	ret = _install_special_mapping(mm, AARCH32_VECTORS_BASE, PAGE_SIZE,
				       VM_READ | VM_EXEC |
				       VM_MAYREAD | VM_MAYEXEC,
				       &aarch32_vdso_spec[C_VECTORS]);

	return PTR_ERR_OR_ZERO(ret);
}

#ifndef CONFIG_COMPAT_VDSO
static int aarch32_sigreturn_setup(struct mm_struct *mm)
{
	unsigned long addr;
	void *ret;

	addr = get_unmapped_area(NULL, 0, PAGE_SIZE, 0, 0);
	if (IS_ERR_VALUE(addr)) {
		ret = ERR_PTR(addr);
		goto out;
	}

	/*
	 * VM_MAYWRITE is required to allow gdb to Copy-on-Write and
	 * set breakpoints.
	 */
	ret = _install_special_mapping(mm, addr, PAGE_SIZE,
				       VM_READ | VM_EXEC | VM_MAYREAD |
				       VM_MAYWRITE | VM_MAYEXEC,
				       &aarch32_vdso_spec[C_SIGPAGE]);
	if (IS_ERR(ret))
		goto out;

	mm->context.vdso = (void *)addr;

out:
	return PTR_ERR_OR_ZERO(ret);
}
#endif /* !CONFIG_COMPAT_VDSO */

int aarch32_setup_additional_pages(struct linux_binprm *bprm, int uses_interp)
{
	struct mm_struct *mm = current->mm;
	int ret;

	if (mmap_write_lock_killable(mm))
		return -EINTR;

	ret = aarch32_kuser_helpers_setup(mm);
	if (ret)
		goto out;

#ifdef CONFIG_COMPAT_VDSO
	ret = __setup_additional_pages(ARM64_VDSO32,
				       mm,
				       bprm,
				       uses_interp);
#else
	ret = aarch32_sigreturn_setup(mm);
#endif /* CONFIG_COMPAT_VDSO */

out:
	mmap_write_unlock(mm);
	return ret;
}
#endif /* CONFIG_COMPAT */

static int vdso_mremap(const struct vm_special_mapping *sm,
		struct vm_area_struct *new_vma)
{
	return __vdso_remap(ARM64_VDSO, sm, new_vma);
}

/*
 * aarch64_vdso_pages:
 * 0 - vvar
 * 1 - vdso
 */
#define A_VVAR		0
#define A_VDSO		1
#define A_PAGES		(A_VDSO + 1)
static struct vm_special_mapping vdso_spec[A_PAGES] __ro_after_init = {
	{
		.name	= "[vvar]",
	},
	{
		.name	= "[vdso]",
		.mremap = vdso_mremap,
	},
};

static int __init vdso_init(void)
{
	int i;
	struct page **vdso_pagelist;
	unsigned long pfn;

	if (memcmp(vdso_start, "\177ELF", 4)) {
		pr_err("vDSO is not a valid ELF object!\n");
		return -EINVAL;
	}

	vdso_pages = (vdso_end - vdso_start) >> PAGE_SHIFT;

	/* Allocate the vDSO pagelist, plus a page for the data. */
	vdso_pagelist = kcalloc(vdso_pages + 1, sizeof(struct page *),
				GFP_KERNEL);
	if (vdso_pagelist == NULL)
		return -ENOMEM;

	/* Grab the vDSO data page. */
	vdso_pagelist[0] = phys_to_page(__pa_symbol(vdso_data));


	/* Grab the vDSO code pages. */
	pfn = sym_to_pfn(vdso_start);

	for (i = 0; i < vdso_pages; i++)
		vdso_pagelist[i + 1] = pfn_to_page(pfn + i);

	vdso_spec[0].pages = &vdso_pagelist[0];
	vdso_spec[1].pages = &vdso_pagelist[1];

	return 0;
}
arch_initcall(vdso_init);

int arch_setup_additional_pages(struct linux_binprm *bprm,
				int uses_interp)
{
	struct mm_struct *mm = current->mm;
	unsigned long vdso_base, vdso_text_len, vdso_mapping_len;
	void *ret;

	if (mmap_write_lock_killable(mm))
		return -EINTR;
	vdso_base = get_unmapped_area(NULL, 0, vdso_mapping_len, 0, 0);
	if (IS_ERR_VALUE(vdso_base)) {
		ret = ERR_PTR(vdso_base);
		goto up_fail;
	}
	ret = _install_special_mapping(mm, vdso_base, PAGE_SIZE,
				       VM_READ|VM_MAYREAD,
				       &vdso_spec[0]);
	if (IS_ERR(ret))
		goto up_fail;

	vdso_base += PAGE_SIZE;
	mm->context.vdso = (void *)vdso_base;
	ret = _install_special_mapping(mm, vdso_base, vdso_text_len,
				       VM_READ|VM_EXEC|
				       VM_MAYREAD|VM_MAYWRITE|VM_MAYEXEC,
				       &vdso_spec[1]);
	if (IS_ERR(ret))
		goto up_fail;

	mmap_write_unlock(mm);

	mmap_write_unlock(mm);
	return 0;

up_fail:
	mm->context.vdso = NULL;
	mmap_write_unlock(mm);
	return PTR_ERR(ret);
}

/*
 * Update the vDSO data page to keep in sync with kernel timekeeping.
 */
void update_vsyscall(struct timekeeper *tk)
{
	u32 use_syscall = !tk->tkr_mono.clock->archdata.vdso_direct;

	++vdso_data->tb_seq_count;
	smp_wmb();

	vdso_data->use_syscall			= use_syscall;
	vdso_data->xtime_coarse_sec		= tk->xtime_sec;
	vdso_data->xtime_coarse_nsec		= tk->tkr_mono.xtime_nsec >>
							tk->tkr_mono.shift;
	vdso_data->wtm_clock_sec		= tk->wall_to_monotonic.tv_sec;
	vdso_data->wtm_clock_nsec		= tk->wall_to_monotonic.tv_nsec;

	/* Read without the seqlock held by clock_getres() */
	WRITE_ONCE(vdso_data->hrtimer_res, hrtimer_resolution);

	if (!use_syscall) {
		/* tkr_mono.cycle_last == tkr_raw.cycle_last */
		vdso_data->cs_cycle_last	= tk->tkr_mono.cycle_last;
		vdso_data->raw_time_sec         = tk->raw_sec;
		vdso_data->raw_time_nsec        = tk->tkr_raw.xtime_nsec;
		vdso_data->xtime_clock_sec	= tk->xtime_sec;
		vdso_data->xtime_clock_nsec	= tk->tkr_mono.xtime_nsec;
		vdso_data->cs_mono_mult		= tk->tkr_mono.mult;
		vdso_data->cs_raw_mult		= tk->tkr_raw.mult;
		/* tkr_mono.shift == tkr_raw.shift */
		vdso_data->cs_shift		= tk->tkr_mono.shift;
	}

	smp_wmb();
	++vdso_data->tb_seq_count;
}

void update_vsyscall_tz(void)
{
	vdso_data->tz_minuteswest	= sys_tz.tz_minuteswest;
	vdso_data->tz_dsttime		= sys_tz.tz_dsttime;
}
