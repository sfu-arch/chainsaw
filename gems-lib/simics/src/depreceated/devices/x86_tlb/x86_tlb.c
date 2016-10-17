/*
  x86_tlb.c

  Copyright (C) 2002-2003 Virtutech AB, All Rights Reserved

  This program is a component ("Component") of Virtutech Simics and is
  being distributed under Section 1(a)(iv) of the Virtutech Simics
  Software License Agreement (the "Agreement").  You should have
  received a copy of the Agreement with this Component; if not, please
  write to Virtutech AB, Norrtullsgatan 15, 1tr, SE-113 27 STOCKHOLM,
  Sweden for a copy of the Agreement prior to using this Component.

  By using this Component, you agree to be bound by all of the terms of
  the Agreement.  If you do not agree to the terms of the Agreement, you
  may not use, copy or otherwise access the Component or any derivatives
  thereof.  You may create and use derivative works of this Component
  pursuant to the terms the Agreement provided that any such derivative
  works may only be used in conjunction with and as a part of Virtutech
  Simics for use by an authorized licensee of Virtutech.

  THIS COMPONENT AND ANY DERIVATIVES THEREOF ARE PROVIDED ON AN "AS IS"
  BASIS.  VIRTUTECH MAKES NO WARRANTIES WITH RESPECT TO THE COMPONENT OR
  ANY DERIVATIVES THEREOF AND DISCLAIMS ALL IMPLIED WARRANTIES,
  INCLUDING, WITHOUT LIMITATION, WARRANTIES OF MERCHANTABILITY AND
  FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.

*/

#define DEVICE_INFO_STRING \
  "Default X86 TLB class."

#include <string.h>

#include <simics/api.h>
#include <simics/alloc.h>
#include <simics/utils.h>
#include <simics/arch/x86.h>

#define DEVICE_NAME "x86-tlb"

#define TLB_WAYS 2 /* instruction or data */

/* Define here the size and associativity */
#define X86_4K_TLB_SIZE 16
#define X86_4K_TLB_ASSOC 4
#define X86_LARGE_TLB_SIZE 16
#define X86_LARGE_TLB_ASSOC 4

static hap_type_t x86_hap_tlb_invalidate[TLB_WAYS];
static hap_type_t x86_hap_tlb_replace[TLB_WAYS];
static hap_type_t x86_hap_tlb_fill[TLB_WAYS];
static hap_type_t x86_hap_tlb_miss[TLB_WAYS];

typedef struct {
        data_or_instr_t  way;
        int              large;
} x86_tlb_id_t;

typedef struct tlb_entry {
        linear_address_t        linear_page_start;
        physical_address_t      physical_page_start;
        processor_mode_t        mode:2;
        read_or_write_t         rw:1;
        unsigned                global_page:1;
        x86_memory_type_t       pat_type;
        x86_memory_type_t       mtrr_type;
} tlb_entry_t;

typedef struct tlb_wrap_entry tlb_wrap_entry_t;

struct tlb_wrap_entry {
        tlb_entry_t             entry;
        int                     valid;
        tlb_wrap_entry_t       *prev;
        tlb_wrap_entry_t       *next;
};

typedef enum {
        Mode_Limited, Mode_Hash_Table, Mode_Multi_Level_Table
} tlb_mode_t;

typedef struct x86_tlb
{
        log_object_t            log;
        conf_object_t          *cpu;

        tlb_mode_t              tlb_mode;
        int                     in_pae_mode;

        linear_address_t        large_page_size;
        int                     large_shift;

        /* Limited size mode */
        tlb_entry_t             pc_tlb_4k[TLB_WAYS][X86_4K_TLB_SIZE][X86_4K_TLB_ASSOC];
        tlb_entry_t             pc_tlb_large[TLB_WAYS][X86_LARGE_TLB_SIZE][X86_LARGE_TLB_ASSOC];

        /* Unlimited size mode (hash table version) */
        ht_table_t              hash_large;
        ht_table_t              hash_4k;

        /* Unlimited size mode (full table version) */
        tlb_wrap_entry_t        table_large[1 << 11];
        tlb_wrap_entry_t        table_large_head;
        tlb_wrap_entry_t        table_4k[1 << 20];
        tlb_wrap_entry_t        table_4k_head;

} x86_tlb_t;

static const char *x86_memory_type_descr[8];

typedef struct {
        x86_tlb_t      *tlb;
        int             pagesize;
        int             keep_global_entries;
} hash_remove_info_t;

#define INVALIDATE(entry) (entry).linear_page_start = 1
#define ISVALID(entry)    ((entry).linear_page_start != 1)

static void
enter_tlb_mode(x86_tlb_t *tlb, tlb_mode_t old_mode, tlb_mode_t new_mode)
{
        int i, j, k;

        if (old_mode == new_mode)
                return;

        if (tlb->log.obj.configured)
                SIM_STC_flush_cache(tlb->cpu);

        switch (old_mode) {
        case Mode_Limited:
                memset(tlb->pc_tlb_4k, 0, sizeof(tlb->pc_tlb_4k));
                memset(tlb->pc_tlb_large, 0, sizeof(tlb->pc_tlb_large));
                for (i = 0; i < TLB_WAYS; i++)
                        for (j = 0; j < X86_4K_TLB_SIZE; j++)
                                for (k = 0; k < X86_4K_TLB_ASSOC; k++)
                                        INVALIDATE(tlb->pc_tlb_4k[i][j][k]);
                for (i = 0; i < TLB_WAYS; i++)
                        for (j = 0; j < X86_LARGE_TLB_SIZE; j++)
                                for (k = 0; k < X86_LARGE_TLB_ASSOC; k++)
                                        INVALIDATE(tlb->pc_tlb_large[i][j][k]);
                break;
        case Mode_Hash_Table:
                ht_clear_table(&tlb->hash_large, 1, 0);
                ht_clear_table(&tlb->hash_4k, 1, 0);
                break;
        case Mode_Multi_Level_Table:
                tlb->table_large_head.next = tlb->table_large_head.prev = &tlb->table_large_head;
                tlb->table_4k_head.next = tlb->table_4k_head.prev = &tlb->table_4k_head;
                memset(tlb->table_large, 0, sizeof(tlb->table_large));
                memset(tlb->table_4k, 0, sizeof(tlb->table_4k));
                break;
        }
}

static int
flush_entry(x86_tlb_t *tlb, tlb_entry_t *tlb_entry, int keep_global_entries, int pagesize)
{
        if (!keep_global_entries || !tlb_entry->global_page) {
                SIM_flush_D_STC_logical(tlb->cpu, tlb_entry->linear_page_start, pagesize);
                return 1;
        };
        return 0;
}

static int
hash_remove_entry(ht_table_t *table, uint64 key, void *value, void *data)
{
        tlb_entry_t *tlb_entry = (tlb_entry_t *)value;
        hash_remove_info_t *remove_info = (hash_remove_info_t *)data;

        if (flush_entry(remove_info->tlb, tlb_entry,
                        remove_info->keep_global_entries,
                        remove_info->pagesize)) {
                ht_remove_int(table, key, tlb_entry);
                MM_FREE(tlb_entry);
        }

        return 1;
}

