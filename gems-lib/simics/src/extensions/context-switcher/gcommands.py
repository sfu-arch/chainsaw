from cli import *
from cli_impl import new_object_name

def new_context_switcher_cmd(name, tracker):
    real_name = new_object_name(name, "switcher")
    if real_name == None:
        print "An object called '%s' already exists." % name
        SIM_command_has_problem()
        return
    else:
        name = real_name
    try:
        tracker = SIM_get_object(tracker)
    except:
        print "There is no object called '%s'." % tracker
        SIM_command_has_problem()
        return
    try:
        tracker.iface.tracker
    except:
        print "%s is not a tracker." % tracker.name
        SIM_command_has_problem()
        return
    SIM_create_object("context-switcher", name,
                      [["tracker", tracker]])
    if SIM_is_interactive():
        print "New context switcher %s created." % name
    return (name,)

new_command("new-context-switcher", new_context_switcher_cmd,
            [arg(str_t, "name", "?", None),
             arg(str_t, "tracker", expander = object_expander("tracker"))],
            type = "Debugging",
            short = "create new context switcher",
            doc = """

Create a new context switcher. It will change the current context of
the processors to match the currently active process. The
<i>tracker</i> parameter specifies the tracker object it should rely
on for process information.""")
