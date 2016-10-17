#include "Core.hpp"

using namespace std;
using namespace boost;


//Finid minimum value of an array with its limit
inline uint32_t
findMinimum(std::array<uint32_t,MAX_NUM_CHAIN>& arr, uint32_t limit)
{
    uint32_t min = arr[0];
    uint32_t index = 0;
    for(uint32_t i = 1; i < limit; i++){
        if(min > arr[i]){
            min = arr[i];
            index = i;
        }
    }
    return index;
}


/**
* Getting a Dilworth's algorithm output and building
* Chains with their dependencies and fill "input_chain"
* @param chains Decomposed graph
*/
void
Core::buildChains(
        std::vector<std::vector<uint64_t>> decom_chains,
        std::map<uint64_t, uint64_t> chain_mapping,
        std::vector<chainDependency> chain_deppendenceis,
        std::map<uint32_t, std::set<uint32_t>> un_dependencies,
        std::map<uint32_t, std::set<uint32_t>> reverse_dependencies,
        std::map<uint32_t, std::vector<uint32_t>> chain_live_out,
        std::map<uint32_t, uint32_t> node_map,
        BoostGraphComp& G)
{

    uint32_t i = 0;
    for(const auto& c : decom_chains){

        Chain newChain(i);

        //Adding instructions into the chain
        for (auto& ins: c){
            Node node_m;
            node_m.name = ins;
            node_m.new_id = ins;
            node_m.old_id = node_map[ins];
            node_m.type = G[ins].opcode;
            node_m.latency = llvm_ins.instruction_llvm.find(G[ins].opcode)->second;
            node_m.c_id = i;
            newChain.id = i;
            newChain.push_ins(node_m);

            //Check whether chain has MEM operations
            if((G[ins].opcode == "Load") || (G[ins].opcode == "Store"))
                newChain.ch_mem = true;
        }

        auto it_depen = reverse_dependencies.find(i);

        //Finding Live-ins
        if(it_depen != reverse_dependencies.end()){
            newChain.live_in = it_depen->second; 
            newChain.num_dependencies = newChain.live_in.size();
        }

        auto it_un_depen = un_dependencies.find(i);
        //Finding Live-outs
        if(it_un_depen != un_dependencies.end())
            newChain.live_out = it_un_depen->second; 

        //Adding the new node to the list
        input_chain.push_back(newChain);

        i++;
    }

    this->un_dependencies = un_dependencies;

    num_chains = input_chain.size();
}

/*
 * Check whether the scheduler has ran all the chains or not?
 */
bool
Core::is_finish()
{
    if(!input_chain.empty())
        return false;
    else{
        for(auto& lane: m_lanes){
            if(!lane.is_free())
                return false;
        }
    }

    return true;
}





/*
 * Runing all the lanes
 */
void
Core::run_all()
{

    //Running all the lanes
    //

    //1) Filling dependency table
    initialize_depen_table();

    //2) Prescheduling lanes
    //Based on "SCHEDULER" parameter:
    //1:    Block scheduling
    //0:    Level scheduling
    if(core_parameter.param_set.find("SCHEDULER")->second)
        block_preschedule();
    else
        preschedule();

    //3) Start processing graphs in a while loop
   
    std::stringstream f;
    f << "output/lane.out";
    lane_status_file.open(f.str(), ios::out);
    f.str("");

    f << "Cycle";
    for(uint32_t i = 0; i < m_lanes.size(); i++)
        f << ", Lane" << i;
    f << endl;
    lane_status_file << f.str();
    lane_status_file.close();

    cycle_cnt = 0;

    //Core's cycle
    while(!is_finish()){
        ////Increamenting program counter
        cycle_cnt++;

        cout << "Cycle: " << cycle_cnt << endl;

        // In each cycle:
        // 1- Run one cycle core:
        //      1- Run each lane
        //      2- Run Ruby one cycle
        // 2- Send lane's mem requests into memory
        // 3- Update exe counter of lanes having mem request
        // 4- Rescheduling 
        //      1- Update chains' dependencies
        //      2- Find new free chains and add them to ready chains
        //
        //Step 1)
        run_one_cycle_core();

        //Step 2)
        send_memory_req();

        //Step 3)
        update_mem_exe();

        //Step 4)
        updateChains();

        //Step 5)
        ooo_scheduler();

    }

    cout << "***************************" << endl;
    cout << "Total cycle: " << cycle_cnt  << endl;
    cout << "***************************" << endl;

}


/**
 * Initilize dependency table
 * Dependency table is matrix which shows chain dependencies
 */
