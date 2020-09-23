```txt
tracing mini-HOWTO:

# echo 0 > tracing_on : quick way to disable tracing
# echo 1 > tracing_on : quick way to re-enable tracing

 Important files:
  trace			- The static contents of the buffer
			  To clear the buffer write into this file: echo > trace
  trace_pipe		- A consuming read to see the contents of the buffer
  current_tracer	- function and latency tracers
  available_tracers	- list of configured tracers for current_tracer
  error_log	- error log for failed commands (that support it)
  buffer_size_kb	- view and modify size of per cpu buffer
  buffer_total_size_kb  - view total size of all cpu buffers

  trace_clock		-change the clock used to order events
       local:   Per cpu clock but may not be synced across CPUs
      global:   Synced across CPUs but slows tracing down.
     counter:   Not a clock, but just an increment
      uptime:   Jiffy counter from time of boot
        perf:   Same clock that perf events use
     x86-tsc:   TSC cycle counter

  timestamp_mode	-view the mode used to timestamp events
       delta:   Delta difference against a buffer-wide timestamp
    absolute:   Absolute (standalone) timestamp

  trace_marker		- Writes into this file writes into the kernel buffer

  trace_marker_raw		- Writes into this file writes binary data into the kernel buffer
  tracing_cpumask	- Limit which CPUs to trace
  instances		- Make sub-buffers with: mkdir instances/foo
			  Remove sub-buffer with rmdir
  trace_options		- Set format or modify how tracing happens
			  Disable an option by prefixing 'no' to the
			  option name
  saved_cmdlines_size	- echo command number in here to store comm-pid list

  available_filter_functions - list of functions that can be filtered on
  set_ftrace_filter	- echo function name in here to only trace these
			  functions
	     accepts: func_full_name or glob-matching-pattern
	     modules: Can select a group via module
	      Format: :mod:<module-name>
	     example: echo :mod:ext3 > set_ftrace_filter
	    triggers: a command to perform when function is hit
	      Format: <function>:<trigger>[:count]
	     trigger: traceon, traceoff
		      enable_event:<system>:<event>
		      disable_event:<system>:<event>
		      stacktrace
		      snapshot
		      dump
		      cpudump
	     example: echo do_fault:traceoff > set_ftrace_filter
	              echo do_trap:traceoff:3 > set_ftrace_filter
	     The first one will disable tracing every time do_fault is hit
	     The second will disable tracing at most 3 times when do_trap is hit
	       The first time do trap is hit and it disables tracing, the
	       counter will decrement to 2. If tracing is already disabled,
	       the counter will not decrement. It only decrements when the
	       trigger did work
	     To remove trigger without count:
	       echo '!<function>:<trigger> > set_ftrace_filter
	     To remove trigger with a count:
	       echo '!<function>:<trigger>:0 > set_ftrace_filter
  set_ftrace_notrace	- echo function name in here to never trace.
	    accepts: func_full_name, *func_end, func_begin*, *func_middle*
	    modules: Can select a group via module command :mod:
	    Does not accept triggers
  set_ftrace_pid	- Write pid(s) to only function trace those pids
		    (function)
  set_graph_function	- Trace the nested calls of a function (function_graph)
  set_graph_notrace	- Do not trace the nested calls of a function (function_graph)
  max_graph_depth	- Trace a limited depth of nested calls (0 is unlimited)

  snapshot		- Like 'trace' but shows the content of the static
			  snapshot buffer. Read the contents for more
			  information
  stack_trace		- Shows the max stack trace when active
  stack_max_size	- Shows current max stack size that was traced
			  Write into this file to reset the max size (trigger a
			  new trace)
  stack_trace_filter	- Like set_ftrace_filter but limits what stack_trace
			  traces
  dynamic_events		- Create/append/remove/show the generic dynamic events
			  Write into this file to define/undefine new trace events.
  kprobe_events		- Create/append/remove/show the kernel dynamic events
			  Write into this file to define/undefine new trace events.
  uprobe_events		- Create/append/remove/show the userspace dynamic events
			  Write into this file to define/undefine new trace events.
	  accepts: event-definitions (one definition per line)
	   Format: p[:[<group>/]<event>] <place> [<args>]
	           r[maxactive][:[<group>/]<event>] <place> [<args>]
	           s:[synthetic/]<event> <field> [<field>]
	           -:[<group>/]<event>
	    place: [<module>:]<symbol>[+<offset>]|<memaddr>
place (kretprobe): [<module>:]<symbol>[+<offset>]|<memaddr>
   place (uprobe): <path>:<offset>[(ref_ctr_offset)]
	     args: <name>=fetcharg[:type]
	 fetcharg: %<register>, @<address>, @<symbol>[+|-<offset>],
	           $stack<index>, $stack, $retval, $comm, $arg<N>,
	           +|-[u]<offset>(<fetcharg>), \imm-value, \"imm-string"
	     type: s8/16/32/64, u8/16/32/64, x8/16/32/64, string, symbol,
	           b<bit-width>@<bit-offset>/<container-size>, ustring,
	           <type>\[<array-size>\]
	    field: <stype> <name>;
	    stype: u8/u16/u32/u64, s8/s16/s32/s64, pid_t,
	           [unsigned] char/int/long
  events/		- Directory containing all trace event subsystems:
      enable		- Write 0/1 to enable/disable tracing of all events
  events/<system>/	- Directory containing all trace events for <system>:
      enable		- Write 0/1 to enable/disable tracing of all <system>
			  events
      filter		- If set, only events passing filter are traced
  events/<system>/<event>/	- Directory containing control files for
			  <event>:
      enable		- Write 0/1 to enable/disable tracing of <event>
      filter		- If set, only events passing filter are traced
      trigger		- If set, a command to perform when event is hit
	    Format: <trigger>[:count][if <filter>]
	   trigger: traceon, traceoff
	            enable_event:<system>:<event>
	            disable_event:<system>:<event>
	            enable_hist:<system>:<event>
	            disable_hist:<system>:<event>
		    stacktrace
		    snapshot
		    hist (see below)
	   example: echo traceoff > events/block/block_unplug/trigger
	            echo traceoff:3 > events/block/block_unplug/trigger
	            echo 'enable_event:kmem:kmalloc:3 if nr_rq > 1' > \
	                  events/block/block_unplug/trigger
	   The first disables tracing every time block_unplug is hit.
	   The second disables tracing the first 3 times block_unplug is hit.
	   The third enables the kmalloc event the first 3 times block_unplug
	     is hit and has value of greater than 1 for the 'nr_rq' event field.
	   Like function triggers, the counter is only decremented if it
	    enabled or disabled tracing.
	   To remove a trigger without a count:
	     echo '!<trigger> > <system>/<event>/trigger
	   To remove a trigger with a count:
	     echo '!<trigger>:0 > <system>/<event>/trigger
	   Filters can be ignored when removing a trigger.
      hist trigger	- If set, event hits are aggregated into a hash table
	    Format: hist:keys=<field1[,field2,...]>
	            [:values=<field1[,field2,...]>]
	            [:sort=<field1[,field2,...]>]
	            [:size=#entries]
	            [:pause][:continue][:clear]
	            [:name=histname1]
	            [:<handler>.<action>]
	            [if <filter>]

	    When a matching event is hit, an entry is added to a hash
	    table using the key(s) and value(s) named, and the value of a
	    sum called 'hitcount' is incremented.  Keys and values
	    correspond to fields in the event's format description.  Keys
	    can be any field, or the special string 'stacktrace'.
	    Compound keys consisting of up to two fields can be specified
	    by the 'keys' keyword.  Values must correspond to numeric
	    fields.  Sort keys consisting of up to two fields can be
	    specified using the 'sort' keyword.  The sort direction can
	    be modified by appending '.descending' or '.ascending' to a
	    sort field.  The 'size' parameter can be used to specify more
	    or fewer than the default 2048 entries for the hashtable size.
	    If a hist trigger is given a name using the 'name' parameter,
	    its histogram data will be shared with other triggers of the
	    same name, and trigger hits will update this common data.

	    Reading the 'hist' file for the event will dump the hash
	    table in its entirety to stdout.  If there are multiple hist
	    triggers attached to an event, there will be a table for each
	    trigger in the output.  The table displayed for a named
	    trigger will be the same as any other instance having the
	    same name.  The default format used to display a given field
	    can be modified by appending any of the following modifiers
	    to the field name, as applicable:

	            .hex        display a number as a hex value
	            .sym        display an address as a symbol
	            .sym-offset display an address as a symbol and offset
	            .execname   display a common_pid as a program name
	            .syscall    display a syscall id as a syscall name
	            .log2       display log2 value rather than raw number
	            .usecs      display a common_timestamp in microseconds

	    The 'pause' parameter can be used to pause an existing hist
	    trigger or to start a hist trigger but not log any events
	    until told to do so.  'continue' can be used to start or
	    restart a paused hist trigger.

	    The 'clear' parameter will clear the contents of a running
	    hist trigger and leave its current paused/active state
	    unchanged.

	    The enable_hist and disable_hist triggers can be used to
	    have one event conditionally start and stop another event's
	    already-attached hist trigger.  The syntax is analogous to
	    the enable_event and disable_event triggers.

	    Hist trigger handlers and actions are executed whenever a
	    a histogram entry is added or updated.  They take the form:

	        <handler>.<action>

	    The available handlers are:

	        onmatch(matching.event)  - invoke on addition or update
	        onmax(var)               - invoke if var exceeds current max
	        onchange(var)            - invoke action if var changes

	    The available actions are:

	        trace(<synthetic_event>,param list)  - generate synthetic event
	        save(field,...)                      - save current event fields
	        snapshot()                           - snapshot the trace buffer
```