static void
table_insert_entry(tlb_wrap_entry_t *head, tlb_wrap_entry_t *ptr)
{
        ASSERT(!ptr->valid);
        ptr->valid = 1;
        ptr->prev = head;
        ptr->next = head->next;
        head->next->prev = ptr;
        head->next = ptr;
}

static void
table_remove_entry(tlb_wrap_entry_t *ptr)
{
        ASSERT(ptr->valid);
        ptr->prev->next = ptr->next;
        ptr->next->prev = ptr->prev;
        ptr->valid = 0;
}

enum cond { Cond_Always, Cond_Nonglobal, Cond_Matching };

static void
flush_tlb_assoc_one(x86_tlb_t *tlb, int large, int select,
                    enum cond when, linear_address_t la, data_or_instr_t way)
{
        int k, prev_size;
        int entries_flushed = 0;
        tlb_entry_t *table = large ? tlb->pc_tlb_large[way][select]
                                   : tlb->pc_tlb_4k[way][select];
        int assoc_size = large ? X86_LARGE_TLB_ASSOC : X86_4K_TLB_ASSOC;
        int page_code = large ? (tlb->in_pae_mode ? 1 : 2) : 0;

        for (k = 0; k < assoc_size; k++) {
                if (!ISVALID(table[k]))
                        break;
                if (when == Cond_Nonglobal) {
                        if (table[k].global_page) continue;
                } else if (when == Cond_Matching) {
                        if (la != table[k].linear_page_start) continue;
                }
                /* flush this entry */
                entries_flushed++;
                SIM_c_hap_occurred_always(
                        x86_hap_tlb_invalidate[way],
                        &tlb->log.obj,
                        page_code,
                        (integer_t)table[k].linear_page_start,
                        (integer_t)table[k].physical_page_start,
                        (integer_t)page_code);
                SIM_clear_exception();
                if (when != Cond_Always)
                        SIM_flush_D_STC_logical(tlb->cpu,
                                table[k].linear_page_start,
                                large ? tlb->large_page_size : 4096);
                INVALIDATE(table[k]);
        }
        prev_size = k;

        /* Pack the entries if holes appeared */
        if (entries_flushed == 0 || entries_flushed == prev_size)
                return;
        int next_slot = 0;
        for (k = 0; k < prev_size; k++) {
                if (ISVALID(table[k]))
                        table[next_slot++] = table[k];
        }
        for (; next_slot < prev_size; next_slot++)
                INVALIDATE(table[next_slot]);
}

static void
flush_tlb_assoc(x86_tlb_t *tlb, int large, int select,
                enum cond when, linear_address_t la)
{
        flush_tlb_assoc_one(tlb, large, select, when, la, Sim_DI_Data);
        flush_tlb_assoc_one(tlb, large, select, when, la, Sim_DI_Instruction);
}

static void
x86_tlb_flush_all(conf_object_t *obj, int keep_global_entries)
{
        x86_tlb_t *tlb = (x86_tlb_t *)obj;
        int i;

        debug_log(2, &tlb->log, "Flush all (keep global: %d).", keep_global_entries);

        if (tlb->tlb_mode == Mode_Hash_Table) {
                hash_remove_info_t remove_info;

                remove_info.tlb = tlb;
                remove_info.keep_global_entries = keep_global_entries;

                remove_info.pagesize = tlb->large_page_size;
                ht_for_each_entry(&tlb->hash_large, hash_remove_entry, &remove_info);
                remove_info.pagesize = 4*1024;
                ht_for_each_entry(&tlb->hash_4k, hash_remove_entry, &remove_info);
        } else if (tlb->tlb_mode == Mode_Multi_Level_Table) {
                tlb_wrap_entry_t *ptr;

                for (ptr = tlb->table_large_head.next; ptr != &tlb->table_large_head; ptr = ptr->next) {
                        ASSERT(ptr->valid);
                        if (flush_entry(tlb, &ptr->entry, keep_global_entries, tlb->large_page_size))
                                table_remove_entry(ptr);
                }
                for (ptr = tlb->table_4k_head.next; ptr != &tlb->table_4k_head; ptr = ptr->next) {
                        ASSERT(ptr->valid);
                        if (flush_entry(tlb, &ptr->entry, keep_global_entries, 4*1024))
                                table_remove_entry(ptr);
                }
        } else if (keep_global_entries) {
                for (i = 0; i < X86_LARGE_TLB_SIZE; i++)
                        flush_tlb_assoc(tlb, 1, i, Cond_Nonglobal, 0);
                for (i = 0; i < X86_4K_TLB_SIZE; i++)
                        flush_tlb_assoc(tlb, 0, i, Cond_Nonglobal, 0);
        } else {
                /* Flush all TLB entries, PGE not supported or everything
                   should be flushed */
                for (i = 0; i < X86_LARGE_TLB_SIZE; i++)
                        flush_tlb_assoc(tlb, 1, i, Cond_Always, 0);
                for (i = 0; i < X86_4K_TLB_SIZE; i++)
                        flush_tlb_assoc(tlb, 0, i, Cond_Always, 0);
                SIM_STC_flush_cache(tlb->cpu);
        }
}

static void
x86_tlb_flush_page(conf_object_t *obj,
                   linear_address_t laddr)
{
        int tlb_index;
        linear_address_t linear_page_start;
        x86_tlb_t *tlb = (x86_tlb_t *)obj;

        debug_log(2, &tlb->log, "Flush page 0x%llx.", laddr);

        if (tlb->tlb_mode == Mode_Hash_Table) {
                tlb_entry_t *tlb_entry;

                if ((tlb_entry = (tlb_entry_t *)ht_lookup_int(&tlb->hash_large, laddr >> tlb->large_shift)) != NULL) {
                        SIM_flush_D_STC_logical(tlb->cpu, tlb_entry->linear_page_start, tlb->large_page_size);
                        ht_remove_int(&tlb->hash_large, laddr >> tlb->large_shift, tlb_entry);
                        MM_FREE(tlb_entry);
                }
                if ((tlb_entry = (tlb_entry_t *)ht_lookup_int(&tlb->hash_4k, laddr >> 12)) != NULL) {
                        SIM_flush_D_STC_logical(tlb->cpu, tlb_entry->linear_page_start, 4*1024);
                        ht_remove_int(&tlb->hash_4k, laddr >> 12, tlb_entry);
                        MM_FREE(tlb_entry);
                }
        } else if (tlb->tlb_mode == Mode_Multi_Level_Table) {
                tlb_wrap_entry_t *tlb_entry;

                tlb_entry = &tlb->table_large[laddr >> tlb->large_shift];
                if (tlb_entry->valid) {
                        SIM_flush_D_STC_logical(tlb->cpu, tlb_entry->entry.linear_page_start, tlb->large_page_size);
                        table_remove_entry(tlb_entry);
                }
                tlb_entry = &tlb->table_4k[laddr >> 12];
                if (tlb_entry->valid) {
                        SIM_flush_D_STC_logical(tlb->cpu, tlb_entry->entry.linear_page_start, 4*1024);
                        table_remove_entry(tlb_entry);
                }
        } else {
                /* Check the 4M/2M TLB entries, clear the page entry if found */
                linear_page_start = laddr & ~(tlb->large_page_size - 1);
                tlb_index = (laddr >> tlb->large_shift) & (X86_LARGE_TLB_SIZE-1);
                flush_tlb_assoc(tlb, 1, tlb_index,
                                Cond_Matching, linear_page_start);

                /* Check the 4K TLB entries, clear the page entry if found */
                linear_page_start = laddr & ~0xfff;
                tlb_index = (laddr >> 12) & (X86_4K_TLB_SIZE-1);
                flush_tlb_assoc(tlb, 0, tlb_index,
                                Cond_Matching, linear_page_start);
        }
}

