/*
 *
 */
#undef BOOTSTRAP
#include "openbios/config.h"
#include "openbios/bindings.h"
#include "openbios/elfload.h"
#include "openbios/nvram.h"
#include "libc/diskio.h"
#include "sys_info.h"

int elf_load(struct sys_info *, const char *filename, const char *cmdline);
int linux_load(struct sys_info *, const char *filename, const char *cmdline);

void boot(void);

struct sys_info sys_info;

void boot(void)
{
	char *path=pop_fstr_copy(), *param;

	// char *param="root=/dev/hda2 console=ttyS0,115200n8 console=tty0";

	if(!path) {
		printk("[x86] Booting default not supported.\n");
		return;
	}

	param = strchr(path, ' ');
	if(param) {
		*param = '\0';
		param++;
	}

	printk("[x86] Booting file '%s' with parameters '%s'\n",path, param);

	if (elf_load(&sys_info, path, param) == LOADER_NOT_SUPPORT)
		if (linux_load(&sys_info, path, param) == LOADER_NOT_SUPPORT)
			printk("Unsupported image format\n");

	free(path);
}
