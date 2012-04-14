/*
 * =====================================================================================
 *
 *       Filename:  gadget.cpp
 *
 *    Description:	Gadget discover
 *
 *        Version:  1.0
 *        Created:  04/14/2012 05:45:54 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Do Hoang Nhat Huy
 *        Company:
 *
 * =====================================================================================
 */

// System library
#include <err.h>
#include <gelf.h>
#include <fcntl.h>
#include <libelf.h>
#include <unistd.h>

// C standard library
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

// C++ standard library
#include <string>
#include <iostream>

// Arguments parsing
#include <argtable2.h>

// XED2
// #include <xed-interface.h>

// Arguments
struct arg_end *arg_des;
struct arg_str *arg_infile;
struct arg_str *arg_outfile;
struct arg_lit *arg_help;

//! Read the ELF file
void process(std::string const & input)
{
	// Init ELF
	if ( elf_version ( EV_CURRENT ) == EV_NONE ) {
		errx(EXIT_FAILURE, "ELF library initialization failed : %s", elf_errmsg(-1) );
	}

	// File descriptor of the ELF
	int fd;
	// Open ELF
	if ((fd = open(input.c_str(), O_RDONLY, 0)) < 0) {
    	err(EXIT_FAILURE, "Open \"%s\" failed", input.c_str());
	}

	// Elf handle
	Elf * e;
	// Start to read the ELF file using the above descriptor
	if ((e = elf_begin(fd, ELF_C_READ, NULL )) == NULL) {
		errx(EXIT_FAILURE, "Call elf_begin() failed : %s.", elf_errmsg(-1));
	}

	// Check the type of the ELF
	if (elf_kind(e) != ELF_K_ELF) {
		errx(EXIT_FAILURE, "File \"%s\" is not an ELF object.", input.c_str());
	}

	// The section index of the ELF section containing the section names
	size_t shstrndx;
	// Get the section index
	if (elf_getshstrndx(e, &shstrndx) != 0) {
		errx(EXIT_FAILURE, "Call elf_getshdrstrndx () failed : %s.", elf_errmsg(-1));
	}

	// Text section
	Elf_Scn * text = NULL;
	GElf_Shdr text_hdr;
	// Dynamic symbol section
	Elf_Scn * dyn  = NULL;
	GElf_Shdr dyn_hdr;
	// Dynamic symbol string section
	Elf_Scn * dyn_str  = NULL;
	GElf_Shdr dyn_str_hdr;

	// Pointer to Elf sections
	Elf_Scn * scn = NULL ;
	// List through all the sections till the end
	while ((scn = elf_nextscn(e, scn)) != NULL) {
		// An ELF section header
		GElf_Shdr shdr ;
		// Get the associated section header
		if (gelf_getshdr(scn, &shdr) != &shdr) {
			errx(EXIT_FAILURE, "Call getshdr() failed : %s.", elf_errmsg(-1));
		}

		char * name;
		// Get the name of the section from the section header
		if ((name = elf_strptr(e, shstrndx, shdr.sh_name)) == NULL) {
			errx(EXIT_FAILURE, "Call elf_strptr() failed : %s.", elf_errmsg(-1));
		}

		if ((shdr.sh_type == SHT_PROGBITS) && (0 == strcmp(name, ".text"))) {
			text = scn;
			text_hdr = shdr;
		}

		if ((shdr.sh_type == SHT_DYNSYM) && (0 == strcmp(name, ".dynsym"))) {
			dyn = scn;
			dyn_hdr = shdr;
		}

		if ((shdr.sh_type == SHT_STRTAB) && (0 == strcmp(name, ".dynstr"))) {
			dyn_str = scn;
			dyn_str_hdr = shdr;
		}
	}

	// No dynamic symbol section
	if (NULL == dyn) {
    	err(EXIT_FAILURE, "No .dynsym section");
	}

	Elf_Data * dyn_data;
	// Get the content of the dynamic symbol section
	if (NULL == (dyn_data = elf_getdata(dyn, dyn_data))) {
		errx(EXIT_FAILURE, "Call getdata() failed : %s.", elf_errmsg(-1));
	}

	Elf_Data * text_data;
	// Get the content of the text section
	if (NULL == (text_data = elf_getdata(text, text_data))) {
		errx(EXIT_FAILURE, "Call getdata() failed : %s.", elf_errmsg(-1));
	}

	// Index
	size_t i = 0;
	size_t s = 0;
	// Go though all dynamic symbol record
	while (true) {
		GElf_Sym sym_tmp;
		// Get the symbol record
		if (gelf_getsym(dyn_data, i, &sym_tmp) != &sym_tmp) {
			break;
		}

		size_t strndx = elf_ndxscn (dyn_str);

		char * name;
		// Get the name of the symbol
		if ((name = elf_strptr(e, strndx, sym_tmp.st_name)) == NULL) {
			errx(EXIT_FAILURE, "Call elf_strptr() failed : %s.", elf_errmsg(-1));
		}

		// Update the index
		i += 0x01;
	}

	printf("%08llx\n", text_hdr.sh_addr);
	// Index
	i = 0;
	s = 0;
	while (s < text_data->d_size) {
		printf("%08x\n", *((uint32_t *)(text_data->d_buf) + i));
		// Update the index
		i += 0x01;
		// Update the size
		s += sizeof(uint32_t);
	}

	// Done
	return ;
}

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:
 * =====================================================================================
 */
int main ( int argc, char *argv[] )
{
	// Just a name
	char static const * progname = "gadget";

	// Parse command line syntax
	void *argtable[] =	{
		/* Global arguments */
		// Input
		arg_infile	= arg_str1("i", "in"	,	"INPUT"	,	"Input file. Must be an executable ELF file"),
		// Output
		arg_outfile	= arg_str1("o", "out"	,	"OUTPUT",	"Output result"),

		// Help
		arg_help	= arg_lit0("h", "help", "Display this help and exit"),
		arg_des		= arg_end(32)
	};

	if (arg_nullcheck(argtable) != 0) {
        // NULL entries were detected, some allocations must have failed
        std::cerr << progname << ": insufficient memory" << std::endl;
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		return (EXIT_FAILURE);
    }

	int nerrors = arg_parse(argc, argv, argtable);

	// Special case: '--help' takes precedence over error reporting
    if (arg_help->count > 0) {
		std::cout << "Usage: " << progname;
		arg_print_syntax(stdout, argtable, "\n");
        arg_print_glossary(stdout, argtable,"  %-25s %s\n");
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		return (EXIT_SUCCESS);
	}

	// If the parser returned any errors then display them and exit
	if (nerrors > 0) {
        // Display the error details contained in the end struct.
        arg_print_errors(stdout, arg_des, progname);
        std::cerr << "Try '" << progname << " --help' for more information." << std::endl;
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		return (EXIT_FAILURE);
	}

	// Process the ELF file
	process(arg_infile->sval[0]);
	// Dump the result
	// dump(arg_outfile->sval[0]);

	return EXIT_SUCCESS;
}	/* ----------  end of function main  ---------- */