static tlb_entry_t *
x86_tlb_lookup_common(x86_tlb_t *tlb, processor_mode_t mode,
                      read_or_write_t rw, data_or_instr_t tlb_select,
                      linear_address_t laddr, physical_address_t *out_addr)
{
        linear_address_t linear_page_start;
        linear_address_t offset = 0;
	int k;
        int tlb_index;
        tlb_entry_t *tlb_entry = NULL;

        if (tlb->tlb_mode == Mode_Hash_Table) {
                if ((tlb_entry = ht_lookup_int(&tlb->hash_large, laddr >> tlb->large_shift)) != NULL) {
                        offset = laddr & (tlb->large_page_size - 1);
                } else if ((tlb_entry = ht_lookup_int(&tlb->hash_4k, laddr >> 12)) != NULL) {
                        offset = laddr & (4*1024 - 1);
                }
        } else if (tlb->tlb_mode == Mode_Multi_Level_Table) {
                tlb_wrap_entry_t *entry;

                if ((entry = &tlb->table_large[laddr >> tlb->large_shift]) && entry->valid) {
                        offset = laddr & (tlb->large_page_size - 1);
                        tlb_entry = &entry->entry;
                } else if ((entry = &tlb->table_4k[laddr >> 12]) && entry->valid) {
                        offset = laddr & (4*1024 - 1);
                        tlb_entry = &entry->entry;
                }
        } else if (tlb->tlb_mode == Mode_Limited) {
                /* First look in the 2M/4M TLB */
                linear_page_start = laddr & ~(tlb->large_page_size - 1);
                tlb_index = (laddr >> tlb->large_shift) & (X86_LARGE_TLB_SIZE-1);
                offset = laddr & (tlb->large_page_size - 1);
                for (k = 0; k < X86_LARGE_TLB_ASSOC; k++) {
                        if (linear_page_start == tlb->pc_tlb_large[tlb_select][tlb_index][k].linear_page_start) {
                                tlb_entry = &tlb->pc_tlb_large[tlb_select][tlb_index][k];
                                break;
                        }
                }

                if (!tlb_entry) {
                        /* Now check the 4K TLB */
                        linear_page_start = laddr & ~0xfff;
                        tlb_index = (laddr >> 12) & (X86_4K_TLB_SIZE-1);
                        offset = laddr & 0xfff;
                        for (k = 0; k < X86_4K_TLB_ASSOC; k++) {
                                if (linear_page_start == tlb->pc_tlb_4k[tlb_select][tlb_index][k].linear_page_start) {
                                        tlb_entry = &tlb->pc_tlb_4k[tlb_select][tlb_index][k];
                                        break;
                                }
                        }
                }
        }

        /* The comparisions below depend those relations */
        CASSERT_STMT(Sim_CPU_Mode_User < Sim_CPU_Mode_Supervisor);
        CASSERT_STMT(Sim_RW_Read < Sim_RW_Write);
        if (tlb_entry && mode >= tlb_entry->mode && rw <= tlb_entry->rw) {
                *out_addr = tlb_entry->physical_page_start + offset;
                return tlb_entry;
        }

        SIM_c_hap_occurred_always(x86_hap_tlb_miss[tlb_select], &tlb->log.obj, laddr, (integer_t)laddr);
        SIM_clear_exception();
        return NULL;
}

static int
x86_tlb_lookup(conf_object_t *obj,
               x86_memory_transaction_t *mem_tr)
{
        x86_tlb_t *tlb = (x86_tlb_t *)obj;
        read_or_write_t rw;
        data_or_instr_t tlb_select;
        tlb_entry_t *entry;

        /* 
        ** Perform TLB lookup 
        ** Measurment on booting Linux showed that we hit roughly 75% in the 
        ** 4 meg TLB and 25% in the 4 k TLB (of all TLB hits) 
        */
        rw = (SIM_mem_op_is_write(&mem_tr->s) || mem_tr->fault_as_if_write)
                ? Sim_RW_Write : Sim_RW_Read;
        tlb_select = (SIM_mem_op_is_data(&mem_tr->s)
                      ? Sim_DI_Data
                      : Sim_DI_Instruction);

        entry = x86_tlb_lookup_common(tlb, mem_tr->mode, rw, tlb_select,
                                      mem_tr->linear_address,
                                      &mem_tr->s.physical_address);
        if (entry) {
                mem_tr->mtrr_type = entry->mtrr_type;
                mem_tr->pat_type = entry->pat_type;
        }
        return entry != NULL;
}

static void
tlb_add_hash(x86_tlb_t *tlb, ht_table_t *hash, integer_t key,
             tlb_entry_t *new_tlb_entry, logical_address_t page_size)
{
        tlb_entry_t *tlb_entry = ht_lookup_int(hash, key);

        if (tlb_entry == NULL) {
                tlb_entry = MM_ZALLOC(1, tlb_entry_t);
                ht_insert_int(hash, key, tlb_entry);
        } else {
                if (tlb_entry->linear_page_start
                    == new_tlb_entry->linear_page_start) {
                        /* Remove STC entries for modified TLB entry */
                        SIM_flush_D_STC_logical(tlb->cpu,
                                                tlb_entry->linear_page_start,
                                                page_size);
                }
        }
        *tlb_entry = *new_tlb_entry;
}

static void
tlb_add_assoc(x86_tlb_t *tlb, data_or_instr_t tlb_select,
              tlb_entry_t *table, int assoc_size, tlb_entry_t *new_entry,
              linear_address_t page_size, int page_code)
{
        int k;
        int first_free = -1;

        for (k = 0; k < assoc_size; k++) {
                if (table[k].linear_page_start
                    == new_entry->linear_page_start) {
                        /* Remove STC entries for modified TLB entry */
                        SIM_flush_D_STC_logical(tlb->cpu,
                                                table[k].linear_page_start,
                                                page_size);
                        break;
                }
                else if (!ISVALID(table[k]) && first_free == -1)
                        first_free = k;
        }
        if (k == assoc_size && first_free >= 0)
                k = first_free;
        if (k == assoc_size) {
                /* Move entries up through FIFO, evict entry 0, fill entry
                   ASSOC-1 */
                k = assoc_size - 1;
                SIM_c_hap_occurred_always(x86_hap_tlb_replace[tlb_select],
                                  &tlb->log.obj, page_code,
                                  (integer_t)table[0].linear_page_start,
                                  (integer_t)table[0].physical_page_start,
                                  (integer_t)page_code);
                SIM_clear_exception();
                /* Remove STC entries for overwritten TLB entry */
                SIM_flush_D_STC_logical(tlb->cpu, table[0].linear_page_start,
                                        page_size);
                int move;
                for (move = 0; move < assoc_size-1; move++)
                        table[move] = table[move+1];
        }
        SIM_c_hap_occurred_always(x86_hap_tlb_fill[tlb_select],
                                  &tlb->log.obj, page_code,
                                  (integer_t)new_entry->linear_page_start,
                                  (integer_t)new_entry->physical_page_start,
                                  (integer_t)page_code);
        SIM_clear_exception();
        table[k] = *new_entry;
}

