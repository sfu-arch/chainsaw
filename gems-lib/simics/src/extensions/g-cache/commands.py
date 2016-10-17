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

def gc_info_cmd(gc):
    gc_default_info_cmd(gc)
    print "Read penalty          :", gc.penalty_read, "cycle" + iff(gc.penalty_read == 0 or gc.penalty_read == 1, "", "s")
    
    print "Read-next penalty     :", gc.penalty_read_next, "cycle" + iff(gc.penalty_read_next == 0 or gc.penalty_read_next == 1, "", "s")
    
    print "Write penalty         :",  gc.penalty_write, "cycle" + iff(gc.penalty_write == 0 or gc.penalty_write == 1, "", "s")
    
    print "Write-next penalty    :", gc.penalty_write_next, "cycle" + iff(gc.penalty_write_next == 0 or gc.penalty_write_next == 1, "", "s")

    print
    
    c_list = gc.snoopers
    if c_list:
        pr(   "Snoopers              : ")
        for c in c_list:
            pr(c.name + " ")
        pr("\n")
    
    c_list = gc.higher_level_caches
    if c_list:
        pr(   "Higher level caches   : ")
        for c in c_list:
            pr(c.name + " ")
        pr("\n")
        
def gc_stats_cmd(gc):
    gc_default_stats_cmd(gc, 1)

gc_define_cache_commands("g-cache", None, gc_info_cmd, gc_stats_cmd, None)
