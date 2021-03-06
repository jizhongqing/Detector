/*
 * =====================================================================================
 *
 *       Filename:  detector.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  03/31/2012 07:02:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Do Hoang Nhat Huy
 *        Company:
 *
 * =====================================================================================
 */

// Standard header
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

// Temu headers
#include <TEMU_lib.h>
#include <slirp/slirp.h>
#include <shared/procmod.h>
#include <shared/hookapi.h>
#include <shared/read_linux.h>
#include <shared/reduce_taint.h>
#include <shared/hooks/reg_ids.h>
#include <shared/hooks/function_map.h>
#include <xed-interface.h>

// Local headers
#include "trace.h"
#include "detector.h"
#include "thread_info.hpp"

//! Some pre-defined value
enum location_t {
	// Not in the monitored process
	LOC_NOWHERE = 0,
	// Directly reside in the monitored module
	LOC_INSIDE,
	// In the code generate by the monitored process.
	// This may be useful (copy from Hookfinder)
	LOC_GEN,
	// In the function called by the monitored process.
	// This may be useful (copy from Hookfinder);
	LOC_FUNCTION
};

//! Name of the current module
char current_mod[512]	 = "";
//! Name of the current process
char current_proc[512]	 = "";
//! Name of the monitored process, the one we care about
char monitored_proc[512] = "";

//! Fudge off
int should_monitor = 0;
//! Counting
uint32_t insn_index = 0;

//! XED2 handle
// xed_state_t xed_state;

//! Handle to the queue log
FILE * loghandle		= NULL;
//! Handle to the instruction sequence file
FILE * inshandle		= NULL;

//! This is the main interface of a plugin
static plugin_interface_t interface;

//! Are we in the monitored process.  Check procmod.h
enum location_t in_checked_module = LOC_NOWHERE;
//! This is the starting address of the monitored module.  Check procmod.h
uint32_t checked_module_base = 0;
//! This is the size of the monitored module.  Check procmod.h
uint32_t checked_module_size = 0;

//! Multi-thread, get from Hookfinder
uint32_t current_thread_id = 0;
//! Multi-thread, get from Hookfinder
thread_info_t * current_thread_node = NULL;

//! Address of the next-to-last instruction, get from Hookfinder
uint32_t last_eip = 0;

//! ID of the tainted keystroke which is sent to the guest os
int taint_sendkey_id = 0;

//! Still don't know what it is
uint32_t depend_id = 1;

//! Handle to the trace file
FILE * tracelog = NULL;

//! Save the name of the requested module, prepare to monitor it
void do_start_check(char const * name)
{
	strcpy(monitored_proc, name);
	// Print the message to TEMU terminal
	term_printf("module to check: %s\n", name);
}

//! Stop monitor the current process
void do_stop_check(void)
{
	monitored_proc[ 0 ] = 0x00;
	// Print the message to TEMU terminal
	term_printf("detector is stopped!\n");
}

//! Get a keystroke from TEMU terminal
void do_taint_sendkey(char const * string, int id)
{
	taint_sendkey_id = id;
	// TEMU api
	do_send_key(string);
}

//! User commands
static term_cmd_t detector_term_cmds[] = {
	{"check_module"	, "s"	, do_start_check	, "procname"	, "specify the name of module to be tested"		},
	{"stop_check"	, ""	, do_stop_check		, ""			, "stop finding return-oriented rootkit"		},
	{"guest_ps"		, ""	, list_procs		, ""			, "list the processes on guest system"			},
	{"start_trace"	, "s"	, start_trace		, "filename"	, "start tracing"								},
	{"stop_trace"	, ""	, stop_trace		, ""			, "stop tracing"								},
	//We don't need this yet
	//{"taint_nic"	,	"i"	,	do_taint_nic	, "state"	, "set the network input to be tainted or not"	},
	{"taint_sendkey", "si"	, do_taint_sendkey	, "key & id"	, "send a tainted key to the guest system"		},
	// Terminator
	{NULL, NULL},
};

//! User manual
static term_cmd_t detector_info_cmds[] = {
	// Nothing, eh ?
	{NULL, NULL},
};

