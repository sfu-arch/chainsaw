#
# commands.py
#
# Copyright (C) 2003-2004 Virtutech AB, All Rights Reserved
#
# This program is a component ("Component") of Virtutech Simics and is
# being distributed under Section 1(a)(iv) of the Virtutech Simics
# Software License Agreement (the "Agreement").  You should have
# received a copy of the Agreement with this Component; if not, please
# write to Virtutech AB, Norrtullsgatan 15, 1tr, SE-113 27 STOCKHOLM,
# Sweden for a copy of the Agreement prior to using this Component.
#
# By using this Component, you agree to be bound by all of the terms of
# the Agreement.  If you do not agree to the terms of the Agreement, you
# may not use, copy or otherwise access the Component or any derivatives
# thereof.  You may create and use derivative works of this Component
# pursuant to the terms the Agreement provided that any such derivative
# works may only be used in conjunction with and as a part of Virtutech
# Simics for use by an authorized licensee of Virtutech.
#
# THIS COMPONENT AND ANY DERIVATIVES THEREOF ARE PROVIDED ON AN "AS IS"
# BASIS.  VIRTUTECH MAKES NO WARRANTIES WITH RESPECT TO THE COMPONENT OR
# ANY DERIVATIVES THEREOF AND DISCLAIMS ALL IMPLIED WARRANTIES,
# INCLUDING, WITHOUT LIMITATION, WARRANTIES OF MERCHANTABILITY AND
# FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
#

from cli import *
from gcache_common import *

#
# info
#
def gc_info_cmd(gc):
    gc_default_info_cmd(gc)

    print "Read per cycle        :", gc.config_read_per_cycle, "transaction" + iff(gc.config_read_per_cycle == 0 or gc.config_read_per_cycle == 1, "", "s")
    
    print "Write per cycle       :", gc.config_write_per_cycle, "transaction" + iff(gc.config_write_per_cycle == 0 or gc.config_write_per_cycle == 1, "", "s")
    
    print "Maximum transactions  :", gc.config_max_out_trans

    print

    print "Read penalty          :", gc.penalty_read, "cycle" + iff(gc.penalty_read == 0 or gc.penalty_read == 1, "", "s")
    
    print "Write penalty         :",  gc.penalty_write, "cycle" + iff(gc.penalty_write == 0 or gc.penalty_write == 1, "", "s")
    
    print
    
#
# statistics
#
def gc_stats_cmd(gc):
    gc_default_stats_cmd(gc, 1)
    
#
# reset the cache lines
#
def gc_reset_cache_lines_cmd(gc):
    for i in range(gc.config_line_number):
        # reset the line's values
        gc.lines[i] = [0,0,0,0,0,0,0,0]
        
gc_define_cache_commands("g-cache-ooo",
                         None,
                         gc_info_cmd,
                         gc_stats_cmd,
                         gc_reset_cache_lines_cmd)


#
# splitter info command
#
def ts_info_cmd(ts):

    print
    print "Splitter Info:"
    print "--------------"
    print "Next cache line size  :", ts.next_cache_line_size
    pr(   "Connected timing model: ")
    if ts.timing_model:
        print ts.timing_model
    else:
        print "None"
    pr(   "Connected to cache    : ")
    if ts.cache:
        print ts.cache
    else:
        print "None"
    print
    
new_command("info", ts_info_cmd,
            [],
            namespace = "trans-splitter-ooo",
            type ="trans-splitter-ooo commands",
            short="print the splitter information",
            doc  = """
            Output the splitter information
            """);

#
# statistics
#
def ts_stats_cmd(ts):

    print
    print "Splitter statistics:"
    print "--------------------"
    print "Total number of transactions   :", ts.transactions
    print "Split transactions             :", ts.split_transactions
    if (ts.transactions > 0):
        print "Split ratio                    : %0.02f%%" % ((100.0 * ts.split_transactions)/ts.transactions)
    print
    
new_command("statistics", ts_stats_cmd,
            [],
            namespace = "trans-splitter-ooo",
            type ="trans-splitter-ooo commands",
            short="print the splitter statistics",
            doc  = """
            Output the splitter statistics
            """);

#
# reset statistics
#
def ts_reset_stats_cmd(ts):
    ts.transactions = 0
    ts.split_transactions = 0
    
new_command("reset-statistics", ts_reset_stats_cmd,
            [],
            namespace = "trans-splitter-ooo",
            type ="trans-splitter-ooo commands",
            short="reset the splitter statistics",
            doc  = """
            Reset all splitter statistics to 0.
            """);



#
# info command
#
def sort_info_cmd(ts):

    print
    print "Sorter Info:"
    print "------------"
    pr(   "Connected to cache   : ")
    if ts.cache:
        print ts.cache
    else:
        print "None"
    print

    print "  Cacheable memory transaction listener:", ts.cacheable_mem_timing_model
    print "  Cacheable device transaction listener:", ts.cacheable_dev_timing_model
    print "Uncacheable memory transaction listener:", ts.uncacheable_mem_timing_model
    print "Uncacheable device transaction listener:", ts.uncacheable_dev_timing_model
    print
    
new_command("info", sort_info_cmd,
            [],
            namespace = "trans-sorter",
            type ="trans-sorter commands",
            short="print the sorter information",
            doc  = """
            Output the sorter information
            """);

#
# statistics
#
def sort_stats_cmd(ts):

    print
    print "Sorter statistics:"
    print "------------------"
    print "  Cacheable memory transactions:", ts.cacheable_mem_transactions
    print "  Cacheable device transactions:", ts.cacheable_dev_transactions
    print "Uncacheable memory transactions:", ts.uncacheable_mem_transactions
    print "Uncacheable device transactions:", ts.uncacheable_dev_transactions
    print
    
new_command("statistics", sort_stats_cmd,
            [],
            namespace = "trans-sorter",
            type ="trans-sorter commands",
            short="print the sorter statistics",
            doc  = """
            Output the sorter statistics
            """);
