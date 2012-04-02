/*
 * =====================================================================================
 *
 *       Filename:  trace.c
 *
 *    Description:	Trace, copied from Hookfinder
 *
 *        Version:  1.0
 *        Created:  04/02/2012 05:07:52 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Do Hoang Nhat Huy
 *        Company:
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/user.h>
#include <sys/time.h>

extern FILE * tracelog = NULL;

void prepare_trace_record(trace_record_t * trec)
{
	bzero(trec, sizeof(trace_record_t));
	// What is this field ?
	trec->is_new = 0;
	// Get the value of EIP register
	TEMU_read_register(eip_reg, &(trec->eip));
	// Get the value of ESP register
	TEMU_read_register(esp_reg, &(trec->esp));

	// Fetch the raw instruction
	TEMU_read_mem(trec->eip, 16, trec->raw_insn);

	// Don't know much about this step, just keep it
	if (current_thread_node) {
		trec->caller = current_thread_node->eip;
		trec->callee = current_thread_node->entry_eip;

		// FIXME: this should be the esp on function entry
		trec->esp = current_thread_node->esp;
	}
}

void write_trace(trace_record_t *trec)
{
	// Just fump the binary record
	fwrite(trec, 1, sizeof(trace_record_t), tracelog);
}

void start_trace(char const * filename)
{
	if (tracelog) {
		// Self explainatory
		term_printf("tracing is already started!\n");
		return ;
	}

	tracelog = fopen(filename, "w");

	// Cannot open the trace log, report as error
	if (NULL == tracelog) {
		// Self explainatory
		term_printf("failed to open %s!\n", filename);
		return;
	}

	// Done
	term_printf("tracing is started successfully!\n");
	return;
}

void stop_trace()
{
	if (tracelog) {
		fclose(tracelog);
		// Reset the tracelog handle
		tracelog = NULL;
		// Done
		term_printf("tracing has been stopped!\n");
	}
}