//! Print the trace record in human readable format
void print_trace_record()
{
	// Everything will be store here
	insn_record_t tmp;

	// Get the value of EIP register
	TEMU_read_register(eip_reg, &(tmp.eip));
	// Fetch the raw instruction
	TEMU_read_mem(tmp.eip, 16, tmp.raw_insn);

	/*
	// XED decoder handle
	xed_decoded_inst_t xdecode;

	xed_decoded_inst_zero_set_mode(&xdecode, &xed_state);
	// Convert to human-readable format using XED
	xed_error_enum_t xed_error = xed_decode(&xdecode, raw_insn, 16);
	// Correct instruction
	if (xed_error == XED_ERROR_NONE) {
		// Convert to intel format (or AT/T)
		// xed_format_att(&xdecode, tmp.asm_buf, sizeof(tmp.asm_buf), tmp.eip);
	}
	*/

	TEMU_read_register(esp_reg, &(tmp.esp));
	TEMU_read_register(ebp_reg, &(tmp.ebp));
	TEMU_read_register(esi_reg, &(tmp.esi));
	TEMU_read_register(edi_reg, &(tmp.edi));

	TEMU_read_register(eax_reg, &(tmp.eax));
	TEMU_read_register(ebx_reg, &(tmp.ebx));
	TEMU_read_register(ecx_reg, &(tmp.ecx));
	TEMU_read_register(edx_reg, &(tmp.edx));

	TEMU_read_register(sp_reg, &(tmp.sp));
	TEMU_read_register(bp_reg, &(tmp.bp));
	TEMU_read_register(si_reg, &(tmp.si));
	TEMU_read_register(di_reg, &(tmp.di));

	TEMU_read_register(ax_reg, &(tmp.ax));
	TEMU_read_register(bx_reg, &(tmp.bx));
	TEMU_read_register(cx_reg, &(tmp.cx));
	TEMU_read_register(dx_reg, &(tmp.dx));

	TEMU_read_register(al_reg, &(tmp.al));
	TEMU_read_register(bl_reg, &(tmp.bl));
	TEMU_read_register(cl_reg, &(tmp.cl));
	TEMU_read_register(dl_reg, &(tmp.dl));

	TEMU_read_register(ah_reg, &(tmp.ah));
	TEMU_read_register(bh_reg, &(tmp.bh));
	TEMU_read_register(ch_reg, &(tmp.ch));
	TEMU_read_register(dh_reg, &(tmp.dh));

	TEMU_read_mem(tmp.esp, 4, &(tmp.tos));

	// Dump the struct
	fwrite(&tmp, sizeof(insn_record_t), 0x01, inshandle);

	return ;
}

//! What will happen when TEMU send the keystroke to the guest os
void detector_send_keystroke(int reg)
{
	taint_record_t record;
	// A tainted keystroke
	if (taint_sendkey_id) {
		bzero( &record, sizeof(record) );
		record.depend_id = depend_id++;
		taintcheck_taint_register(reg, 0, 1, 1, (unsigned char *) &record);
		// Reset the tainted key
		taint_sendkey_id = 0;
	}
}

/*!
 * This is the main function which decide how to customize taint analysis.
 * The operand structure is defined in taintcheck.h.  This may be necessary
 * but we don't use it for now.
 */
