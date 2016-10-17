#include "Chain.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <queue>
#include <map>
#include <set>


using namespace std;

//Returning instructions
vector<Node>
Chain::return_instruction()
{
    return this->instruction_list;
}


/**
 * Check whther the chain is free or not
 * @param out True if it's free
 */
bool
Chain::is_free()
{
    return (num_dependencies == 0);
}

/**
 * Reduce dependencies
 */
void
Chain::depen_increase() { num_dependencies++; }

/**
 * Increase dependencies
 */
void
Chain::depen_decrease()
{
    num_dependencies--;
}

/*
 * Add instruction to the list
 */
void
Chain::push_ins(Node m_node)
{
    instruction_list.push_back(m_node);
}

/**
 * Add live-in to the list
 */
void
Chain::push_liveIn(uint32_t m_temp)
{
    live_in.emplace(m_temp);
    num_dependencies++;
}

/**
 * Add live-out to the list
 */
void
Chain::push_liveOut(uint32_t m_temp)
{
    live_out.emplace(m_temp);
}

/**
 * Returning chain id
 */
uint32_t
Chain::return_id()
{
    return id;
}

