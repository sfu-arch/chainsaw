#include "Lane.hpp"

using namespace std;
using namespace boost;


queue<int32_t> Lane::finished_chains;

queue<std::tuple<uint32_t, uint32_t>> Lane::mem_req_queue;

/**
 * Set Decode counter
 */
void
Lane::setDCounter(const uint32_t delay){ this->decode_cnt = delay;}

/**
 * Set Fetch counter
 */
void
Lane::setFCounter(const uint32_t delay){ this->fetch_cnt = delay;}

/**
 * Set Exe counter
 */
void
Lane::setExeCounter(const int32_t delay){ this->exe_cnt = delay;}

/**
 * Getting vector of instruction and load them into
 * instruction buffer
 * @output number of free spaces
 */
int
Lane::LoadInstructionQueue(vector<Node> input_chain, uint32_t ch_id)
{

    //Fill instruction queue
    for (auto& ins : input_chain){
        this->instruction_queue.emplace(ins);
    }

    //set current chain id
    m_current_ch = ch_id;

    //Set F/D counter
    assert(lane_param.param_set.find("DECODE_DELAY") != lane_param.param_set.end() &&
            "ERROR: Lanes' parameters have not been loaded");
    setDCounter(lane_param.param_set.find("DECODE_DELAY")->second);
    
    //updating number of scheduled chains counter
    schChainsList.push_back(ch_id);
    return (this->lane_size - this->instruction_queue.size());
}

/**
 * Simulating fetch stage
 * look at the fetch stage if it's free
 * fetch an instruction from instruction buffer
 * and set fetch counter
 */
void
Lane::fetch()
{

    //If fetch counter is set we should decrease it
    //otherwise we can fetch an instruction from
    //instruction buffer
    if(fetch_cnt > 0)
        return;

    //Fetch instruction of instruction buffer and push it
    //into pipline queue so that we can follow on going
    //instructions
    //And set fetch counter
    //
    //Check whether instruction queue is empty
    else{
        if(instruction_queue.empty())
            return;

        else{
            auto& ins_head = instruction_queue.front();
            fetch_stack.push(ins_head);
            instruction_queue.pop();
            assert(lane_param.param_set.find("FETCH_DELAY") != lane_param.param_set.end() &&
            "\nERROR: Lanes' parameters have not been loaded");
            setFCounter(4);
            //setFCounter(lane_param.param_set.find("FETCH_DELAY")->second);
        }
    }
}

/**
 * Simulating decode stage
 * look at the decode stage if it's free
 */
void
Lane::decode()
{
    //If decode counter is set we should decrease it
    //otherwise we can fetch an instruction from
    //instruction buffer
    if(decode_cnt > 0)
        return;
    else{
       
        //Check whether fetch queue is empty
        if(fetch_stack.empty())
            return;

        //Fetch instruction from fetch stack and put in decode stack
        else{
            auto& ins_head = fetch_stack.front();
            decode_stack.push(ins_head);
            fetch_stack.pop();
            assert(lane_param.param_set.find("DECODE_DELAY") != lane_param.param_set.end() &&
            "\nERROR: Lanes' parameters have not been loaded");
            setDCounter(lane_param.param_set.find("DECODE_DELAY")->second);
        }
    }
}

