/*
 *
 *       <xcoff-loader.c>
 *
 *       XCOFF file loader
 *
 *   Copyright (C) 2009 Laurent Vivier (Laurent@vivier.eu)
 *
 *   from original XCOFF loader by Steven Noonan <steven@uplinklabs.net>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   version 2
 *
 */

#include "openbios/config.h"
#include "openbios/bindings.h"
#include "modules.h"
#include "ofmem.h"

#include "openbios/xcoff.h"

//#define DEBUG_XCOFF

#ifdef DEBUG_XCOFF
#define DPRINTF(fmt, args...) \
    do { printk("%s: " fmt, __func__ , ##args); } while (0)
#else
#define DPRINTF(fmt, args...) \
    do { } while (0)
#endif

DECLARE_NODE(xcoff_loader, INSTALL_OPEN, 0, "+/packages/xcoff-loader" );

#ifdef CONFIG_PPC
extern void             flush_icache_range( char *start, char *stop );
#endif

static void
xcoff_loader_init_program( void *dummy )
{
	char *base;
	COFF_filehdr_t *fhdr;
	COFF_aouthdr_t *ahdr;
	COFF_scnhdr_t *shdr;
	uint32_t offset;
	int i;

	feval("0 state-valid !");

	feval("load-base");
	base = (char*)POP();

	fhdr = (COFF_filehdr_t*)base;

	/* Is it an XCOFF file ? */

	if (fhdr->f_magic != U802WRMAGIC &&
            fhdr->f_magic != U802ROMAGIC &&
	    fhdr->f_magic != U802TOCMAGIC &&
	    fhdr->f_magic != U802TOMAGIC) {
		DPRINTF("Not a XCOFF file %02x\n", fhdr->f_magic);
		return;
	}

	/* Is it executable ? */

	if (fhdr->f_magic != 0x01DF &&
	    (fhdr->f_flags & COFF_F_EXEC) == 0) {
		DPRINTF("Not an executable XCOFF file %02x\n", fhdr->f_flags);
		return;
	}

	/* Optional header is a.out ? */

	if (fhdr->f_opthdr != sizeof(COFF_aouthdr_t)) {
		DPRINTF("AOUT optional error size mismatch in XCOFF file\n");
		return;
	}

        ahdr = (COFF_aouthdr_t*)(base + sizeof(COFF_filehdr_t));

	/* check a.out magic number */

	if (ahdr->magic != AOUT_MAGIC) {
		DPRINTF("Invalid AOUT optional header\n");
		return;
	}

	offset = sizeof(COFF_filehdr_t) + sizeof(COFF_aouthdr_t);

	DPRINTF("XCOFF file with %d sections\n", fhdr->f_nscns);

	for (i = 0; i < fhdr->f_nscns; i++) {

		DPRINTF("Read header at offset %0x\n", offset);

		shdr = (COFF_scnhdr_t*)(base + offset);

		DPRINTF("Initializing '%s' section from %0x %0x to %0x (%0x)\n",
			shdr->s_name, offset, shdr->s_scnptr,
			shdr->s_vaddr, shdr->s_size);

		if (strcmp(shdr->s_name, ".text") == 0) {

			memcpy((char*)shdr->s_vaddr, base + shdr->s_scnptr,
			       shdr->s_size);
#ifdef CONFIG_PPC
			flush_icache_range((char*)shdr->s_vaddr,
					 (char*)(shdr->s_vaddr + shdr->s_size));
#endif
		} else if (strcmp(shdr->s_name, ".data") == 0) {

			memcpy((char*)shdr->s_vaddr, base + shdr->s_scnptr,
			       shdr->s_size);

		} else if (strcmp(shdr->s_name, ".bss") == 0) {

			memset((void *)shdr->s_vaddr, 0, shdr->s_size);

		} else {
			DPRINTF("    Skip '%s' section\n", shdr->s_name);
		}
		offset += sizeof(COFF_scnhdr_t);
	}

	/* FIXME: should initialize saved-program-state. */

	DPRINTF("XCOFF entry point: %x\n", *(uint32_t*)ahdr->entry);
	PUSH(*(uint32_t*)ahdr->entry);
	feval("xcoff-entry !");

	feval("-1 state-valid !");
}

NODE_METHODS( xcoff_loader ) = {
	{ "init-program", xcoff_loader_init_program },
};

void xcoff_loader_init( void )
{
	REGISTER_NODE( xcoff_loader );
}
