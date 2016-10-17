from cli import *
import sim_commands

#
# ------------------------ info -----------------------
#

def get_sample_info(obj):
    return []

sim_commands.new_info_command('sample-device', get_sample_info)

#
# ------------------------ status -----------------------
#

def get_sample_status(obj):
    return [(None,
             [("Attribute 'value'", obj.value)])]

sim_commands.new_status_command('sample-device', get_sample_status)

#
# ------------------------ add-log -----------------------
#

def add_log_cmd(obj, str):
    try:
        obj.add_log = str
    except Exception, msg:
        print "Error adding log string: %s" % msg


new_command("add-log", add_log_cmd,
            [arg(str_t, "log-string", "?", "default text")],
            alias = "",
            type  = "sample-device commands",
            short = "add a text line to the device log",
	    namespace = "sample-device",
            doc = """
Add a line of text to the device log. Use the 'io' command to view the log.<br/>
""")

#
# ---------------------- range-sum -----------------------
#

def sum_cmd(obj, start, end):
    try:
        sum = SIM_get_attribute_idx(obj, "range-sum", [start, end])
        print "The range sum [%d - %d] equals: %d" % (start, end, sum)
    except Exception, msg:
        print "Error calculating sum: %s" % msg


new_command("range-sum", sum_cmd,
            [arg(int_t, "start"), arg(int_t, "end")],
            alias = "",
            type  = "sample-device commands",
            short = "calculate range sum",
	    namespace = "sample-device",
            doc = """
Calculate the sum of all integers from 'start' to 'end'.
""")