void detector_taint_propagate(	int nr_src,
								taint_operand_t * src_oprnds,
								taint_operand_t * dst_oprnd,
								int mode	)
{
	taint_record_t *taint_record = NULL;
	// The trace will be stored nyaa
	trace_record_t trace_rec;
	// Which index are they ?
  	int src_index = 0, dst_index = 0;

	// Prepare the record if the trace is on
	if (tracelog) { prepare_trace_record( &trace_rec ); }

	// Which mode is this ?
	if (mode == PROP_MODE_MOVE) {
		// Use the prop union
		trace_rec.meme.prop.is_move = 1;

		if (nr_src == 1) {
			// There is only one operand
			for (int i = 0; i < src_oprnds[0].size; ++i) {
				// What the f**k is this ?
				if (src_oprnds[0].taint & (1 << i)) {
					taint_record_t * dst_rec = (taint_record_t *) dst_oprnd->records + i;
					taint_record_t * src_rec = (taint_record_t *) src_oprnds[0].records + i;
					memmove(dst_rec, src_rec, sizeof(taint_record_t));

					if (tracelog) {
            			trace_rec.meme.prop.src_id[ src_index++ ] = src_rec->depend_id;
					}

					dst_rec->depend_id = depend_id++;

					if (tracelog) {
						trace_rec.meme.prop.dst_id[ dst_index++ ] = dst_rec->depend_id;
					}
        		}
			}

			if (tracelog) {
				// Check the type of the operand
				if (dst_oprnd->type == OPERAND_MEM) {
					// Get the content of A0 register
					trace_rec.mem_addr = TEMU_cpu_regs[ R_A0 ];
					// Get the content of what register ?
					trace_rec.mem_val  = TEMU_cpu_regs[ src_oprnds[0].addr >> 2 ];

					taint_record_t tmp;
					if (taintcheck_register_check(R_A0, 0, 1, (uint8_t *) & tmp)) {
						trace_rec.address_id = tmp.depend_id;
					}
		        }
			}

			// Dump the trace
			if (tracelog) { write_trace(&trace_rec); }
			// and it's done
			return;
		}
	}

	// Deal with multiple sources
	if (tracelog) {
    	trace_rec.meme.prop.is_move = 0;
	}

	for (int i = 0; i < nr_src; ++i) {
		if (src_oprnds[ i ].taint == 0) { continue; }

		for (int j = 0; j < src_oprnds[ i ].size; ++j) {
      		if (src_oprnds[ i ].taint & (1 << j)) {
				taint_record_t * tmp = (taint_record_t *) src_oprnds[ i ].records + j;

				if (! taint_record) {
					taint_record = tmp;
					// Dump the record
					if (! tracelog) { goto _copy; }
        		}

				if (tracelog) {
					trace_rec.meme.prop.src_id[ src_index++ ] = tmp->depend_id;
				}
      		}
  		}
	}

	// There is no tainted record
	if (! taint_record) { return; }

_copy:

	for (int i = 0; i < dst_oprnd->size; ++i) {
		taint_record_t * dst_rec = (taint_record_t *) dst_oprnd->records + i;
		memmove(dst_rec, taint_record, sizeof(taint_record_t));

		dst_rec->depend_id = depend_id++;

		if (tracelog) {
			trace_rec.meme.prop.dst_id[ dst_index++ ] = dst_rec->depend_id;
		}
	}

	// Save the trace for later use
	if (tracelog) { write_trace(&trace_rec); }

	// Do nothing, just use the default policy
	// default_taint_propagate(nr_src, src_oprnds, dst_oprnd, mode);
}

/*!
 * Parse the message from guest system to extract OS-level semantics.
 * What the fuck does it actually do ?
 */
void detector_guest_message(char * message)
{
	switch (message[0]) {
		case 'P':
			// An internal function.  Don't know about it
			parse_process( message );
			break;
		case 'M':
			// Again
			parse_module( message );
			break;
		default:
			// Which message is it ?
			fprintf(loghandle, "Unknown message: %s\n", message);
			break;
	}
}

/*!
 * This is not clear how Temu define a block, however, both Hookfinder
 * & the sample plugin use this function to check if they are working
 * on the monitored process or not.
 */
int detector_block_begin()
{
	uint32_t eip, esp, cr3;
	// Get the value of EIP register
	TEMU_read_register(eip_reg, &eip);
	// Get the value of CR3 register
	TEMU_read_register(cr3_reg, &cr3);
	// Get the value of ESP register
	TEMU_read_register(esp_reg, &esp);

	// Get the current process using the above registers
	tmodinfo_t * mi = locate_module(eip, cr3, current_proc);

	// Fudge off
	should_monitor = (strcasecmp(current_proc, monitored_proc) == 0);

	// The module we care about
	if (should_monitor) {
		// We are inside the monitored module
		in_checked_module = LOC_INSIDE;
	} else {
		in_checked_module = LOC_NOWHERE;
	}

	//we should always check if there is a hook at this point,
	//no matter we are in the monitored context or not, because
	//some hooks are global.
	hookapi_check_call(should_monitor);

	// Done
	return 0;
}

