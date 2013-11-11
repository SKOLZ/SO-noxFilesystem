#include "kernel.h"
#include "apps.h"
#include "../include/filesystem.h"
#include "../include/disk.h"

#define BUFSIZE 200
#define NARGS 20

char *sector_table;
file_sys_t *root;
file_sys_t *current_directory;
int table_size;

static struct cmdentry
{
	char *name;
	int (*func)(int argc, char **argv);
}
cmdtab[] =
{
	{	"setkb",		setkb_main },
	{	"shell",		shell_main },
	{	"sfilo",		simple_phil_main },
	{	"filo",			phil_main },
	{	"xfilo",		extra_phil_main },
	{	"afilo",		atomic_phil_main },
	{	"camino",		camino_main },
	{	"camino_ns",	camino_ns_main },
	{	"prodcons",		prodcons_main },
	{	"divz",			divz_main },
	{ }
};

int
shell_main(int argc, char **argv)
{
	char buf[BUFSIZE];
	char *args[NARGS+1];
	unsigned nargs;
	struct cmdentry *cp;
	unsigned fg, bg;

	cprintk(WHITE, BLACK, "powered by ");
	cprintk(MAGENTA, BLACK, "nox");
	cprintk(WHITE, BLACK, "fileSystem\n\n");

	if (!checkDisk()) { //DEBERIA SER !CHEKDISK()

		int i, chkdsk1 = CHKDSK_1, chkdsk2 = CHKDSK_2;
		
		cprintk(YELLOW, BLACK, "Creating file system for the first time...\n");

		char buffer[512] = "";

		//table_size = initializeSectorTableSize(sector_table);
		table_size = SECTOR_TABLE_SIZE;		// HARDCODEO 64 MB EN SECTORES DE DISCO...

		cprintk(YELLOW, BLACK, "Table size: %d\n", table_size);

		sector_table = malloc(table_size*sizeof(char));
		for (i = 0; i < table_size; i++) {
			sector_table[i] = FREE;
		}

		ata_write(ATA0, &chkdsk1, sizeof(chkdsk1), CHKDSK_1_SECTOR, 0);
		ata_write(ATA0, &chkdsk2, sizeof(chkdsk2), CHKDSK_2_SECTOR, 0);
		ata_write(ATA0, &table_size, sizeof(table_size), TABLE_SIZE_SECTOR, 0);
		ata_write(ATA0, sector_table, sizeof(sector_table), TABLE_DISC_SECTOR, 0);
		root = initializeFileSystem(sector_table, table_size);
		current_directory = root;
	} else {
		cprintk(WHITE, BLACK, "Loading system resources:\n=========================\n");
		cprintk(YELLOW, BLACK, "Reading sector table size from disk...");
		ata_read(ATA0, &table_size, sizeof(table_size), TABLE_SIZE_SECTOR, 0);
		cprintk(LIGHTGREEN, BLACK, "DONE\n");
		cprintk(YELLOW, BLACK, "Reading sector table from disk...");
		ata_read(ATA0, sector_table, table_size, TABLE_DISC_SECTOR, 0);
		cprintk(LIGHTGREEN, BLACK, "DONE\n");
		root = restoreFileSystem(NULL, FIRST_SECTORS_OCCUPIED);
		current_directory = root;
	}

	mt_cons_getattr(&fg, &bg);
	while ( true )
	{
		mt_cons_setattr(LIGHTGRAY, BLACK);
		cprintk(LIGHTCYAN, BLACK, current_directory->file_name);
		cprintk(LIGHTCYAN, BLACK, "$>");

		/* leer linea de comando, fraccionarla en tokens y armar argv */
		mt_getline(buf, sizeof buf);
		nargs = separate(buf, args, NARGS);
		if ( !nargs )
			continue;
		args[nargs] = NULL;

		/* comandos internos */
		if ( strcmp(args[0], "help") == 0 )
		{
			printk("Comandos internos:\n");
			printk("\thelp\n");
			printk("\texit\n");
			printk("\treboot\n");
			printk("Aplicaciones:\n");\
			for ( cp = cmdtab ; cp->name ; cp++ )
				printk("\t%s\n", cp->name);
			continue;
		}

		if ( strcmp(args[0], "exit") == 0 )
		{
			mt_cons_setattr(fg, bg);
			return nargs > 1 ? atoi(args[1]) : 0;
		}

		if ( strcmp(args[0], "reboot") == 0 )
		{
			*(short *) 0x472 = 0x1234;
			while ( true )
				outb(0x64, 0xFE);
		}

		if ( strcmpLimits(args[0], "cd", 0, 1) == 0) {
			current_directory = cd(root, current_directory, args[1]);
			continue;
		} else if ( strcmpLimits(args[0], "mkdir", 0, 4) == 0) {
			mkdir(current_directory, getFatherDirectory(current_directory), sector_table, args[1]);
			continue;
		} else if ( strcmp(args[0], "ls") == 0) {
			ls(current_directory);
			continue;
		} else if ( strcmp(args[0], "rm") == 0) {
			rm(current_directory, sector_table, args[1], args[2]);
			continue;
		} else if ( strcmp(args[0], "mkfile") == 0) {
			if (args[2] == NULL)
			{
				args[2] = "";
			}
			mkfile(current_directory, getFatherDirectory(current_directory), sector_table, args[1], args[2]);
			continue;
		} else if ( strcmp(args[0], "cat") == 0) {
			cat(current_directory, args[1]);
			continue;
		}

		/* aplicaciones */
		bool found = false;
		for ( cp = cmdtab ; cp->name ; cp++ )
			if ( strcmp(args[0], cp->name) == 0 )
			{
				found = true;
				int n = cp->func(nargs, args);
				if ( n != 0 )
					cprintk(LIGHTRED, BLACK, "Status: %d\n", n);
				break;
			}

		if ( !found )
			cprintk(LIGHTRED, BLACK, "Comando %s desconocido\n", args[0]);
	}
}