static void
x86_tlb_add(conf_object_t *obj,
            processor_mode_t mode,
            read_or_write_t read_or_write,
            data_or_instr_t tlb_select,
            int global_page,
            x86_memory_type_t pat_type,
            x86_memory_type_t mtrr_type,
            linear_address_t laddr,
            physical_address_t paddr,
            int page_code)
{
        int tlb_index;
        tlb_entry_t new_tlb_entry;
        x86_tlb_t *tlb = (x86_tlb_t *)obj;
        linear_address_t pagesize;

        SIM_log_info(4, &tlb->log, 0, "Add %s %s %s l:0x%llx p:0x%llx sz %d.",
                     read_or_write == Sim_RW_Read ? "read" : "write",
                     tlb_select == Sim_DI_Data ? "data" : "instr",
                     global_page ? "global" : "nonglobal",
                     laddr, paddr, page_code);

        new_tlb_entry.mode = mode;
        /* Note: We cannot insert the page in the TLB with writes
           enabled if the current access is a read/fetch. A later write
           will generate a 'false' TLB miss that updates the dirty bit
           correctly. */
        new_tlb_entry.rw = read_or_write;
        new_tlb_entry.global_page = global_page;
	new_tlb_entry.pat_type = pat_type;
	new_tlb_entry.mtrr_type = mtrr_type;

        if (page_code == 2) {
                pagesize = 1 << 22;
        } else if (page_code == 1) {
                pagesize = 1 << 21;
        } else {
                pagesize = 1 << 12;
        }
        new_tlb_entry.linear_page_start = laddr & ~(pagesize - 1);
        new_tlb_entry.physical_page_start = paddr & ~(pagesize - 1);

        if (tlb->tlb_mode == Mode_Hash_Table) {
                if (page_code == 2) {
                        tlb_add_hash(tlb, &tlb->hash_large,
                                     new_tlb_entry.linear_page_start >> 22,
                                     &new_tlb_entry, pagesize);
                } else if (page_code == 1) {
                        tlb_add_hash(tlb, &tlb->hash_large,
                                     new_tlb_entry.linear_page_start >> 21,
                                     &new_tlb_entry, pagesize);
                } else {
                        tlb_add_hash(tlb, &tlb->hash_4k,
                                     new_tlb_entry.linear_page_start >> 12,
                                     &new_tlb_entry, pagesize);
                }
        } else if (tlb->tlb_mode == Mode_Multi_Level_Table) {
                tlb_wrap_entry_t *wrap;
                if (page_code == 2) {
                        wrap = &tlb->table_large[new_tlb_entry.linear_page_start >> 22];
                } else if (page_code == 1) {
                        wrap = &tlb->table_large[new_tlb_entry.linear_page_start >> 21];
                } else {
                        wrap = &tlb->table_4k[new_tlb_entry.linear_page_start >> 12];
                }
                if (wrap->valid) {
                        /* Remove STC entries for modified TLB entry */
                        SIM_flush_D_STC_logical(tlb->cpu,
                                                new_tlb_entry.linear_page_start,
                                                pagesize);
                }
                wrap->entry = new_tlb_entry;
                if (!wrap->valid) {
                        if (page_code == 2) {
                                table_insert_entry(&tlb->table_large_head, wrap);
                        } else if (page_code == 1) {
                                table_insert_entry(&tlb->table_large_head, wrap);
                        } else {
                                table_insert_entry(&tlb->table_4k_head, wrap);
                        }
                }
        } else {
                if (page_code > 0) {
                        /* The page table entry represented a large page */
                        tlb_index = (laddr >> tlb->large_shift) & (X86_LARGE_TLB_SIZE-1);
                        tlb_add_assoc(tlb, tlb_select,
                                      tlb->pc_tlb_large[tlb_select][tlb_index],
                                      X86_LARGE_TLB_ASSOC,
                                      &new_tlb_entry,
                                      pagesize,
                                      page_code);
                } else {
                        /* The page table entry represented a 4K page */
                        tlb_index = (laddr >> 12) & (X86_4K_TLB_SIZE-1);
                        tlb_add_assoc(tlb, tlb_select,
                                      tlb->pc_tlb_4k[tlb_select][tlb_index],
                                      X86_4K_TLB_ASSOC,
                                      &new_tlb_entry,
                                      pagesize,
                                      page_code);
                }
        }
}

static tagged_physical_address_t
x86_tlb_itlb_lookup(conf_object_t *obj,
                    linear_address_t laddr,
                    processor_mode_t mode)
{
        x86_tlb_t *tlb = (x86_tlb_t *)obj;
        tagged_physical_address_t ret;
        ret.valid = x86_tlb_lookup_common(tlb, mode, Sim_RW_Read,
                                          Sim_DI_Instruction,
                                          laddr, &ret.paddr) != NULL; 
        return ret;
}

static void
x86_cr4_write(lang_void *_ptr, conf_object_t *cpu_obj, integer_t reg_no, integer_t val)
{
        x86_tlb_t *ptr = (x86_tlb_t *)_ptr;
        /* Check if the hap is from the CPU this TLB is connected to */
        tlb_mode_t old_mode = ptr->tlb_mode;
        if (cpu_obj == ptr->cpu) {
                ASSERT(reg_no == SIM_get_register_number(cpu_obj, "cr4"));
                /* Check the PAE enable bit (bit 5) */
                if (ptr->in_pae_mode && !((val >> 5) & 1)) {
                        debug_log(1, &ptr->log, "Large TLB select: 4 Mb");
                        ptr->in_pae_mode = 0;
                        ptr->large_page_size = 4*1024*1024;
                        ptr->large_shift = 22;
                        if (ptr->tlb_mode == Mode_Hash_Table)
                                ptr->tlb_mode = Mode_Multi_Level_Table;
                } else if (!ptr->in_pae_mode && ((val >> 5) & 1)) {
                        debug_log(1, &ptr->log, "Large TLB select: 2 Mb");
                        ptr->in_pae_mode = 1;
                        ptr->large_page_size = 2*1024*1024;
                        ptr->large_shift = 21;
                        if (ptr->tlb_mode == Mode_Multi_Level_Table)
                                ptr->tlb_mode = Mode_Hash_Table;
                }
                enter_tlb_mode(ptr, old_mode, ptr->tlb_mode);
        }
}