void
Lane::execute()
{
    //If exe counter is set we should decrease it
    //otherwise we can fetch an instruction from
    //instruction buffer
    if((exe_cnt == 0) && (!exe_stack.empty())){
        //Execution has been finished
        auto temp_it = exe_stack.front();
        exe_stack.pop();
        this->finished_chains.push(temp_it.c_id);
        m_current_op="";
        return;
    }

    else if(exe_cnt > 0){
        auto temp_ins = exe_stack.front();
        m_current_op = temp_ins.type;
        return;
    }
    else{

        //If exe counter is not grater than 0 there are two posibilities:
        //1) Exe queue is empty
        //2) We are stall for memory operations
        
        if(exe_cnt < 0){
            //The lane is stall because of memory operations
            //m_memory_stall++;
            return;
        }
        else{
            //Exe queue is empty we should fetch from decode queue
            //or instructions have been finished
            if(fetch_stack.empty())
                return;
            else if(fetch_cnt == 0){
                auto& temp_ins = this->fetch_stack.front();
                this->exe_stack.push(temp_ins);
                m_current_op = temp_ins.type;

                //Seting Exe counter base on type of instruction
                //
                //
                //
                //Check whether fected instruction is MEM_LD
                //if it's LD operation then we add a tuple having
                //core id and Addr and put it into mem_req queue
                // CASE 0: L1_MISS_LATENCY
                // CASE 1: L2_MISS_LATENCY
                // CASE 2: L3_MISS_LATENCY
                // CASE 3: Ruby system
                if(temp_ins.type == "Load"){
                    switch(m_memory_mode){
                        case 0:
                            assert(lane_param.param_set.find("L1_MISS_LATENCY") != lane_param.param_set.end() &&
                            "\nERROR: Lanes' parameters have not been loaded");
                            setExeCounter(lane_param.param_set.find("L1_MISS_LATENCY")->second);
                            break;
                        case 1:
                            assert(lane_param.param_set.find("L2_MISS_LATENCY") != lane_param.param_set.end() &&
                            "\nERROR: Lanes' parameters have not been loaded");
                            setExeCounter(lane_param.param_set.find("L2_MISS_LATENCY")->second);
                            break;
                        case 2:
                            assert(lane_param.param_set.find("L3_MISS_LATENCY") != lane_param.param_set.end() &&
                            "\nERROR: Lanes' parameters have not been loaded");
                            setExeCounter(lane_param.param_set.find("L3_MISS_LATENCY")->second);
                            break;
                        case 3:
                            this->mem_req_queue.push(std::make_tuple(this->laneID, temp_ins.old_id));
                            setExeCounter(-1);
                            break;
                    }
                }
                else if(temp_ins.type == "Store"){
                    switch(m_memory_mode){
                        case 0:
                            assert(lane_param.param_set.find("L1_MISS_LATENCY") != lane_param.param_set.end() &&
                            "\nERROR: Lanes' parameters have not been loaded");
                            setExeCounter(lane_param.param_set.find("L1_MISS_LATENCY")->second);
                            break;
                        case 1:
                            assert(lane_param.param_set.find("L2_MISS_LATENCY") != lane_param.param_set.end() &&
                            "\nERROR: Lanes' parameters have not been loaded");
                            setExeCounter(lane_param.param_set.find("L2_MISS_LATENCY")->second);
                            break;
                        case 2:
                            assert(lane_param.param_set.find("L3_MISS_LATENCY") != lane_param.param_set.end() &&
                            "\nERROR: Lanes' parameters have not been loaded");
                            setExeCounter(lane_param.param_set.find("L3_MISS_LATENCY")->second);
                            break;
                       case 3:
                            this->mem_req_queue.push(std::make_tuple(this->laneID, temp_ins.old_id));
                            setExeCounter(-1);
                            break;
                    }
                }
                else{
                    //FIXME FIND OUT ISSUE WITH NOP !!!!!
                    if(temp_ins.type == "Nop"){
                        setExeCounter(2);
                    }
                    setExeCounter(temp_ins.latency);
                }

                this->fetch_stack.pop();
 
            }
        }

    }
}

/**
 * Updating lane's counters
 */
void
Lane::updateCounter()
{
    //if(decode_cnt > 0)
        //decode_cnt--;

    if(fetch_cnt > 0)
        fetch_cnt--;

    if(exe_cnt > 0)
        exe_cnt--;
    else if (exe_cnt == -1){
        m_memory_stall++;
    }

}



/**
 * Run one cycle of the lanes
 */
void
Lane::run()
{
    updateCounter();
    execute();
    //decode();
    fetch();
}


/**
 * Print lane's status
 */
void
Lane::printLaneStatus()
{
    cout << "Lane: " << laneID << endl;
    cout << "\t Lane size: " << lane_size << endl;
    cout << "\t Fetch counter: " << fetch_cnt << endl;
    cout << "\t Exe counter: " << exe_cnt << endl;
    cout << "\t Memory Stall: " << m_memory_stall << endl;
    //auto ch_it = ins_name.find(static_cast<int32_t>(this->instruction_queue.front().type));
    //if(ch_it != ins_name.end())
        //cout << "\t Scheduled instructions: " << ch_it->second << endl;
}

/**
 * Check wheter the lane is free
 * free here means utilization of the lane
 */
bool
Lane::is_free()
{ 
    return ((fetch_cnt == 0) && (exe_cnt == 0) &&
            (instruction_queue.empty()) && (exe_stack.empty()));
}