void
Core::initialize_depen_table()
{
    for(auto& dep : un_dependencies){
        for(auto& ch_id: dep.second){
            depen_table[ch_id][dep.first] = 1;
        }
    }
}

/**
 * Filling lanes' buffer and preschedule them
 */
void
Core::preschedule()
{
    assert(m_graph.chain_graph.size() > 0 && "\nERROR: Before scheduling you should load the graph");

    std::map<uint32_t, std::vector<uint32_t>> chain_map_level;

    for(auto& ch : m_graph.chain_graph)
        chain_map_level[ch.level].push_back(ch.chainId);

    //XXX Printing levels
    for(auto& ch : chain_map_level){
        cout << "level: " << ch.first << endl;
        for(auto &a : ch.second)
            cout << a << endl;
    }

    //Fill the scheduling stack base on the levels
    if(stack_sche_chain.size() == 0){
        for(auto&ch_l : chain_map_level){
            for(auto&ch : ch_l.second)
                stack_sche_chain.push_back(ch);
        }
    }


    //Scheduling NORMAL lanes
    for(auto& sch_ch : stack_sche_chain){
        //Find chain object
        auto ch_it = std::find_if(input_chain.begin(), input_chain.end(),
                [&sch_ch](Chain const& item) -> bool
                {
                return item.id == sch_ch;
                });

        //Check wether scheduling is correct
        assert(ch_it != input_chain.end() && "\nERROR: Wrong scheduling!\n");

        //Find a lane which has more available free slots
        uint32_t lane_index = findMinimum(lane_pre_schedule_size, num_lanes);

        //Schedul the chain to minimum lane
        lane_pre_schedule[lane_index].push_back(sch_ch);

        //Decrease size of free slots
        lane_pre_schedule_size[lane_index] += ch_it->instruction_list.size();

    }

    //Print final scheduling
    uint32_t k =0;
    for(auto& lane: lane_pre_schedule){
        cout << "Lane[" << k++ << "]: \n";
        for(auto& c : lane)
            cout << c << endl;
    }

    internal_com = 0;
    external_com = 0;
    //Find internal and external communications
    for(auto& dep : m_graph.dependencies){
        uint32_t source = dep.source;
        uint32_t target = dep.target;

        uint32_t l_source = m_lanes.size() + 1;
        uint32_t l_target = m_lanes.size() + 2;

        uint32_t i = 0;
        for(auto& lane: lane_pre_schedule){

            auto it = std::find_if(lane.begin(), lane.end(),
                    [&source] (uint32_t const& item) -> bool
                    {
                        return item == source;
                    });
            if(it != lane.end()){
                l_source = i;
                break;
            }
            else
                i++;
        }

        uint32_t j = 0;
        for(auto& lane: lane_pre_schedule){
            auto n_it = std::find_if(lane.begin(), lane.end(),
                    [&target] (uint32_t const& item) -> bool
                    {
                        return item == target;
                    });
            if(n_it != lane.end()){
                l_target = j;
                break;
            }
            else
                j++;

        }

        if(l_source == l_target)
            internal_com++;
        else
            external_com++;

    }

}


/*
 * Run one cycle of the core
 */
void
Core::run_one_cycle_core()
{
    //Running normal lanes
    for(auto& l : m_lanes){
        l.run();
    }

    //XXX uncomment if it's not stand alone
    m_memory.m_ruby->advance_time();
}


/*
 * Looking at lanes' memory queue and send their "Load" requests
 * to the memory system
 */
void
Core::send_memory_req()
{
    while(!Lane::mem_req_queue.empty()){
        //DBGVAR(std::cout, Lane::mem_req_queue.size());
        auto& in_req = Lane::mem_req_queue.front();
        Lane::mem_req_queue.pop();
        uint32_t l_id;
        uint32_t n_id;
        std::tie(l_id, n_id) = in_req;
        mem_req_map[l_id] = n_id;

        m_memory.send_req(num_iter,n_id);
    }

}

/*
 * Update lanes that are waiting for their memory request
 * It looks at memory out queue and pop all the ready requests
 */
void
Core::update_mem_exe()
{
    //Wakeup the lanes
    //
    //
    //
    auto nodeid_list = m_memory.recv_resp();
    for( auto &n : nodeid_list)
    {
        //cerr<<"graph: get resp from memory nodeid: "<<n<<endl;

        auto lane_it = find_if(mem_req_map.begin(), mem_req_map.end(),
                [n](const std::pair<uint32_t, uint32_t>& temp_mem) -> bool{
                return temp_mem.second == n;
                });
        m_lanes[lane_it->first].setExeCounter(0);

    }
}