static conf_object_t *
x86_tlb_new_instance(parse_object_t *parse_obj)
{
        int i, j, k;
        x86_tlb_t *ptr = MM_ZALLOC(1, x86_tlb_t);
        SIM_log_constructor(&ptr->log, parse_obj);

        /* Invalidate the entire TLB */
        for (i = 0; i < X86_LARGE_TLB_SIZE; i++)
                for (j = 0; j < TLB_WAYS; j++)
                        for (k = 0; k < X86_LARGE_TLB_ASSOC; k++)
                                INVALIDATE(ptr->pc_tlb_large[j][i][k]);
        for (i = 0; i < X86_4K_TLB_SIZE; i++)
                for (j = 0; j < TLB_WAYS; j++)
                        for (k = 0; k < X86_4K_TLB_ASSOC; k++)
                                INVALIDATE(ptr->pc_tlb_4k[j][i][k]);

        ht_init_table(&ptr->hash_large, 1);
        ht_init_table(&ptr->hash_4k, 1);

        ptr->table_large_head.next = ptr->table_large_head.prev = &ptr->table_large_head;
        ptr->table_4k_head.next = ptr->table_4k_head.prev = &ptr->table_4k_head;

        ptr->tlb_mode = Mode_Limited;

        ptr->large_page_size = 4*1024*1024;
        ptr->large_shift = 22;

        return (conf_object_t *)ptr;
}

static x86_tlb_id_t *
x86_tlb_id(data_or_instr_t way, int large)
{
        x86_tlb_id_t *ret = MM_ZALLOC(1, x86_tlb_id_t);
        ret->way = way;
        ret->large = large;
        return ret;
}

static set_error_t
set_cpu(void *_id, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        x86_tlb_t *ptr = (x86_tlb_t *)obj;
        ptr->cpu = val->u.object;
        SIM_hap_add_callback_index("Core_Control_Register_Write",
                                   (obj_hap_func_t)x86_cr4_write, ptr, SIM_get_register_number(ptr->cpu, "cr4"));
        return Sim_Set_Ok;
}

static attr_value_t
get_cpu(void *_id, conf_object_t *obj, attr_value_t *idx)
{
        x86_tlb_t *ptr = (x86_tlb_t *)obj;
        attr_value_t ret;
        if (ptr->cpu) {
                ret.kind = Sim_Val_Object;
                ret.u.object = ptr->cpu;
        } else {
                ret.kind = Sim_Val_Nil;
        }
        return ret;
}

static int
parse_memtype(const char *str)
{
        int i;
        for (i = 0; x86_memory_type_descr[i]; i++)
                if (strcmp(x86_memory_type_descr[i], str) == 0)
                        return i;
        return -1;
}

static int
valid_tlb_entry(attr_value_t *tlb_entry, int align_mask)
{
        if (tlb_entry->kind != Sim_Val_List
            || (tlb_entry->u.list.size != 5 && tlb_entry->u.list.size != 7))
                return 0;
        attr_value_t *vector = tlb_entry->u.list.vector;
        if (vector[0].kind != Sim_Val_Integer ||
            vector[1].kind != Sim_Val_Integer ||
            vector[2].kind != Sim_Val_Integer ||
            vector[3].kind != Sim_Val_Integer ||
            vector[4].kind != Sim_Val_Integer)
                return 0;
        if (tlb_entry->u.list.size == 7) {
                if (vector[5].kind != Sim_Val_String ||
                    vector[6].kind != Sim_Val_String)
                        return 0;
                if (parse_memtype(vector[5].u.string) < 0)
                        return 0;
                if (parse_memtype(vector[6].u.string) < 0)
                        return 0;
        }
        if ((vector[0].u.integer & align_mask) ||
            (vector[1].u.integer & align_mask))
                return 0;
        return 1;
}

static void
set_tlb_entry(tlb_entry_t *entry, attr_value_t *tlb_entry)
{
        entry->linear_page_start = tlb_entry->u.list.vector[0].u.integer;
        entry->physical_page_start = tlb_entry->u.list.vector[1].u.integer;
        entry->mode = tlb_entry->u.list.vector[2].u.integer ?
                Sim_CPU_Mode_Supervisor : Sim_CPU_Mode_User;
        entry->rw = tlb_entry->u.list.vector[3].u.integer ?
                Sim_RW_Read : Sim_RW_Write;
        entry->global_page = tlb_entry->u.list.vector[4].u.integer;
        if (tlb_entry->u.list.size == 7) {
                entry->pat_type = parse_memtype(tlb_entry->u.list.vector[5]
                                                .u.string);
                entry->mtrr_type = parse_memtype(tlb_entry->u.list.vector[6]
                                                 .u.string);
        } else {
                entry->pat_type = X86_None;
                entry->mtrr_type = X86_None;
        }
}

static void
get_tlb_entry(attr_value_t *tlb_entry, tlb_entry_t *entry)
{
        *tlb_entry = SIM_make_attr_list(7,
                SIM_make_attr_integer(entry->linear_page_start),
                SIM_make_attr_integer(entry->physical_page_start),
                SIM_make_attr_integer(entry->mode == Sim_CPU_Mode_Supervisor),
                SIM_make_attr_integer(entry->rw == Sim_RW_Read),
                SIM_make_attr_integer(entry->global_page),
                SIM_make_attr_string(x86_memory_type_descr[entry->pat_type]),
                SIM_make_attr_string(x86_memory_type_descr[entry->mtrr_type])
        );
}

static int
set_tlb_check_valid(attr_value_t *val, int assoc, int align_mask)
{
        int idx, way;

        for (idx = 0; idx < val->u.list.size; idx++) {
                if (val->u.list.vector[idx].kind != Sim_Val_List ||
                    (assoc && val->u.list.vector[idx].u.list.size != assoc))
                        return 0;
                for (way = 0; way < val->u.list.vector[idx].u.list.size; way++) {
                        attr_value_t *tlb_entry = &val->u.list.vector[idx].u.list.vector[way];
                        if (!valid_tlb_entry(tlb_entry, align_mask))
                                return 0;
                }
        }

        return 1;
}

