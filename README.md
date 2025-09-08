# ****Linux作業系統Project1:my_get_physical_addresses&verify_lazy_allocation****


## 建立system call

1.在linux-5.15.137裡面建立資料夾
```
mkdir my_get_physical_addresses
```
![Image](https://github.com/user-attachments/assets/c2f683e3-0b3a-4f9e-a11f-6607c62af5f1)

2.進入所建的資料夾
```
cd my_get_physical_addresses
```
![Image](https://github.com/user-attachments/assets/4ef6ea7e-19ba-49f9-834b-47b8e6d591d0)


3.新增system call
```
touch my_get_physical_addresses.c
```
![image](https://github.com/user-attachments/assets/3644919e-227e-4d9b-a18d-9c3e08ca15bf)

4.編輯my_get_physical_addresses.c
```
vim my_get_physical_addresses.c
```
## 以下為my_get_physical_addresses.c的程式碼:
```C
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


```
## Linear address 轉成 Physical address 流程圖
![1751887114130](https://hackmd.io/_uploads/rJHQTQtrxe.jpg)

![1751875950917](https://hackmd.io/_uploads/HyBq--YBge.jpg)

## trace code


* (1)
https://elixir.bootlin.com/linux/v5.15.137/source/include/linux/syscalls.h#L223
這個連結可以知道SYSCALL_DEFINEx




* (2)
https://elixir.bootlin.com/linux/v5.15.137/source/arch/x86/include/asm/current.h
這個連結可以知道
#define current get_current()
將 current 定義為 get_current() 函數的結果，這樣在程式碼中可以簡單地通過 current 獲取當前的任務結構指標。


* (3)
https://elixir.bootlin.com/linux/v5.15.137/source/include/linux/sched.h#L758
這個連結可以知道task_struct結構裡面有mm_struct

![image](https://hackmd.io/_uploads/HJ2a4UD-1x.png)



**進入mm找以下連結:**
https://elixir.bootlin.com/linux/v5.15.137/source/include/linux/mm_types.h#L779
找到pgd_t *pgd;
![image](https://hackmd.io/_uploads/B1zBv8P-Jl.png)


**進入pgd 找到以下兩個連結:**
![image](https://hackmd.io/_uploads/B1WviUvW1e.png)
![messageImage_1731219002165](https://hackmd.io/_uploads/rySj0Ta-yl.jpg)



* (4)
https://elixir.bootlin.com/linux/v5.15.137/source/arch/x86/include/asm/pgtable.h
https://elixir.bootlin.com/linux/v5.15.137/source/include/linux/pgtable.h
這兩個連結可以知道
提供頁表操作的輔助函數 (pgd_offset, p4d_offset, pud_offset,pmd_offset,pgd_none pgd_bad,p4d_none p4d_bad,pud_none pud_bad,pmd_none pmd_bad,pte_none,pte_offset_map)

    (一). none
pgd_none(*pgd), p4d_none(*p4d), 等等。
這些函數用於檢查特定頁表條目是否「不存在」。
當頁表條目無法映射到有效的物理頁時，內核會設置該條目為 none 狀態。
使用 none 檢查可以避免訪問無效的頁表條目。
(二). bad
pgd_bad(*pgd), p4d_bad(*p4d), 等等。
這些函數用於檢查頁表條目是否「無效」。
無效的頁表條目可能是由於內核或硬件錯誤而損壞的。
通過 bad 檢查可以確保頁表條目是正常的，以防止訪問不正確的內存位置。






* (5)
https://elixir.bootlin.com/linux/v5.15.137/source/arch/x86/include/asm/page_types.h
這個連結可以知道
#define PAGE_SHIFT		12
#define PAGE_SIZE		(_AC(1,UL) << PAGE_SHIFT)
#define PAGE_MASK		(~(PAGE_SIZE-1))



* (6)
https://elixir.bootlin.com/linux/v5.15.137/source/include/asm-generic/memory_model.h#L53
#define pfn_to_page __pfn_to_page
#define __page_to_pfn(page)	((unsigned long)((page) - mem_map) + \
				 ARCH_PFN_OFFSET)
1. page
是一個指標，指向一個 struct page 結構體，代表一個物理記憶體頁框。

2. mem_map
是一個全局陣列指標，指向記憶體管理中所有物理頁框的 struct page 陣列起點，這個陣列連續排列所有物理頁框的結構。

3. (page - mem_map)
指標相減，結果是「元素數量」。
是兩個指標相減，計算 page 在 mem_map 陣列中的偏移位置，
意味著從物理記憶體起點算起，這是第幾個頁框（索引）。

4. (unsigned long)
將指標偏移量轉成無號整數型態，方便後續運算。

5. +ARCH_PFN_OFFSET
這是架構相關的物理頁框偏移量，
因為有些硬體平台物理頁框編號起點不是從 0 開始，
所以要加上這個偏移以求精確。


---

5.編輯完my_get_physical_addresses.c程式碼保存並退出
在my_get_physical_addresses的資料夾下建立Makefile
建立完並編輯，內容為:

![Image](https://github.com/user-attachments/assets/c2f683e3-0b3a-4f9e-a11f-6607c62af5f1)

6.回到linux-5.15.137並編輯Makefile

![Image](https://github.com/user-attachments/assets/f0f60df1-7257-44d3-a444-aa51c4562171)

找到core-y並在最後面新增my_get_physical_addresses

![image](https://hackmd.io/_uploads/SJxl8wPWyg.png)

7.在syscall_64.tbl中添加新的systemcall
為450     common  my_get_physical_addresses sys_my_get_physical_addresses
```
vim arch/x86/entry/syscalls/syscall_64.tbl
```
![image](https://hackmd.io/_uploads/ByOoUvvZ1g.png)

8.編輯linux-5.15.137/include/linux/syscalls.h
```
vim include/linux/syscalls.h
    
#在檔案最下方加入
asmlinkage long sys_my_get_physical_addresses(void *);    //要求的檔案型別
```
![image](https://hackmd.io/_uploads/S1XowvvWyg.png)

9.編譯kernel
```
make -j12
```


10.安裝kernel
```
make modules_install -j12
make install -j12
```

## Question1

```C
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>             
#include <sys/types.h>
#include <sys/wait.h>

void *my_get_physical_addresses(void *addr){

    void *paddr;

    paddr = (void *)syscall(450, addr);

    return paddr;
}


int global_a = 123;

void hello(void) {
    printf("======================================================================================================\n");
}

int main() {
    int loc_a;
    void *parent_use, *child_use;

    printf("=========================== Before Fork ==================================\n");
    parent_use = my_get_physical_addresses(&global_a);
    printf("pid=%d: global variable global_a:\n", getpid());
    printf("Offset of logical address:[%p]   Physical address:[%p]\n", &global_a, parent_use);
    printf("========================================================================\n");
    printf("\n");

    if (fork()) {  
        /* parent code */
        printf("vvvvvvvvvvvvvvvvvvvvvvvvvv  After Fork by parent  vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n");
        parent_use = my_get_physical_addresses(&global_a);
        printf("pid=%d: global variable global_a:\n", getpid());
        printf("******* Offset of logical address:[%p]   Physical address:[%p]\n", &global_a, parent_use);
        printf("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n");
        printf("\n");
        wait(NULL); 
    } else {  
        /* child code */
        printf("llllllllllllllllllllllllll  After Fork by child  llllllllllllllllllllllllllllllll\n");
        child_use = my_get_physical_addresses(&global_a);
        printf("******* pid=%d: global variable global_a:\n", getpid());
        printf("******* Offset of logical address:[%p]   Physical address:[%p]\n", &global_a, child_use);
        printf("llllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll\n");
        printf("____________________________________________________________________________\n");
        printf("\n");

        /* trigger CoW (Copy on Write) */
        global_a = 789;

        printf("iiiiiiiiiiiiiiiiiiiiiiiiii  Test copy on write in child  iiiiiiiiiiiiiiiiiiiiiiii\n");
        child_use = my_get_physical_addresses(&global_a);
        printf("******* pid=%d: global variable global_a:\n", getpid());
        printf("******* Offset of logical address:[%p]   Physical address:[%p]\n", &global_a, child_use);
        printf("iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii\n");
        printf("____________________________________________________________________________\n");
        printf("\n");
        sleep(1000);
    }
}
```

### Q1 result
![Image](https://github.com/user-attachments/assets/dea30777-8f1c-4d3b-af96-4c4e11307eb3)

* PID：pid=1295是Parent Process的pid，表示目前運行的 Parent Process。
* PID：pid=1296是Child Process的pid。
* Virtual Address：0x55f574189010 是global variable global_a 對應的Virtual Address，為Process內存中Logical Memory位置，由系統分配。
* Physical Address：0x11300e010是global_a對應的 Physical Address，為Physical Memory的實際地址。
(每次執行時Physical Address與Virtual Address不固定，主要原因與系統的安全性機制及內存管理策略有關)

以上程式碼表現出copy-on-write的影響，從result可以看見在Before Fork、After Fork by parent、After by child中，global_a的logical address和physical address都相同，而最後copy on write in child的Physical Address 發生變化：當Child Process修改了 global_a 的值後，對應的 Physical Address 變成了0x115493010，表示觸發了Copy on Write，在 Child Process 中，當 global_a 被修改時，Kernel 會分配一個新的 Physical Address，這樣 Parent Process 和 Child Process 不再使用同一個 Physical Memory Page。因此 Child Process 的 Physical Address 變成0x115493010。 


### Bonus
exe.c
```C
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>             
#include <sys/types.h>
#include <sys/wait.h>

void *my_get_physical_addresses(void *addr){

    void *paddr;

    paddr = (void *)syscall(450, addr);

    return paddr;
}


int global_a = 123;

void hello(void) {
    printf("======================================================================================================\n");
}

int main() {
    int loc_a;
    void *parent_use, *child_use;

    printf("=========================== Before Fork ==================================\n");
    parent_use = my_get_physical_addresses(&global_a);
    printf("pid=%d: global variable global_a:\n", getpid());
    printf("Offset of logical address:[%p]   Physical address:[%p]\n", &global_a, parent_use);
    printf("========================================================================\n");
    printf("\n");

    if (fork()) {  
        /* parent code */
        printf("vvvvvvvvvvvvvvvvvvvvvvvvvv  After Fork by parent  vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n");
        parent_use = my_get_physical_addresses(&global_a);
        printf("pid=%d: global variable global_a:\n", getpid());
        printf("******* Offset of logical address:[%p]   Physical address:[%p]\n", &global_a, parent_use);
        printf("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n");
        printf("\n");
        wait(NULL); 
    } else { 
        printf("****************before execlp******************")
        printf("******* pid=%d: global variable global_a:\n", getpid());
        printf("\n");
        execlp("/home/clayton/exe2","exe2",NULL);
	/* child code */
        printf("llllllllllllllllllllllllll  After Fork by child  llllllllllllllllllllllllllllllll\n");
        child_use = my_get_physical_addresses(&global_a);
        printf("******* pid=%d: global variable global_a:\n", getpid());
        printf("******* Offset of logical address:[%p]   Physical address:[%p]\n", &global_a, child_use);
        printf("llllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll\n");
        printf("____________________________________________________________________________\n");
        printf("\n");

        /* trigger CoW (Copy on Write) */
        global_a = 789;

        printf("iiiiiiiiiiiiiiiiiiiiiiiiii  Test copy on write in child  iiiiiiiiiiiiiiiiiiiiiiii\n");
        child_use = my_get_physical_addresses(&global_a);
        printf("******* pid=%d: global variable global_a:\n", getpid());
        printf("******* Offset of logical address:[%p]   Physical address:[%p]\n", &global_a, child_use);
        printf("iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii\n");
        printf("____________________________________________________________________________\n");
        printf("\n");
        sleep(1000);
    }
}

```

exe2.c
```C
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>             
#include <sys/types.h>
#include <sys/wait.h>
void *my_get_physical_addresses(void *addr){

    void *paddr;

    paddr = (void *)syscall(450, addr);

    return paddr;
}


int global_a = 123;

int main(){
	int loc_a;
    	void *parent_use, *child_use;
	printf("llllllllllllllllllllllllll  After Fork by child  llllllllllllllllllllllllllllllll\n");
        child_use = my_get_physical_addresses(&global_a);
        printf("******* pid=%d: global variable global_a:\n", getpid());
        printf("******* Offset of logical address:[%p]   Physical address:[%p]\n", &global_a, child_use);
        printf("llllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll\n");
        printf("____________________________________________________________________________\n");
        printf("\n");
}

```

### Bonus Result
![Image](https://github.com/user-attachments/assets/e284eb80-a3c3-4073-bfa8-38f5b92080ee)

* execlp會取代目前執行的代碼，執行新的程式碼，而execlp 是在當前進度上執行另一個程式，並不會創建新的ID，所以pid保持不變。
* 但執行execlp時系統會重建目前的記憶體空間以載入新程式的檔案，所以logical address會有變化。



## Question2


建立load.c
```
vim load.c
```

load.c的內容
```C++=
#include <stdio.h>
#include <unistd.h>
#define MY_SYSCALL_NUM 450

void *my_get_physical_addresses(void *addr){

    void *paddr;

    paddr = (void *)syscall(MY_SYSCALL_NUM, addr);

    if (paddr == 0) {
        printf("failure: cannot get physical address\n");
        return NULL;
    } else {
        printf("The virtual address %p ---> The physical address: %p \n", addr, paddr);
    }

    return paddr;
}


int a[2000000]; 
                 
int main(){ 
    int loc_a;
    void *phy_add;  

    phy_add=my_get_physical_addresses(&a[0]);
    printf("global element a[0]:\n");  
    printf("Offest of logical address:[%p]   Physical address:[%p]\n", &a[0], phy_add);              
    printf("========================================================================\n"); 
    
    phy_add=my_get_physical_addresses(&a[1999999]);
    printf("global element a[1999999]:\n");  
    printf("Offest of logical address:[%p]   Physical address:[%p]\n", &a[1999999], phy_add);              
    printf("========================================================================\n");  
}

```

 編譯 load.c
```
gcc -o load load.c
```
 執行 load data
```
./load
```
### Q2 Result
![image](https://hackmd.io/_uploads/H1A78-Pb1l.png)



* 從結果可以看出只有a[0]有Physical address
a[1999999]並沒有Physical address
便可得知loader並沒有在一開始就將Physical address分配給陣列中的所有元素

---

### Bonus
```C++=
#include <stdio.h>
#include <unistd.h>
#define MY_SYSCALL_NUM 450

void *my_get_physical_addresses(void *addr){

    void *paddr;

    paddr = (void *)syscall(MY_SYSCALL_NUM, addr);

    if (paddr == 0) {
        printf("failure: cannot get physical address\n");
        return NULL;
    } else {
        printf("The virtual address %p ---> The physical address: %p \n", addr, paddr);
    }

    return paddr;
}

int a[2000000];

int main() {
    int loc_a;
    void *phy_add;

    for (int i = 0; i < 2000000; i++) {
        phy_add = my_get_physical_addresses(&a[i]);
        if (phy_add == NULL) {
            printf("\nLazy allocation end at ");
            printf("element: a[%d]\n\n", i);
            break;
        }
    }

    phy_add = my_get_physical_addresses(&a[0]);
    printf("global element a[0]:\n");
    printf("Offset of logical address:[%p]   Physical address:[%p]\n", &a[0], phy_add);
    printf("========================================================================\n");

    phy_add = my_get_physical_addresses(&a[1007]);
    printf("global element a[1007]:\n");
    printf("Offset of logical address:[%p]   Physical address:[%p]\n", &a[1007], phy_add);
    printf("========================================================================\n");

    phy_add = my_get_physical_addresses(&a[1008]);
    printf("global element a[1008]:\n");
    printf("Offset of logical address:[%p]   Physical address:[%p]\n", &a[1008], phy_add);
    printf("========================================================================\n");

    phy_add = my_get_physical_addresses(&a[1999999]);
    printf("global element a[1999999]:\n");
    printf("Offset of logical address:[%p]   Physical address:[%p]\n", &a[1999999], phy_add);
    printf("========================================================================\n");
}

```

### Bonus Result
![image](https://hackmd.io/_uploads/Sk79qzwZkl.png)


* 由結果可得知因為Lazy allocation的原因，所以系統只分配實體記憶體空間到a[1007]，從a[1008]開始就沒有被分配到實體記憶體空間

* Lazy allocation:當程式宣告變數或請求記憶體空間時，系統並不會立即分配實體記憶體空間，而是等到真正使用該記憶體時才進行分配。

* 主要目的是提高效能和節省記憶體資源，當程式可能不會使用到所有宣告的記憶體時。透過Lazy allocation，系統可以避免不必要的記憶體佔用，並減少啟動時間或初始化時間

---

當我們將"1"這個值存到a[1008]時
可以看到a[1008]就被分配到實體記憶體空間了:

![image](https://hackmd.io/_uploads/rkRHxlabkg.png)



一直分配直到a[2032]才沒有被分配到:

![image](https://hackmd.io/_uploads/B1Nrkga-yg.png)

```C++=
#include <stdio.h>
#include <unistd.h>
#define MY_SYSCALL_NUM 450

void *my_get_physical_addresses(void *addr){

    void *paddr;

    paddr = (void *)syscall(MY_SYSCALL_NUM, addr);

    if (paddr == 0) {
        printf("failure: cannot get physical address\n");
        return NULL;
    } else {
        printf("The virtual address %p ---> The physical address: %p \n", addr, paddr);
    }

    return paddr;
}

int a[2000000];

int main() {
    int loc_a;
    void *phy_add;

    for (int i = 0; i < 2000000; i++) {
        phy_add = my_get_physical_addresses(&a[i]);
        if (phy_add == NULL) {
            printf("\nLazy allocation end at ");
            printf("element: a[%d]\n\n", i);
            break;
        }
    }

    phy_add = my_get_physical_addresses(&a[0]);
    printf("global element a[0]:\n");
    printf("Offset of logical address:[%p]   Physical address:[%p]\n", &a[0], phy_add);
    printf("========================================================================\n");

    phy_add = my_get_physical_addresses(&a[1007]);
    printf("global element a[1007]:\n");
    printf("Offset of logical address:[%p]   Physical address:[%p]\n", &a[1007], phy_add);
    printf("========================================================================\n");

    phy_add = my_get_physical_addresses(&a[1008]);
    printf("global element a[1008]:\n");
    printf("Offset of logical address:[%p]   Physical address:[%p]\n", &a[1008], phy_add);
    printf("========================================================================\n");

    phy_add = my_get_physical_addresses(&a[1999999]);
    printf("global element a[1999999]:\n");
    printf("Offset of logical address:[%p]   Physical address:[%p]\n", &a[1999999], phy_add);
    printf("========================================================================\n");
}

//第58~79行的code為本次測試新增的
    printf("a[1008]=1\n");
    a[1008]=1;
    phy_add = my_get_physical_addresses(&a[1008]);
    printf("global element a[1008]:\n");
    printf("Offset of logical address:[%p]   Physical address:[%p]\n", &a[1008],    phy_add);
    printf("========================================================================\n");



    for (int i =1009; i < 2000000; i++) {
        phy_add = my_get_physical_addresses(&a[i]);
        if (phy_add == NULL) {
            printf("\nLazy allocation end at ");
            printf("element: a[%d]\n\n", i);
            break;
        }
    }

    phy_add = my_get_physical_addresses(&a[2032]);
    printf("global element a[2032]:\n");
    printf("Offset of logical address:[%p]   Physical address:[%p]\n", &a[2032], phy_add);
    printf("========================================================================\n");
```

## Note
### What is Systemcall?



在 Linux 系統中，系統呼叫（System Call）是應用程式與作業系統內核（Kernel）之間的一種介面，讓應用程式可以向內核請求執行特定的操作，例如檔案操作、網路通訊、記憶體管理、進程控制等。由於內核掌控了系統資源的管理與安全性，應用程式無法直接存取這些資源，必須透過系統呼叫來請求內核幫助完成。

當應用程式進行系統呼叫時，會觸發一個軟體中斷，將控制權轉移給內核，使內核可以在受保護的環境下執行指定操作。這樣的機制確保了系統的穩定性與安全性，避免應用程式直接操作硬體資源可能帶來的風險。

常見的 Linux 系統呼叫包括：

open()：開啟檔案
read()：讀取檔案內容
write()：寫入檔案內容
fork()：建立子進程
exec()：執行其他程式
exit()：結束程式
透過這些系統呼叫，應用程式能夠間接地控制硬體資源，並實現多種功能。



---


### What is Task_struct?



task_struct 是 Linux 核心中用來表示每個process的資料結構，它包含了進程控制塊（PCB）中所有用於管理和排程進程的關鍵資訊。根據你列出的理論架構，這些要素確實是 task_struct 的核心組成部分。以下是每個欄位在 task_struct 中的作用說明：

(1)進程狀態：記錄process的狀態，例如是否正在運行、等待、已終止等。在 task_struct 中，透過 state 欄位來表示進程的當前狀態，使用不同的標誌位來表示各類狀態（例如 TASK_RUNNING、TASK_INTERRUPTIBLE 等）。

(2)排程資訊：排程資訊決定process的優先級、排程策略等。在 task_struct 中包含 sched_entity 結構體，用於儲存排程實體的資訊，涵蓋排程策略、優先級、進程在隊列中的位置等，以實現多種排程演算法。

(3)進程通訊狀況：進程間通訊（IPC）資訊，例如信號處理和文件描述符等。在 task_struct 中，有專門的欄位（如 signal、files 等）來管理信號、開啟的文件及 IPC 相關的資源。

(4)進程關係指標：為了將進程插入進程樹，task_struct 具有指向父進程、子進程以及兄弟進程的指標欄位，如 parent、children、sibling 等。這些指標將每個進程的結構連接在一起，形成父子兄弟關係的進程樹。

(5)時間資訊：該欄位用來記錄進程執行的時間資訊，支援計算 CPU 時間分配和排程決策。task_struct 包含 utime（用戶態時間）、stime（核心態時間）等欄位。

(6)標號：用於表示進程的身份和歸屬關係，包括 pid、tgid（執行緒組 ID）、uid（用戶 ID）等欄位，以幫助操作系統管理不同用戶的權限和資源。

(7)文件資訊：記錄進程可以讀寫的文件、開啟的文件描述符等。task_struct 透過 files 指標指向一個 files_struct，其中儲存了進程的文件描述符表。

(8)進程上下文與核心上下文：包括用戶態與核心態的上下文資訊，這些資訊是進程切換時保存和恢復所需的內容。task_struct 具有 mm 欄位（指向記憶體管理資訊的指標）和 active_mm（當前使用的記憶體資訊）以區分用戶態與核心態的上下文。

(9)處理器上下文：用於保存處理器的暫存器狀態，以便在上下文切換時恢復該進程的運行狀態。thread_struct 是 task_struct 的一部分，用於保存 CPU 暫存器、堆疊指標等。

(10)記憶體資訊：task_struct 中透過 mm 指標管理進程的記憶體資訊，例如堆疊、記憶體映射等。mm 指標指向 mm_struct 結構體，該結構體記錄了進程的位址空間、堆、堆疊等資訊。



---


### What is current?




current 宏是 Linux 核心中一個非常重要的指針，它指向當前進程的 task_struct 結構體。這個宏可以用來快速訪問和操作當前執行的進程資訊。例如：

(1)current->pid 用來獲取當前進程的 PID。
(2)current->comm 用來獲取當前進程的名稱（通常是進程的執行檔名）。
(3)current->mm 是一個指向當前進程虛擬內存結構體 (mm_struct) 的指標。





---

### What is mm_struct?
mm_struct 結構體在 Linux 核心中描述了一個process的整個虛擬位址空間。它包含了有關process記憶體管理的所有資訊，包括該process如何使用虛擬記憶體，以及各個段（如 程式碼段 code segment、堆 heap、棧 stack 等）的位址範圍。



以下是 mm_struct 的一些主要欄位：

(1)pgd（Page Global Directory）：指向該process的頁目錄，用於地址轉換。這是頁表的根節點。

(2)mmap：指向虛擬記憶體區域（VMA，vm_area_struct 結構體）的鏈表，這些區域描述了進程內存映射的各個範圍，如程式碼區、堆區和共享庫等。

(3)start_code 和 end_code：表示code segment的起始和結束地址範圍。

(4)start_data 和 end_data：表示data segment的起始和結束地址範圍。

(5)start_brk 和 brk：表示堆區的起始位置和當前的結束位置。堆區的大小可以隨著 brk 系統調用進行動態擴展。

(6)start_stack：表示stack的起始地址。

(7)total_vm、locked_vm、shared_vm、exec_vm 等：表示process的總虛擬記憶體大小、鎖定的記憶體大小、共享記憶體大小、執行記憶體大小等，用來管理不同類型的內存分配。

(8)rss_stat：記錄process實際使用的實體記憶體頁數（Resident Set Size），反映了process當前佔用的實體記憶體大小。


---


### What is thread?



thread 是指比 Process 更輕量的執行單元。最大的不同在於 thread 共享同一個 Process 的資源（Memory space、File descriptor…等），但它們有各自的 stack、program counter 和 register。多個 thread 可以同時在一個 process 中運行。thread 通過共享 process 的資源來提高運行效率與並行處理能力，適合需要同時處理多重任務的應用程式中。


---


### 造成 Page Fault 的原因



當虛擬記憶體與實體記憶體之間尚未建立映射關係時，CPU 無法存取對應的實體記憶體，這時就會發生分頁錯誤（Page Fault）。

觸發 Page Fault 的原因主要為：

(1)使用共享記憶體區域，但尚未建立 Virtual Address 與 Physical Address 的對應關係，會導致 Page Fault。這種情況只需要在 page table 中建立正確的映射即可解決。
(2)訪問的地址在 Physical Address 中不存在，必須從硬碟或 swap 分區中讀取該頁才能使用。這類 Page Fault 對效能影響較大，因為磁碟讀取速度遠低於內存存取速度。
(3)訪問了非法的 Memory Address，這類 Page Fault 會觸發 SIGSEGV 訊號 - segmentation violation（區段違例），終止程序執行。當出現這種錯誤時，程序會立即被系統強制關閉。


---

### copy_to_user 和 copy_from_user


優點:
(1)安全性：能防止使用者空間對核心空間的非法存取，因為它們會檢查記憶體的有效性，以避免存取到無效的或未分配的記憶體區域。
穩定性：函式會在傳輸資料時處理一些常見的錯誤，核心可以根據回傳值去做後續的錯誤處理，確保資料在使用者空間與核心空間之間傳輸的穩定性。
(2)開發簡化：函式內部有標準化的方式處理核心空間和使用者空間之間的資料交換，減少了開發時間，也降低出錯機率。
(3)跨架構支援：copy_to_user 和 copy_from_user 被設計為跨架構的，可以在不同的硬體架構上使用，不需要進行額外的記憶體位址轉換。

缺點:
(1)額外的開銷：每次複製都需要進行記憶體位址的合法性檢查，這會帶來一些效能上的開銷，特別是在需要頻繁交換大量資料時會明顯影響性能。且每次在核心和使用者空間之間交換資料，都會佔用一些 CPU 資源。如果應用程式需要頻繁且大量地在核心空間和使用者空間之間傳輸資料，效能便會受到影響。
(2)有限的複製功能：這些函式只能用於簡單的記憶體區塊複製，不適合進行複雜的資料結構處理，無法滿足某些需要進行多層複製或深層結構的操作需求。
(3)僅限於核心和使用者空間之間的交換：copy_from_user() 和 copy_to_user() 僅適用於核心空間與使用者空間之間的資料交換。因為這兩個函式的主要目的是在跨越這個邊界時，保護系統安全並避免使用者空間的錯誤存取影響核心空間的穩定性。這些函式並不適合核心內部傳輸資料，因為核心內部的資料傳輸不需要進行額外的安全性檢查。

---

### bss segment和data segment


data segment:存放已經有明確初始化的 global 和 static 變數，當程式被讀進記憶體中時，這些值就會從執行檔被讀入。

bss segment:包含未明確初始的global 和 static 變數，當執行程式時會將其這區段的記憶體初始為零。
![image](https://hackmd.io/_uploads/B1PoeB1fkx.png)

---

### zombie process



當子程序（child process）結束後，它的狀態會變成 "殭屍程序"（zombie process）。此時，子程序的主要資源已釋放，但它的進程描述符（包含退出狀態）仍在，等待parent process來處理。如果parent process沒有即時獲取子程序的退出狀態，子程序就會保留在殭屍狀態。

處理方式：parent process可以使用 wait() 或 waitpid() 函數來取得子程序的退出狀態。當parent process調用這些函數時，殭屍程序的資源將被完全釋放，避免系統資源的浪費。

init process（PID 1）處理：
如果parent process在子程序結束前已經退出，子程序會自動被 init 程序接管，成為它的子程序。init 會定期回收zombie process，確保系統資源不會被占用。

---

### 指令解釋


```
#以系統管理員的權限依據 Makefile 的指令來編譯Kernel

sudo make 
```

```
#將已編譯好的Kernel模組安裝到系統指定的模組路徑

make modules_install
```
```
#按照 Makefile 的設定，將編譯後的Kernel安裝到系統中

make install 
```
---

### 什麼是MMU?


Memory Management Unit 為記憶體管理單元。用來進行 Virtual Address 到 Physical Address 的轉換，並提供記憶體保護、分頁和快取管理等功能。

主要功能：

Virtual Address 到 Physical Address 的轉換：透過查詢 page table 來完成這個映射過程。每個 Virtual Address在 page table 中對應著一個 Physical Address。
記憶體保護：透過設定每個 Page 的訪問權限來限制哪些程式可以讀、寫或執行某個特定的記憶體區域。
分頁和分段機制：將記憶體分成一個個固定大小的區塊（頁），並在需要時將部分頁換入或換出主記憶體，實現虛擬記憶體技術。這讓應用程式可以使用比實體記憶體更大的 Virtual Address 空間。
流程：

當程式請求存取某個 Virtual Address 時，請求會先送到 MMU。
MMU 檢查 Virtual Address，並通過 page table 將其轉換成對應的 Physical Address 。
如果 Virtual Address 對應的 page table 存在且有效，MMU 將 Physical Address 傳給記憶體控制器來完成數據存取。
如果 Virtual Address 對應的 page 不在主記憶體中，MMU 會引發 Page Fault，操作系統會處理這個情況，並將數據載入到記憶體中。

---

### Race Condition 發生時的處理方法

Race Condition 是當多個程序或執行緒同時訪問和操作共享資源，而導致不正確的結果的一種情況。處理 Race Condition 的兩種方法：

 • 使用spin lock：在進入共享資源的臨界區前加鎖，離開時解鎖。當一個程序發現鎖被另一個程序關閉時，它會反覆旋轉，執行一個緊密的指令迴圈，直到鎖開啟，以確保同一時間只有一個執行緒能訪問該資源。
 • 使用原子操作（Atomic Operations）：利用 CPU 支持的原子指令（例如 atomic_add()），這些指令可以在單個指令週期內完成，保證不會被其他執行緒中斷。

---

### gcd...function

1. pgd_offset: 根據目前的 Virtual Address 和目前 Process 的資料結構 task_struct，存取其中的 mm point。
2. mm point 中儲存著 mm_struct 位置，而 mm_struct 儲存該 Process 虛擬位置資料的結構，在該結構中我們就可以找到 pgd 的初始位置。使用 pgd_offset 即可存取 pgd page 中的 entry (pgd entry)
(entry 內容為 pud table 的 base address)
3. pud_offset: 根據透過 pgd_offset 得到的 pgd entry 和 Virtual Address，可得到 pud entry
(entry 內容為 pmd table 的 base address)
4. pmd_offset: 根據 pud entry 的內容和 Virtual Address，可得到 pte table 的 base address
5. pte_offset: 根據 pmd entry 的內容與 Virtual Address，可得到 pte 的 base address
6. 將從 pte 得到的 Base Address 與 Mask(0xf…fff000)做 AND 運算，即可得到 Page 的 Base Physical Address
7. Virtual Address 與 ~Mask(0x0…000fff)做 AND 運算得到 offset，再與 Page 的 base Physical Address 做 OR 運算即可得到轉換過後且完整的 Physical Address。

---

### Virtual Address 跟 Physical Address的關係

Virtual Address: 程式在執行時所使用的位址。它由應用程式生成，並且不直接對應到實體記憶體（RAM）。Virtual Address 是操作系統提供給每個應用程式的獨立、隔離的記憶體空間。這使得每個應用程式看起來都有自己的連續的記憶體地址範圍。

Physical Address: 實體位址是物理記憶體（RAM）中的實際地址。它代表了數據在硬體中的具體存放位置。物理位址是由記憶體管理單元（MMU）生成並存取的，且每一個位置對應到實際的硬體存儲位置。

虛擬位址和實體位址之間的關係是透過操作系統和硬體中的 MMU 來管理的。假設一個程式存取虛擬位址 0xB800 這個位址。MMU 查詢 page table，發現 Virtual Address: 0xB800 對應到 Physical Address: 0x3F00。MMU 將把對 0xB800 的存取轉換成對實體記憶體中 0x3F00 的存取。

---

### What is Makefile?

Linux 中的 Makefile 是用來告訴 make 工具如何編譯程式碼的檔案。

如 obj-y := hello.o，代表將 hello.o（即 hello.c 編譯後的目標檔案）包含在最終的編譯輸出中，並將其連結進去。

obj-y 是一個變數，通常用在 Linux Kernel 的 Makefile 中，表示這些目標檔（.o 文件）應該被編譯並且連結到 Kernel 或模組中。也就是說這些目標檔案會成為最終的 Kernel 的一部分。

:= 是賦值運算符，表示將右邊的值（這裡是 hello.o）賦值給左邊的變數（這裡是 obj-y）。

hello.o 是從 hello.c 源文件編譯生成的目標檔。

所以這整行的意思就是我有一個驅動程式 hello.c，希望將它編譯進 Linux 核心中。讓 hello.c 成為 Kernel 的一部分，這時你會在該模組的 Makefile 中添加：obj-y := hello.o

---

### What is GRUB?

GRUB 是「GNU GRand Unified Bootloader」是一個 boot loader，專門用來啟動 Linux 和其他多種作業系統。當電腦開機後，BIOS 會尋找硬碟上的 boot loader。
GRUB 就是這個 boot loader，它首先被加載，然後由 GRUB 將控制權交給 Linux Kernel。
在這個過程中，GRUB 可以提供一個選單，允許用戶選擇要啟動哪一個作業系統，或者通過手動命令修改啟動過程。