/**
 * This function reads from pre-schedule buffer and load instruction
 * into the lanes at each cycle
 * the algorithm is out of order
 */
void
Core::ooo_scheduler()
{
    stringstream f;
    f << "output/lane.out";
    lane_status_file.open(f.str(), ios::app);

    std::stringstream m_lane_status;
    m_lane_status << cycle_cnt ;

    for(uint32_t i = 0; i < m_lanes.size(); ++i){

        bool is_free = false;

        //Check whether lane is free or not
        if(!m_lanes[i].is_free()){
            m_lane_status << ", C" << m_lanes[i].m_current_ch << "(" <<
                m_lanes[i].m_current_op << ")";
            continue;
        }

        //Reads scheduled chains for a lane
        //looks at the dependancy table to see if
        //the chain is free or not
        //if it's free then we load it to the lane
        //otherwise we should wait
        auto& sche_buff = lane_pre_schedule[i];
        uint32_t temp_free_id;

        if(sche_buff.size() == 0){
            m_lane_status << ", FREE";
            core_free++;
        }
        else{
            for(auto& sch_ch: sche_buff){
                uint32_t temp_status=0;
                for(uint32_t j = 0; j < num_chains; ++j){
                    temp_status += depen_table[sch_ch][j];
                }
                if(temp_status == 0){
                    is_free = true;
                    temp_free_id = sch_ch;
                    break;
                }
            }
            m_lane_status << ", IDLE";
            core_idle++;
            m_lanes[i].m_stall++;
        }
        

        if(is_free){
            //Steps to load a chain
            //1- Remove chain from sche_buff
            //2- Find the chain inside input_chain
            //3- load the chain into the lane
            //4- remove it from input_chain
            //
            
            auto it = std::find_if(sche_buff.begin(), sche_buff.end(),
                    [&temp_free_id] (uint32_t const& item) -> bool
                    {
                    return item == temp_free_id;
                    });
            //if(it == sche_buff.end())
            assert(it != sche_buff.end() && "ERROR: Couldn't find chain in preschedule lanes");
            sche_buff.erase(it);
            sche_buff.shrink_to_fit();

            auto ch_it = std::find_if(input_chain.begin(), input_chain.end(), 
                    [&temp_free_id] (Chain const& item) -> bool
                    {
                    return item.id == temp_free_id;
                    });

            lane_pre_schedule_size[i] -= ch_it->instruction_list.size();

            if(ch_it != input_chain.end()){
                //std::cout << "\n**********\n";
                //std::cout << "Chain: " << temp_free_id << " Scheduled on Lane: " << i << std::endl;
                //std::cout << "********** \n";
                m_lanes[i].LoadInstructionQueue(ch_it->return_instruction(), temp_free_id);

                //Register Chain to Lane mapping
                chaintoLane[ch_it->id]=i;

                input_chain.erase(ch_it);
                input_chain.shrink_to_fit();
            }

        }
    }

    //Write lane status in the file
    m_lane_status << endl;

    lane_status_file << m_lane_status.str();
    lane_status_file.close();

}

/**
 * Reading from finished chains
 * and update dependencies base on 
 * completed chains
 */
void
Core::updateChains()
{
    //update dependency table
    while(!Lane::finished_chains.empty()){

        auto& temp_c = Lane::finished_chains.front();
        Lane::finished_chains.pop();

        auto ch_num = un_dependencies[temp_c];

        for(auto&ch_free : ch_num)
            depen_table[ch_free][temp_c] = 0;
    }
}


/**
 * Reading memory map file
 */
void
Core::read_memcsv(std::ifstream& file)
{
    boost::iostreams::filtering_streambuf<boost::iostreams::input> inbuf;
    inbuf.push(boost::iostreams::gzip_decompressor());
    inbuf.push(file);
    
    //Convert streambuf to istream
    std::istream instream(&inbuf);

    boost::char_separator<char> sep(",");

    std::string line;
    while(std::getline(instream, line)){
        std::vector<std::string> match;
        boost::tokenizer<boost::char_separator<char>> tokens(line, sep);
        for(const auto t : tokens)
            match.push_back(t);
        
        assert(match.size() == 4 && "\nAccelerator input file is in wrong format");

        uint64_t iter;
        uint64_t nodeid;
        string   opcode;
        bool isWrite =false;
        uint64_t addr;
        
        iter = stoi(match[0]);
        opcode = match[1];
        nodeid = stoi(match[2]);
        if(opcode == "Store")
            isWrite = true;
        else
            isWrite = false;
        addr = stoll(match[3]);
        
        m_memory.fill_global_memaddr_map( iter, nodeid , MemValue {isWrite,addr});
    }
}

