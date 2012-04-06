/*
 * =====================================================================================
 *
 *       Filename:  trace.h
 *
 *    Description:	Trace, copied from Hookfinder
 *
 *        Version:  1.0
 *        Created:  04/02/2012 05:07:26 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Do Hoang Nhat Huy
 *        Company:
 *
 * =====================================================================================
 */

#ifndef TRACING_INC
#define TRACING_INC

//! This structure is copied from Hookfinder.
typedef struct {
	int is_new;

	union {
		// What is this ?
    	struct {
			int is_move;
			uint32_t src_id[12];
			uint32_t dst_id[4];
		} prop;

		// and this ?
		struct {
			uint32_t dst_id[4];
		} define;
	} meme;

	// EIP
	uint32_t eip;
	// ESP
	uint32_t esp;

	// Address of the CALL instruction
	uint32_t caller;
	// and there it jumps to, i guess
	uint32_t callee;

	// Don't know
	uint32_t address_id;
	// Value of what ?
	uint32_t mem_val;
	// Address of what ?
	uint32_t mem_addr;

	// This it the raw INTEL instruction with the maximum of 16 bytes. We care about this.
	uint8_t raw_insn[16];
} trace_record_t;

//! This function reads the state of the machine and save it as a trace record
void prepare_trace_record(trace_record_t * trec);

//! Controller, start the trace if the user want it
void start_trace(char const * filename);
//! Stop the running trace
void stop_trace();

//! Dump the trace record to a file, Hookfinder provide a trace reader to read this file
void write_trace(trace_record_t * trec);

#endif

