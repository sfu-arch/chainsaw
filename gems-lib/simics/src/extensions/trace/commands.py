from cli import *

def trace_start_cmd(obj, file, raw, same):
       if not file and raw:
              print "Raw output can only be written to a file."
              return
       try:
              if same:
                     if file or raw:
                            print ("Other arguments are ignored"
                                   + " when -same is specified.")
              else:
                     if file:
                            obj.file = file
                     else:
                            obj.file = None
                     if raw:
                            obj.raw_mode = 1
                     else:
                            obj.raw_mode = 0
              obj.enabled = 1
              if not SIM_get_quiet():
                     print ("Tracing enabled. Writing %s output to %s."
                            % (["text", "raw"][obj.raw_mode],
                               ["'%s'" % obj.file,
                                "standard output"][obj.file == None]))
       except Exception, msg:
              print "Failed to start tracing."
              print "Error message: %s" % msg

new_command("start", trace_start_cmd,
            [arg(filename_t(), "file", "?", ""),
             arg(flag_t, "-raw"),
             arg(flag_t, "-same")],
            type = "base-trace-mem-hier commands",
            alias = "trace-start",
            see_also = ['new-tracer'],
            namespace = "base-trace-mem-hier",
            short = "control default tracer",
            doc = """
An installed tracer is required for this commands to work. Use the
command <b>new-tracer</b> to get a default tracer.

The <b>start</b> command turns on the tracer. The tracer logs all executed
instructions, memory accesses and exceptions. The output is written to
<i>file</i> (appended) if given, otherwise to standard output. If the
<tt>-raw</tt> flag is used, each trace entry is written in binary form; i.e., a
<tt>struct trace_entry</tt>. This structure can be found in trace_api.h. Raw
format can only be written to file.

If the <tt>-same</tt> flag is used, you will get the same output file
and mode as the last time.

<b>stop</b> switches off the tracer and closes the file. Until you
have given the <b>stop</b> command, you can not be sure that the
entire trace has been written to the file.""")

def trace_stop_cmd(obj):
       obj.enabled = 0
       if not SIM_get_quiet():
              print "Tracing disabled"

new_command("stop", trace_stop_cmd,
            [],
            type = "base-trace-mem-hier commands",
            alias = "trace-stop",
            namespace = "base-trace-mem-hier",
            short = "stop default tracer",
            doc_with = "<base-trace-mem-hier>.start")