/**
 * Printing memory map loaded from memory map file
 */
void
Core::print_memmap()
{
    m_memory.print_global_memaddr_map();
}


void
Core::coreSetIter(uint32_t n)
{
    this->num_iter = n;
}

bool
Core::run_a_cycle()
{
    ////Increamenting program counter
    //cycle_cnt++;
    
    // In each cycle:
    // 1- Run one cycle core:
    //      1- Run each lane
    //      2- Run Ruby one cycle
    // 2- Send lane's mem requests into memory
    // 3- Update exe counter of lanes having mem request
    // 4- Rescheduling 
    //      1- Update chains' dependencies
    //      2- Find new free chains and add them to ready chains
    //
    //Step 1)
    run_one_cycle_core();
    
    //Step 2)
    send_memory_req();
    
    //Step 3)
    update_mem_exe();
    
    //Step 4)
    updateChains();
    
    //Step 5)
    ooo_scheduler();
    
    //Step 6) Chech whether it has been finished
    return (is_finish());

}

/**
 * Initilizing core before start running
 */
void
Core::initialize()
{

    //1) Filling dependency table
    initialize_depen_table();

    //2) Prescheduling lanes
    //Based on "SCHEDULER" parameter:
    //1:    Block scheduling
    //0:    Level scheduling
    if(core_parameter.param_set.find("SCHEDULER")->second)
        block_preschedule();
    else
        preschedule();
}

/**
 * Scheduling chains base on their block ID
 */
void
Core::block_preschedule()
{
    assert(m_graph.chain_graph.size() > 0 && "\nERROR: Before scheduling you should load the graph");

    uint32_t i = 0;
    for(auto& bl : m_graph.block_chain){

        auto index = (i++)%m_lanes.size();

        for(auto& ch : bl.chains){

            //Find chain object
            auto ch_it = std::find_if(input_chain.begin(), input_chain.end(),
                    [&ch](Chain const& item) -> bool
                    {
                    return item.id == ch;
                    });
            //Check wether scheduling is correct
            assert(ch_it != input_chain.end() && "\nERROR: Wrong scheduling!\n");

            lane_pre_schedule[index].push_back(ch);

            //Decrease size of free slots
            lane_pre_schedule_size[index] += ch_it->instruction_list.size();
        }



    internal_com = 0;
    external_com = 0;
    //Find internal and external communications
    for(auto& dep : m_graph.dependencies){
        uint32_t source = dep.source;
        uint32_t target = dep.target;
        uint32_t l_source;
        uint32_t l_target;
        uint32_t i = 0;
        for(auto& lane: lane_pre_schedule){
            auto it = std::find_if(lane.begin(), lane.end(),
                    [&source] (uint32_t const& item) -> bool
                    {
                        return item == source;
                    });
            if(it != lane.end())
                l_source = i;

            it = std::find_if(lane.begin(), lane.end(),
                    [&target] (uint32_t const& item) -> bool
                    {
                        return item == target;
                    });
            if(it != lane.end())
                l_target = i;

            if(l_source == l_target)
                internal_com++;
            else
                external_com++;

            i++;

        }
    }

    }

    //Print final scheduling
    uint32_t k =0;
    for(auto& lane: lane_pre_schedule){
        cout << "Lane[" << k++ << "]: \n";
        for(auto& c : lane)
            cout << c << endl;

    }

}

void
Core::memoryInitialize(){

    //Load memory map file
    std::ifstream ldstfile(mem_map_path.c_str(), std::ios_base::in | std::ios_base::binary);
    assert(ldstfile.is_open() && "COULDNT OPEN MEMORY MAPP FILE");
    //std::cerr<<"ldst: "<<mem_map_path.c_str()<<std::endl;
    read_memcsv(ldstfile);
    
    m_memory.initialize();
}

void
Core::finishCore()
{
    uint32_t total_mem_stall = 0;
    uint32_t total_core_stall = 0;
    for(auto& l : m_lanes){
        total_mem_stall += l.m_memory_stall;
        total_core_stall += l.m_stall;
    }
    cout << "TOTAL MEM  STALL: \t" << total_mem_stall  << endl;
    cout << "TOTAL CORE STALL: \t" << total_core_stall << endl;
    cout << "TOTAL INTERNAL COMM: \t" << internal_com << endl;
    cout << "TOTAL EXTERNAL COMM: \t" << external_com << endl;
    cout << "TOTAL CORE FREE    : \t" << core_free    << endl;
    cout << "TOTAL CORE IDLE    : \t" << core_idle    << endl;


}