static set_error_t
set_x86_tlb(void *_id, conf_object_t *obj, attr_value_t *val, attr_value_t *_unused)
{
        x86_tlb_id_t *id = (x86_tlb_id_t *)_id;
        x86_tlb_t *ptr = (x86_tlb_t *)obj;
        int idx, way;

        if (val->kind != Sim_Val_List && val->kind != Sim_Val_Nil)
                return Sim_Set_Need_List;

        if (val->kind == Sim_Val_List) {
                if (id->large) {
                        switch (ptr->tlb_mode) {
                        case Mode_Hash_Table:
                                if (id->way != Sim_DI_Data)
                                        return Sim_Set_Illegal_Value;
                                if (val->u.list.size != 1)
                                        return Sim_Set_Illegal_Value;
                                if (!set_tlb_check_valid(val, 0, ptr->large_page_size - 1))
                                        return Sim_Set_Illegal_Value;
                                break;

                        case Mode_Multi_Level_Table:
                                if (id->way != Sim_DI_Data)
                                        return Sim_Set_Illegal_Value;
                                if (val->u.list.size != 1)
                                        return Sim_Set_Illegal_Value;
                                if (!set_tlb_check_valid(val, 0, ptr->large_page_size - 1))
                                        return Sim_Set_Illegal_Value;
                                break;

                        case Mode_Limited:
                                /* we can handle the case where the tlb is larger than in the checkpoint */
                                if (val->u.list.size > X86_LARGE_TLB_SIZE)
                                        return Sim_Set_Illegal_Value;
                                if (!set_tlb_check_valid(val, X86_LARGE_TLB_ASSOC, (ptr->large_page_size - 1) & ~(linear_address_t)1))
                                        return Sim_Set_Illegal_Value;
                        }
                } else { /* 4k */
                        switch (ptr->tlb_mode) {
                        case Mode_Hash_Table:
                                if (id->way != Sim_DI_Data)
                                        return Sim_Set_Illegal_Value;
                                if (val->u.list.size != 1)
                                        return Sim_Set_Illegal_Value;
                                if (!set_tlb_check_valid(val, 0, 0xfff))
                                        return Sim_Set_Illegal_Value;
                                break;

                        case Mode_Multi_Level_Table:
                                if (id->way != Sim_DI_Data)
                                        return Sim_Set_Illegal_Value;
                                if (val->u.list.size != 1)
                                        return Sim_Set_Illegal_Value;
                                if (!set_tlb_check_valid(val, 0, 0xfff))
                                        return Sim_Set_Illegal_Value;
                                break;

                        case Mode_Limited:
                                /* we can handle the case where the tlb is larger than in the checkpoint */
                                if (val->u.list.size > X86_4K_TLB_SIZE)
                                        return Sim_Set_Illegal_Value;
                                if (!set_tlb_check_valid(val, X86_4K_TLB_ASSOC, 0xffe))
                                        return Sim_Set_Illegal_Value;
                                break;
                        }
                }
        }

        if (id->large) {
                switch (ptr->tlb_mode) {
                case Mode_Hash_Table:
                        ht_clear_table(&ptr->hash_large, 1, 0);
                        break;

                case Mode_Multi_Level_Table:
                        ptr->table_large_head.next = ptr->table_large_head.prev = &ptr->table_large_head;
                        memset(ptr->table_large, 0, sizeof(ptr->table_large));
                        break;

                case Mode_Limited:
                        for (idx = 0; idx < X86_LARGE_TLB_SIZE; idx++) {
                                for (way = 0; way < X86_LARGE_TLB_ASSOC; way++) {
                                        INVALIDATE(ptr->pc_tlb_large[id->way][idx][way]);
                                }
                        }
                        break;
                }
        } else { /* 4k */
                switch (ptr->tlb_mode) {
                case Mode_Hash_Table:
                        ht_clear_table(&ptr->hash_4k, 1, 0);
                        break;

                case Mode_Multi_Level_Table:
                        ptr->table_4k_head.next = ptr->table_4k_head.prev = &ptr->table_4k_head;
                        memset(ptr->table_4k, 0, sizeof(ptr->table_4k));
                        break;

                case Mode_Limited:
                        for (idx = 0; idx < X86_4K_TLB_SIZE; idx++) {
                                for (way = 0; way < X86_4K_TLB_ASSOC; way++) {
                                        INVALIDATE(ptr->pc_tlb_4k[id->way][idx][way]);
                                }
                        }
                        break;
                }
        }

        if (val->kind == Sim_Val_List) {
                if (id->large) {
                        switch (ptr->tlb_mode) {
                        case Mode_Hash_Table:
                                for (idx = 0; idx < val->u.list.size; idx++) {
                                        for (way = 0; way < val->u.list.vector[idx].u.list.size; way++) {
                                                attr_value_t *tlb_entry = &val->u.list.vector[idx].u.list.vector[way];
                                                tlb_entry_t *entry = MM_ZALLOC(1, tlb_entry_t);
                                                set_tlb_entry(entry, tlb_entry);
                                                ht_insert_int(&ptr->hash_large, entry->linear_page_start >> ptr->large_shift, entry);
                                        }
                                }
                                break;

                        case Mode_Multi_Level_Table:
                                for (idx = 0; idx < val->u.list.size; idx++) {
                                        for (way = 0; way < val->u.list.vector[idx].u.list.size; way++) {
                                                attr_value_t *tlb_entry = &val->u.list.vector[idx].u.list.vector[way];
                                                tlb_entry_t entry;
                                                tlb_wrap_entry_t *wrap;
                                                set_tlb_entry(&entry, tlb_entry);
                                                wrap = &ptr->table_large[entry.linear_page_start >> ptr->large_shift];
                                                wrap->entry = entry;
                                                table_insert_entry(&ptr->table_large_head, wrap);
                                        }
                                }
                                break;

                        case Mode_Limited:
                                for (idx = 0; idx < val->u.list.size; idx++) {
                                        for (way = 0; way < X86_LARGE_TLB_ASSOC; way++) {
                                                attr_value_t *tlb_entry = &val->u.list.vector[idx].u.list.vector[way];
                                                tlb_entry_t *entry = &ptr->pc_tlb_large[id->way][idx][way];
                                                set_tlb_entry(entry, tlb_entry);
                                        }
                                }
                                break;
                        }
                } else { /* 4k */
                        switch (ptr->tlb_mode) {
                        case Mode_Hash_Table:
                                for (idx = 0; idx < val->u.list.size; idx++) {
                                        for (way = 0; way < val->u.list.vector[idx].u.list.size; way++) {
                                                attr_value_t *tlb_entry = &val->u.list.vector[idx].u.list.vector[way];
                                                tlb_entry_t *entry = MM_ZALLOC(1, tlb_entry_t);
                                                set_tlb_entry(entry, tlb_entry);
                                                ht_insert_int(&ptr->hash_4k, entry->linear_page_start >> 12, entry);
                                        }
                                }
                                break;

                        case Mode_Multi_Level_Table:
                                for (idx = 0; idx < val->u.list.size; idx++) {
                                        for (way = 0; way < val->u.list.vector[idx].u.list.size; way++) {
                                                attr_value_t *tlb_entry = &val->u.list.vector[idx].u.list.vector[way];
                                                tlb_entry_t entry;
                                                tlb_wrap_entry_t *wrap;
                                                set_tlb_entry(&entry, tlb_entry);
                                                wrap = &ptr->table_4k[entry.linear_page_start >> 12];
                                                wrap->entry = entry;
                                                table_insert_entry(&ptr->table_4k_head, wrap);
                                        }
                                }
                                break;

                        case Mode_Limited:
                                for (idx = 0; idx < val->u.list.size; idx++) {
                                        for (way = 0; way < X86_4K_TLB_ASSOC; way++) {
                                                attr_value_t *tlb_entry = &val->u.list.vector[idx].u.list.vector[way];
                                                tlb_entry_t *entry = &ptr->pc_tlb_4k[id->way][idx][way];
                                                set_tlb_entry(entry, tlb_entry);
                                        }
                                }
                                break;
                        }
                }
        }

        /* Flush data structures that depend on the address mapping. */
        if (ptr->cpu)
                SIM_STC_flush_cache(ptr->cpu);
        return Sim_Set_Ok;
}

typedef struct {
        attr_value_t *attr;
        int i;
} hash_iter_t;

