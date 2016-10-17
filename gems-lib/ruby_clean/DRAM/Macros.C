#include <iostream>
#include <fstream>
#include <sstream>
#include <getopt.h>
#include <map>
#include <list>

#include "SystemConfiguration.h"
#include "MemorySystem.h"
#include "MultiChannelMemorySystem.h"
#include "Transaction.h"


using namespace DRAMSim;
using namespace std;

//#define RETURN_TRANSACTIONS 1


int SHOW_SIM_OUTPUT = 1;
ofstream visDataOut; //mostly used in MemoryController
