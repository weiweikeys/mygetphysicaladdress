#include <linux/kernel.h>//用來printk
#include <linux/syscalls.h>
#include <linux/mm.h>       // Data structure and functions related to memory management
#include <asm/pgtable.h>
#include <linux/pid.h>      // To obtain process info
#include <linux/sched.h>    // Definition of process control area
#include <linux/highmem.h>  // Functions related to page table operations
#include <asm/page.h>       // Functions related to page table operations

SYSCALL_DEFINE1(my_get_physical_addresses, void *, vaddr) {
    struct task_struct *task = current; // 獲取當前進程
    struct mm_struct *mm = task->mm;     // 獲取當前進程的內存管理結構
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    struct page *page;
    unsigned long physical_address;
    //none 和 bad 是內核頁表操作中的兩個重要概念，用於檢查頁表條目的狀態，分別代表頁表條目的「不存在」和「無效」。
    
    
    //查詢pgd
    pgd = pgd_offset(mm, (unsigned long)vaddr);
    if (pgd_none(*pgd) || pgd_bad(*pgd)) {
	    printk("pgd not found!!!\n");
        return 0;
    }
   
    //查詢p4d  X86_64架構 p4d 是空殼，可以省略，但寫通用程式碼時不建議省。
    p4d = p4d_offset(pgd, (unsigned long)vaddr);
    if (p4d_none(*p4d) || p4d_bad(*p4d)) {
	    printk("p4d not found!!!\n");
        return 0;
    }
 /*
 Linux kernel 為了支援多種硬體架構，
 設計了五層分頁接口（pgd → p4d → pud → pmd → pte）作為統一的抽象層級。
 然而，實際的物理分頁層數依架構而異。
 例如，x86_64 架構只有四層分頁，p4d 層通常與 pud 層合併或重定義為同一層，並無獨立實體頁表；
 而某些支援五層分頁的 CPU 則會有獨立的 p4d 層。
 這種設計使 Linux 能跨架構統一管理分頁表，提升可維護性與移植性。
 */
    //查詢pud
    pud = pud_offset(p4d, (unsigned long)vaddr);
    if (pud_none(*pud) || pud_bad(*pud)) {
	    printk("pud not found!!!\n");
        return 0;
    }
    
    //查詢pmd
    pmd = pmd_offset(pud, (unsigned long)vaddr);
    if (pmd_none(*pmd) || pmd_bad(*pmd)) {
	    printk("pmd not found!!!\n");
        return 0;
    }
   

   //查詢pte
    pte = pte_offset_map(pmd, (unsigned long)vaddr);
    if (pte_none(*pte)) {
	    printk("pte not found!!!\n");
        return 0;
    }
   

     //取得當前process對應的page
    page = pte_page(*pte);
    if (!page) {
	    printk("page not found!!!\n");
        return 0;
    }
   

    //page丟進page_to_pfn()得到頁框索引，然後向左移12bits
    //PFN 是從 0 開始算的頁框編號。
    //PFN = 0 → 第 1 個物理頁框，地址範圍是 0x0000 ~ 0x0FFF（假設頁大小 4KB）。
    //PFN = 1 → 第 2 個物理頁框，地址範圍是 0x1000 ~ 0x1FFF。
    //~PAGE_MASK：將 PAGE_MASK 取反。page大小是 4KB，PAGE_MASK 就是 0xFFFFF000，
    //#define PAGE_MASK (~(PAGE_SIZE-1)) -> ~(PAGE_SIZE-1)=~(4095)=~(0X00000FFF)=0XFFFFF000
    //而 ~PAGE_MASK 就會是 0x00000FFF。這樣一來，vaddr & ~PAGE_MASK 只保留了virtual memory 的 Offset。
    //index跟virtual memory的offset做OR 相當於合併得到實體記憶體位置
    physical_address = (page_to_pfn(page) << PAGE_SHIFT) | ((unsigned long)vaddr & ~PAGE_MASK);
    
    return (void *)physical_address;

}