static int
get_hash_entry(ht_table_t *table, uint64 key, void *value, void *data)
{
        tlb_entry_t *entry = (tlb_entry_t *)value;
        hash_iter_t *iter = (hash_iter_t *)data;
        get_tlb_entry(&iter->attr->u.list.vector[iter->i], entry);
        iter->i++;
        return 1;
}

static attr_value_t
get_x86_tlb(void *_id, conf_object_t *obj, attr_value_t *_unused)
{
        x86_tlb_id_t *id = (x86_tlb_id_t *)_id;
        x86_tlb_t *ptr = (x86_tlb_t *)obj;
        int idx, way;
        attr_value_t ret;
        hash_iter_t hash_iter;
        tlb_wrap_entry_t *p;
        int i, size;
        ret.kind = Sim_Val_List;
        if (id->large) {
                switch (ptr->tlb_mode) {
                case Mode_Hash_Table:
                        if (id->way != Sim_DI_Data)
                                return SIM_make_attr_nil();
                        ret = SIM_alloc_attr_list(1);
                        ret.u.list.vector[0] = SIM_alloc_attr_list(ht_num_entries(&ptr->hash_large));
                        hash_iter.attr = &ret.u.list.vector[0];
                        hash_iter.i = 0;
                        ht_for_each_entry(&ptr->hash_large, get_hash_entry, &hash_iter);
                        ASSERT(hash_iter.i == ret.u.list.vector[0].u.list.size);
                        break;
                case Mode_Multi_Level_Table:
                        if (id->way != Sim_DI_Data)
                                return SIM_make_attr_nil();
                        ret = SIM_alloc_attr_list(1);
                        size = 0;
                        for (p = ptr->table_large_head.next; p != &ptr->table_large_head; p = p->next) {
                                if (p->valid)
                                        size++;
                        }
                        ret.u.list.vector[0] = SIM_alloc_attr_list(size);
                        i = 0;
                        for (p = ptr->table_large_head.next; p != &ptr->table_large_head; p = p->next) {
                                if (p->valid) {
                                        get_tlb_entry(&ret.u.list.vector[0].u.list.vector[i], &p->entry);
                                        i++;
                                }
                        }
                        break;
                case Mode_Limited:
                        ret = SIM_alloc_attr_list(X86_LARGE_TLB_SIZE);
                        for (idx = 0; idx < X86_LARGE_TLB_SIZE; idx++) {
                                ret.u.list.vector[idx] = SIM_alloc_attr_list(X86_LARGE_TLB_ASSOC);
                                for (way = 0; way < X86_LARGE_TLB_ASSOC; way++) {
                                        attr_value_t *tlb_entry = &ret.u.list.vector[idx].u.list.vector[way];
                                        tlb_entry_t *entry = &ptr->pc_tlb_large[id->way][idx][way];
                                        get_tlb_entry(tlb_entry, entry);
                                }
                        }
                        break;
                }
        } else { /* 4k */
                switch (ptr->tlb_mode) {
                case Mode_Hash_Table:
                        if (id->way != Sim_DI_Data)
                                return SIM_make_attr_nil();
                        ret = SIM_alloc_attr_list(1);
                        ret.u.list.vector[0] = SIM_alloc_attr_list(ht_num_entries(&ptr->hash_4k));
                        hash_iter.attr = &ret.u.list.vector[0];
                        hash_iter.i = 0;
                        ht_for_each_entry(&ptr->hash_4k, get_hash_entry, &hash_iter);
                        ASSERT(hash_iter.i == ret.u.list.vector[0].u.list.size);
                        break;
                case Mode_Multi_Level_Table:
                        if (id->way != Sim_DI_Data)
                                return SIM_make_attr_nil();
                        ret = SIM_alloc_attr_list(1);
                        size = 0;
                        for (p = ptr->table_4k_head.next; p != &ptr->table_4k_head; p = p->next) {
                                if (p->valid)
                                        size++;
                        }
                        ret.u.list.vector[0] = SIM_alloc_attr_list(size);
                        i = 0;
                        for (p = ptr->table_4k_head.next; p != &ptr->table_4k_head; p = p->next) {
                                if (p->valid) {
                                        get_tlb_entry(&ret.u.list.vector[0].u.list.vector[i], &p->entry);
                                        i++;
                                }
                        }
                        break;
                case Mode_Limited:
                        ret = SIM_alloc_attr_list(X86_4K_TLB_SIZE);
                        for (idx = 0; idx < X86_4K_TLB_SIZE; idx++) {
                                ret.u.list.vector[idx] = SIM_alloc_attr_list(X86_4K_TLB_ASSOC);
                                for (way = 0; way < X86_4K_TLB_ASSOC; way++) {
                                        attr_value_t *tlb_entry = &ret.u.list.vector[idx].u.list.vector[way];
                                        tlb_entry_t *entry = &ptr->pc_tlb_4k[id->way][idx][way];
                                        get_tlb_entry(tlb_entry, entry);
                                }
                        }
                        break;
                }
        }
        return ret;
}

static set_error_t
set_tlb_select(void *_id, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        x86_tlb_t *ptr = (x86_tlb_t *)obj;
        tlb_mode_t old_mode = ptr->tlb_mode;
        if (val->kind != Sim_Val_Integer)
                return Sim_Set_Need_Integer;
        if (val->u.integer != 2 && val->u.integer != 4)
                return Sim_Set_Illegal_Value;
        ptr->in_pae_mode = (val->u.integer == 2);
        if (ptr->in_pae_mode) {
                ptr->large_page_size = 2*1024*1024;
                ptr->large_shift = 21;
        } else {
                ptr->large_page_size = 4*1024*1024;
                ptr->large_shift = 22;
        }
        if (ptr->tlb_mode == Mode_Multi_Level_Table && ptr->in_pae_mode)
                ptr->tlb_mode = Mode_Hash_Table;
        else if (ptr->tlb_mode == Mode_Hash_Table && !ptr->in_pae_mode)
                ptr->tlb_mode = Mode_Multi_Level_Table;
        enter_tlb_mode(ptr, old_mode, ptr->tlb_mode);
        return Sim_Set_Ok;
}

static attr_value_t
get_tlb_select(void *_id, conf_object_t *obj, attr_value_t *idx)
{
        x86_tlb_t *ptr = (x86_tlb_t *)obj;
        return SIM_make_attr_integer(ptr->in_pae_mode ? 2 : 4);
}

static set_error_t
set_tlb_type(void *_id, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        x86_tlb_t *ptr = (x86_tlb_t *)obj;
        tlb_mode_t old_mode = ptr->tlb_mode;
        if (strcmp(val->u.string, "normal") == 0) {
                ptr->tlb_mode = Mode_Limited;
                enter_tlb_mode(ptr, old_mode, ptr->tlb_mode);
                return Sim_Set_Ok;
        } else if (strcmp(val->u.string, "unlimited") == 0) {
                if (ptr->in_pae_mode)
                        ptr->tlb_mode = Mode_Hash_Table;
                else
                        ptr->tlb_mode = Mode_Multi_Level_Table;
                enter_tlb_mode(ptr, old_mode, ptr->tlb_mode);
                return Sim_Set_Ok;
        } else {
                return Sim_Set_Illegal_Value;
        }
}

