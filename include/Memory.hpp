#ifndef __MEMORY__
#define __MEMORY__

#include "common.hpp"
#include <functional>
#include <fstream>
#include <list>
#include <map>
#include <unordered_map>
#include <assert.h>
#include "O3sim_ruby.h"

using namespace std;


//using namespace std;

#define MEM_MSHR_SIZE  10

typedef unsigned long long uns64;
typedef uns64 Addr;

struct MemValue 
{
  bool  isWrite;
  // std::string opcode;
  Addr addr;
};

struct MemValue_t
{
  bool isWrite;
  uint64_t nodeid;
  Addr vaddr;
  bool is_issue;
  bool operator==(const MemValue_t& lhs)
  {
    return (lhs.isWrite == isWrite && lhs.nodeid == nodeid && lhs.vaddr == vaddr);
  }
};

  template <class T>
inline void hash_combine(std::size_t & seed, const T & v)
{
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

namespace std
{
  template<typename S, typename T> struct hash<pair<S, T>>
  {
    inline size_t operator()(const pair<S, T> & v) const
    {
      size_t seed = 0;
      ::hash_combine(seed, v.first);
      ::hash_combine(seed, v.second);
      return seed;
    }
  };
}




class Memory
{
    public:
    O3sim_ruby* m_ruby;
  private:
    //--------------------------------
    std::unordered_map< std::pair<uint64_t,uint64_t>, MemValue > memaddr_map;
    // std::unordered_map<uint64_t, MemValue_t> ldst_addr_buffer;
    std::list<MemValue_t> ldst_addr_buffer;
    //--------------------------------
    int m_num_core=1; /**< number of cores */
    int m_num_l3=1; /**< number of l3 caches */
    int m_num_mc=1; /**< number of memory controllers */
    int m_core_id=0;
    int m_thread_id=0;

    int  mem_req_stack;
    uint64_t m_mem_mshr_stack = MEM_MSHR_SIZE;

  public:
    Memory(O3sim_ruby* _ruby);
    ~Memory();

    void print_mem_stats();
    void initialize();
    bool access();
    void send_req( uint64_t iter, uint64_t nodeid /*, bool isWrite*/ );
    std::list<uint64_t> recv_resp();
    void advance_time();
    std::list<uint64_t> process_mshr();
    MemValue_t* search_req(Addr addr, uint64_t size);
    // void run_a_cycle(void);
    
    uint64_t line_size();
    static Addr base_addr(Addr addr);
    void run_a_cycle_core(int core_id);
    void fill_global_memaddr_map(uint64_t iter, uint64_t nodeid , MemValue mem_value);
    void print_global_memaddr_map();
};
#endif 


/*
 *Iter:
 Ld/St
 Memory addr
nodeid:
-----------------------------------------
for every above info -- create a uop
-- to integrate with macsim
-------------

 *
 *
 */
