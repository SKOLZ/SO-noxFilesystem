#include "disk.h"

int diskwrite_main(int argc, char **argv) {

	char buffer[512] = "Test!\0";

	printk("Writing to disk.\n");
	ata_write(ATA0, buffer, 5, 2, 0);
	printk("Wrote: %s\n", buffer);

	return 0;
}