static attr_value_t
get_tlb_type(void *_id, conf_object_t *obj, attr_value_t *idx)
{
        x86_tlb_t *ptr = (x86_tlb_t *)obj;
        if (ptr->tlb_mode == Mode_Limited)
                return SIM_make_attr_string("normal");
        else
                return SIM_make_attr_string("unlimited");
}

#define TLB_FORMAT_STRING \
   "(((la, pa, mode, rw, g, pat_type, mtrr_type){associativity}){way-size}). "

DLL_EXPORT void
init_local(void)
{
        class_data_t           class_data;
        conf_class_t          *conf_class;
        x86_tlb_interface_t   *tlb_iface;
        x86_tlb_id_t          *tlb_id;

        memset(&class_data, 0, sizeof(class_data_t));
        class_data.new_instance = x86_tlb_new_instance;
        class_data.description = DEVICE_INFO_STRING;
        conf_class = SIM_register_class(DEVICE_NAME, &class_data);

        tlb_iface = MM_ZALLOC(1, x86_tlb_interface_t);
        tlb_iface->flush_all = x86_tlb_flush_all;
        tlb_iface->flush_page = x86_tlb_flush_page;
        tlb_iface->lookup = x86_tlb_lookup;
        tlb_iface->add = x86_tlb_add;
        tlb_iface->itlb_lookup = x86_tlb_itlb_lookup;
        SIM_register_interface(conf_class, X86_TLB_INTERFACE, tlb_iface);

        SIM_register_typed_attribute(conf_class, "cpu", get_cpu, 0,
                set_cpu, 0, Sim_Attr_Required, "o", NULL,
                "CPU object to which TLB object is bound.");
        SIM_register_typed_attribute(conf_class, "type", get_tlb_type, 0,
                set_tlb_type, 0, Sim_Attr_Optional, "s", NULL,
                "Type of TLB. Can be either normal for a set associative TLB,"
                " or unlimited for a very large TLB.");
        SIM_register_attribute(conf_class, "large_tlb_select", get_tlb_select,
                0, set_tlb_select, 0, Sim_Attr_Optional,
                "Determines the size for pages mapped in the large page TLBs."
                " 2 for 2Mb and 4 for 4Mb.");
        tlb_id = x86_tlb_id(Sim_DI_Instruction, 1);
        SIM_register_attribute(conf_class, "itlb_large", get_x86_tlb,
                tlb_id, set_x86_tlb, tlb_id, Sim_Attr_Optional,
                TLB_FORMAT_STRING "Instruction TLB for 2Mb and 4Mb pages.");
        tlb_id = x86_tlb_id(Sim_DI_Data, 1);
        SIM_register_attribute(conf_class, "dtlb_large", get_x86_tlb,
                tlb_id, set_x86_tlb, tlb_id, Sim_Attr_Optional,
                TLB_FORMAT_STRING "Data TLB for 2Mb and 4Mb pages.");
        tlb_id = x86_tlb_id(Sim_DI_Instruction, 0);
        SIM_register_attribute(conf_class, "itlb_4k", get_x86_tlb,
                tlb_id, set_x86_tlb, tlb_id, Sim_Attr_Optional,
                TLB_FORMAT_STRING "Instruction TLB for 4kb pages.");
        tlb_id = x86_tlb_id(Sim_DI_Data, 0);
        SIM_register_attribute(conf_class, "dtlb_4k", get_x86_tlb,
                tlb_id, set_x86_tlb, tlb_id, Sim_Attr_Optional,
                TLB_FORMAT_STRING "Data TLB for 4kb pages.");

	x86_memory_type_descr[X86_None] = "none";
        x86_memory_type_descr[X86_Strong_Uncacheable] = "UC";
	x86_memory_type_descr[X86_Uncacheable] = "UC-";
	x86_memory_type_descr[X86_Write_Combining] = "WC";
	x86_memory_type_descr[X86_Write_Through] = "WT";
	x86_memory_type_descr[X86_Write_Back] = "WB";
	x86_memory_type_descr[X86_Write_Protected] = "WP";

        x86_hap_tlb_fill[Sim_DI_Instruction] =
                SIM_hap_add_type("TLB_Fill_Instruction",
                                 "III", "linear physical page_size",
                                 "page_size",
                                 "Triggered when a TLB entry is filled after a table walk. "
                                 "Page size encoding: 0==4k, 1==2M, 2==4M.", 0);
        x86_hap_tlb_fill[Sim_DI_Data] =
                SIM_hap_add_type("TLB_Fill_Data",
                                 "III", "linear physical page_size",
                                 "page_size",
                                 "Triggered when a TLB entry is filled after a table walk. "
                                 "Page size encoding: 0==4k, 1==2M, 2==4M.", 0);
        x86_hap_tlb_replace[Sim_DI_Instruction] =
                SIM_hap_add_type("TLB_Replace_Instruction",
                                 "III", "linear physical page_size",
                                 "page_size",
                                 "This hap is triggered when a TLB entry is replaced by "
                                 "another. The parameters relate to the old entry, and the "
                                 "insertion of the new entry will trigger a fill hap. "
                                 "Page size encoding: 0==4k, 1==2M, 2==4M.", 0);
        x86_hap_tlb_replace[Sim_DI_Data] =
                SIM_hap_add_type("TLB_Replace_Data",
                                 "III", "linear physical page_size",
                                 "page_size",
                                 "This hap is triggered when a TLB entry is replaced by "
                                 "another. The parameters relate to the old entry, and the "
                                 "insertion of the new entry will trigger a fill hap. "
                                 "Page size encoding: 0==4k, 1==2M, 2==4M.", 0);
        x86_hap_tlb_invalidate[Sim_DI_Instruction] =
                SIM_hap_add_type("TLB_Invalidate_Instruction",
                                 "III", "linear physical page_size",
                                 "page_size",
                                 "Triggered when a TLB entry is invalidated. The invalidation "
                                 "can be caused by an INVLPG instruction, a write to CR3, "
                                 "or by changes to paging bits in CR0 and CR4. "
                                 "Page size encoding: 0==4k, 1==2M, 2==4M.", 0);
        x86_hap_tlb_invalidate[Sim_DI_Data] =
                SIM_hap_add_type("TLB_Invalidate_Data",
                                 "III", "linear physical page_size",
                                 "page_size",
                                 "Triggered when a TLB entry is invalidated. The invalidation "
                                 "can be caused by an INVLPG instruction, a write to CR3, "
                                 "or by changes to paging bits in CR0 and CR4. "
                                 "Page size encoding: 0==4k, 1==2M, 2==4M.", 0);
        x86_hap_tlb_miss[Sim_DI_Instruction] =
                SIM_hap_add_type("TLB_Miss_Instruction",
                                 "I", "linear_address",
                                 "linear_address",
                                 "Triggered when an ITLB miss occurs.", 0);
        x86_hap_tlb_miss[Sim_DI_Data] =
                SIM_hap_add_type("TLB_Miss_Data",
                                 "I", "linear_address",
                                 "linear_address",
                                 "Triggered when a DTLB miss occurs.", 0);
}
