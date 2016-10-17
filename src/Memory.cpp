#include "Memory.hpp"

Memory::Memory(O3sim_ruby* _ruby)
{
  m_num_core = 1;
  m_num_l3   = 1;
  m_num_mc   = 1;

  m_core_id =0;
  m_thread_id =0;

  this->initialize();
  m_ruby = _ruby;

}


void Memory::initialize()
{
  m_mem_mshr_stack = MEM_MSHR_SIZE;
  mem_req_stack = MEM_MSHR_SIZE;

  ldst_addr_buffer.clear();
}

void Memory::print_mem_stats()
{
    cout<<"----------------------------------\n";
    cout<<"Printing Chainsaw memory state \n";

  std::cout<<"m_mem_mshr_stack: "<<m_mem_mshr_stack<<endl;
  std::cout<<"mem_req_stack: "<<mem_req_stack<<endl;
  std::cout<<"ldst_addr_buffer size: "<<ldst_addr_buffer.size()<<endl;

  for(auto &mem_value: ldst_addr_buffer)
  {
      cout<<"vaddr:" <<mem_value.vaddr<<endl;
      cout<<"isWrite:"<<mem_value.isWrite<<endl;
      cout<<"is_issue:"<<mem_value.is_issue<<endl;
  }
}

Memory::~Memory()
{
  ldst_addr_buffer.clear();
  memaddr_map.clear();
}


void Memory::send_req( uint64_t iter, uint64_t nodeid/*, bool isWrite*/ )
{
  auto key =std::make_pair (iter,nodeid);
  if (memaddr_map.find(key) != memaddr_map.end())
  {
    MemValue mem_value = memaddr_map[key];
    bool isWrite = mem_value.isWrite;
    Addr vaddr = mem_value.addr;
    MemValue_t ldst_value = MemValue_t{isWrite,nodeid,vaddr,false};
    // ldst_addr_buffer.insert( std::make_pair#<{(|<Addr,MemValue_t>|)}># (vaddr,ldst_value )); 
    ldst_addr_buffer.push_back(ldst_value);
  }
  else
    assert(0 && "Chainsaw: sent request not in memaddr_map \n");


}

//checks each cycle
std::list<uint64_t> Memory::recv_resp()
{
  // m_ruby->advance_time();
  this->access();
  return process_mshr();
}

//checks each cycle
bool Memory::access()
{

  if(!ldst_addr_buffer.empty() && mem_req_stack!=0)
  {

    //-------------
    for( auto &mem_value: ldst_addr_buffer)
    {
      // MemValue_t  mem_value = ldst_addr_buffer.front();
      Addr req_addr = base_addr(mem_value.vaddr);
      uint64_t req_size  = line_size();
      bool isWrite = mem_value.isWrite;
      bool is_issue = mem_value.is_issue;


      if(m_ruby->isReady(req_addr,req_size,m_core_id, m_thread_id, false, false, NULL) && !is_issue )
      {
        mem_value.is_issue = true;

        //D(std::cout<<"m_ruby: send_req: req_addr:"<<req_addr<<"\t"
            //"req_size:"<<req_size<<"\t"
            //<<"nodeid: "<<mem_value.nodeid <<"\t"
            //<<"is_issue:"<<mem_value.is_issue
            //<<endl);
        --mem_req_stack;
        m_ruby->send_request(req_addr, req_size, m_core_id,m_thread_id, isWrite, false, NULL);
        return true;
      }


    }

    return false;
    //-------------
  }
  return false;
}


// get base line address
// replace 63 with the cache line size
Addr 
Memory::base_addr(Addr addr)
{
  return (addr & ~63);
}


// get cache line size
uint64_t 
Memory::line_size()
{
  return 64; 
}


std::list<uint64_t> 
Memory::process_mshr()
{

  uint64_t tmp_line_size = line_size();
  std::list<uint64_t> m_mem_resp_list;

  while(!m_ruby->RubyQueueEmpty(m_core_id))
  {

    ++mem_req_stack;
    Addr addr = m_ruby->RubyQueueTop(m_core_id);
    // Search the mshr for the request
    const MemValue_t* matching_req = search_req(addr, tmp_line_size);
    assert(matching_req != NULL && "Addr returned from Ruby not found in ldst_addr_buffer");

    m_mem_resp_list.push_back(matching_req->nodeid);
    ldst_addr_buffer.remove(*matching_req);
    m_ruby->RubyQueuePop(m_core_id);
  }

  return m_mem_resp_list;
}


///////////////////////////////////////////////////////////////////////////////////////////////

// search matching request
MemValue_t* 
Memory::search_req(Addr addr, uint64_t size)
{
  for( auto &mem_value : ldst_addr_buffer)
  {
    Addr vaddr = mem_value.vaddr;
    //XXX: vaddr + size => size corresponds to m_mem_size from macsim : in uop.cc 
    //This size actually varies from opcode to opcode. and to set this we need a decoder i.e we need more info
    //than just Load op or store op
    // if(vaddr <= addr && vaddr + size >= addr+size)
    if(vaddr >= addr && vaddr < addr+size)
      return &mem_value;
  }

  return NULL;
}

void Memory::fill_global_memaddr_map(uint64_t iter, uint64_t nodeid , MemValue mem_value)
{
  memaddr_map[std::make_pair (iter,nodeid)] = ( mem_value );
}

void Memory::print_global_memaddr_map()
{
  for( auto &x : memaddr_map)
  {
    cout<<"iter:"<<x.first.first <<"\t";
    cout<<"nodeid:"<<x.first.second <<"\t";
    // cout<<"opcode:"<<x.second.opcode <<"\t";
    cout<<"isWrite:"<<x.second.isWrite <<"\t";
    cout<<"mem_addr::"<<x.second.addr<<endl;

  }
}

