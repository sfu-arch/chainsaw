from cli import *
import sim_commands

# increment command
def increment_value_cmd(obj):
    try:
        obj.value += 1
        print "The new counter value is:", obj.value
    except Exception, msg:
        print "Error incrementing counter:", msg

new_command("increment", increment_value_cmd, [],
            alias = "",
            type  = "empty-device commands",
            short = "increment value",
	    namespace = "empty-device",
            doc = """
Increments the value by adding 1 to it.
""")

#
# ------------------------ info -----------------------
#

def get_info(obj):
    # USER-TODO: Return something useful here
    return []

sim_commands.new_info_command('empty-device', get_info)

#
# ------------------------ status -----------------------
#

def get_status(obj):
    # USER-TODO: Return something useful here
    return [("Internals",
             [("Attribute 'value'", SIM_get_attribute(obj, "value"))])]

sim_commands.new_status_command('empty-device', get_status)