//! This callback is invoked for every instruction
void detector_insn_begin()
{
	// if this is not the process we want to monitor, return immediately
	if (! should_monitor) { return; }

	// Now we can analyze this instruction.  In Hookfinder, they save the
	// address of the next to last instruction
	uint32_t static eip;
	// Save the previous instruction
	last_eip = eip;
	// and get the next one
	TEMU_read_register(eip_reg, &eip);

	// Print the trace record for debug
	if (inshandle) {
		print_trace_record();
	}
}

//! Cleanup
void detector_cleanup()
{
	// Clean up everything we create
	procmod_cleanup();
	hookapi_cleanup();
	function_map_cleanup();
	// What's about the XED structure

	// Close the log file
	fclose(loghandle);
	// Reload
	loghandle = NULL;

	// Close the instruction sequence
	fclose(inshandle);
	// Reload
	inshandle = NULL;
	// Reload
	insn_index = 0;
}

//! Standard callback function - Initialize the plugin interface
plugin_interface_t * init_plugin()
{
	// Name of the queue log
	char const * logfile	= "detector.log";
	// Name of the instruction sequence file
	char const * insfile	= "instructions.log";

	// Fail to create the plugin log
	if (!(loghandle = fopen(logfile, "w"))) {
		fprintf(stderr, "cannot create %s\n", logfile);
		return NULL;
	}

	// Fail to create the instruction log
	if (!(inshandle = fopen(insfile, "wb"))) {
		fprintf(stderr, "cannot create %s\n", insfile);
		return NULL;
	}

	// Init the function map, Don't really know its role
	// Check the definition in shared/hooks/function_map
	function_map_init();
	// Same as above, Don't know nothing about it
	// Check the definition in shared/hookapi
	init_hookapi();
	// Check the definition in shared/procmod
	procmod_init();
	// Check the definition in shared/reduce_taint.  There
	// is no documentation.  The f**k.
	reduce_taint_init();


	// XED2 is a X86 Encoder / Decoder.  My guess is that
	// TEMU use this tool to translate instruction from
	// assembly to machine language.
	// Check the following link for XED2 documentation
	// www.cs.virginia.edu/kim/publicity/pin/docs/24110/Xed/html/main.html
	// Init XED instruction table
	// xed_tables_init();
	// Zero-out the structure ?
	// xed_state_zero( &xed_state );
	// Update the XED2 structure with some pre-defined values
	// xed_state_init( &xed_state, XED_MACHINE_MODE_LEGACY_32, XED_ADDRESS_WIDTH_32b, XED_ADDRESS_WIDTH_32b);


	// The following portion of code registers all the
	// call-back functions.  Check the file TEMU_main.h
	// for the full list of these functions.
	// These functions are user-defined functions which
	// we will play with

	// Cleanup
	interface.plugin_cleanup	= detector_cleanup;
	// How the taint analysis works
	interface.taint_record_size	= sizeof(taint_record_t);
	interface.taint_propagate	= detector_taint_propagate;
	// Copy from sample plugin & hookfinder
	interface.guest_message		= detector_guest_message;
	// Beginning of a block, used to indentify process
	interface.block_begin		= detector_block_begin;
	// Beginning of an instruction
	interface.insn_begin		= detector_insn_begin;
	// These twos are for user-interface
	interface.term_cmds			= detector_term_cmds;
	interface.info_cmds			= detector_info_cmds;
	// Send a tainted keystroke
	interface.send_keystroke	= detector_send_keystroke;
	// What to do with network I/O, we don't need this yet
	// my_interface.nic_recv	= detector_nic_recv;
	// my_interface.nic_send	= detector_nic_send;
	// Need this to get the value of CR3 register
	interface.monitored_cr3		= 0;

	// We have TEMU interface now
	return & interface;
}


