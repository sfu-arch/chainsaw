from cli import *
from cli_impl import new_object_name
from tracker import register_tracker

class_name = "ppc64-linux-process-tracker"

def ppc64_ptrack_settings_ifc():
    return SIM_get_class_interface(
        SIM_get_class(class_name), "tracker_settings")

def ppc64_linux_kernel_expander(string):
    return get_completions(
        string, ppc64_ptrack_settings_ifc().settings_id_list())

def new_ppc64_linux_process_tracker_cmd(name, task_struct_offset,
                                        pid_offset, kernel):

    # Input validation.
    if (task_struct_offset == None) != (pid_offset == None):
        print ("You must specify none or both of task_struct_offset"
               + " and pid_offset.")
        return
    if task_struct_offset == None and kernel == None:
        print ("You must specify either kernel, or task_struct_offset"
               + " and pid_offset.")
        return
    if task_struct_offset != None and kernel != None:
        print ("You cannot specify both kernel and"
               + " task_struct_offset/pid_offset.")
        return

    # Choose a name.
    real_name = new_object_name(name, "tracker")
    if real_name == None:
        print "An object called '%s' already exists." % name
        return
    else:
        name = real_name

    # Using canned parameters?
    if kernel:
        if kernel in ppc64_ptrack_settings_ifc().settings_id_list():
            print ("Using parameters suitable for %s."
                   % ppc64_ptrack_settings_ifc().describe_settings(kernel))
        else:
            print "There is no named set of parameters called '%s'." % kernel
            return

    # Finally, create the tracker.
    tracker = SIM_create_object(class_name, name, [])
    if kernel:
        ppc64_ptrack_settings_ifc().use_settings(tracker, kernel)
    else:
        tracker.task_struct_offset_in_paca = task_struct_offset
        tracker.pid_offset_in_task_struct = pid_offset
    print "New process tracker %s created." % name
    print "    task_struct_offset %d" % tracker.task_struct_offset_in_paca
    print "    pid_offset         %d" % tracker.pid_offset_in_task_struct

new_command("new-" + class_name,
            new_ppc64_linux_process_tracker_cmd,
            [arg(str_t, "name", "?", None),
             arg(int_t, "task_struct_offset", "?", None),
             arg(int_t, "pid_offset", "?", None),
             arg(str_t, "kernel", "?", None,
                 expander = ppc64_linux_kernel_expander)],
            type = "Debugging",
            see_also = ["new-context-switcher", "<tracker>.add-processor"],
            short = "create new ppc64-linux process tracker",
            doc = """

Create a new ppc64-linux process tracker. You must either specify the
<i>task_struct_offset</i> and <i>pid_offset</i> integer parameters, or
choose a named set of parameters by giving the <i>kernel</i>
argument.

The tracker's set of processors will initially be empty. You can add
the processors you want it to watch with its add-processors
command.""")

register_tracker(class_name)
