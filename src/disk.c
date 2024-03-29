#include "../include/disk.h"
#include "../include/kernel.h"

#define BIT(i) (1 << (i))

#define IS_REMOVABLE(D)    printk("%ses removible\n", (D & BIT(7)) ? "" : "No ")
#define IS_ATA_DRIVE(D)    printk("%ses ATA\n", (D & BIT(15)) ? "No " : "")
#define DMA_SUP(D)         printk("%ssoporta DMA\n", (D & BIT(8)) ? "" : "No ")
#define LBA_SUP(D)         printk("%ssoporta LBA\n", (D & BIT(9)) ? "" : "No ")
#define DMA_QUEUED_SUP(D)  printk("%ssoporta DMA QUEUED\n", (D & BIT(1)) ? "" : "No ")

void sendComm(int ata, int rdwr, unsigned short sector);
void _read(int ata, char * ans, unsigned short sector, int offset, int count);
void _write(int ata, char * msg, int bytes, unsigned short sector, int offset);

unsigned short getDataRegister(int ata);
void writeDataToRegister(int ata, char upper, char lower);
void translateBytes(char ans[], unsigned short sector);

void ata_normalize(unsigned short* sector, int* offset) {
	if (*offset >= SECTOR_SIZE) {
		*sector += (*offset / SECTOR_SIZE);
		*offset %= SECTOR_SIZE;
	}
}

void ata_read(int ata, void* ans, int bytes, unsigned short sector, int offset) {
	mt_cli();
	if (ata != ATA0 && ata != ATA1) {
		printk("No existe el disco %d - [%d, %d]\n", ata, sector, offset);
		return;
	}
	while (bytes != 0) {
		if (offset >= SECTOR_SIZE) {
			sector += (offset / SECTOR_SIZE);
			offset %= SECTOR_SIZE;
		}
		int size;
		if (offset + bytes > SECTOR_SIZE) { // read remaining part from the sector
			size = SECTOR_SIZE - offset;
			_read(ata, ans, sector, offset, size);
			//sleep(1);
			sector++;
			offset = 0;
			bytes -= size;
			ans += size;
		} else { // The remaining msg fits in the actual sector
			size = bytes;
			_read(ata, ans, sector, offset, size);
			//sleep(1);
			offset += size;
			bytes = 0;
			ans += size;
		}
	}
	mt_sti();
}

void _read(int ata, char * ans, unsigned short sector, int offset, int count) {
	char tmp[SECTOR_SIZE];
	sendComm(ata, LBA_READ, sector);
	/* Read sector*/
	int b;
	unsigned short data;
	for (b = 0; b < SECTOR_SIZE; b += 2) {
		data = getDataRegister(ata);
		translateBytes(tmp + b, data);
	}
	int i;
	for (i = 0; i < count; i++) {
		ans[i] = tmp[offset + i];
	}
}

void translateBytes(char * ans, unsigned short databyte) {
	ans[0] = databyte & 0xff;
	ans[1] = (databyte >> 8) & 0xFF;
}

void ata_write(int ata, void * msg, int bytes, unsigned short sector,
		int offset) {
	mt_cli();
	if (ata != ATA0 && ata != ATA1) {
		printk("El disco no existe %d - [%d, %d]\n", ata, sector, offset);
		return;
	}
	char* ans = (char*) msg;
	while (bytes != 0) {
		if (offset >= SECTOR_SIZE) {
			sector += (offset / SECTOR_SIZE);
			offset %= SECTOR_SIZE;
		}
		int size;
		if (offset + bytes > SECTOR_SIZE) { // Fill sector
			size = SECTOR_SIZE - offset;
			_write(ata, ans, size, sector, offset);
			wait_Ready();
			sector++;
			offset = 0;
			bytes -= size;
			ans += size;
		} else { // The remaining msg fits in the actual sector
			size = bytes;
			_write(ata, ans, size, sector, offset);
			wait_Ready();
			offset += size;
			bytes = 0;
			ans += size;
		}
	}
	mt_sti();
}

void _write(int ata, char * msg, int bytes, unsigned short sector, int offset) {
	char tmp[SECTOR_SIZE];
	int i;
	// Read actual sector because ATA always writes a complete sector!
	// Don't overwrite previous values!
	_read(ata, tmp, sector, 0, SECTOR_SIZE);
	for (i = 0; i < bytes; i++) {
		tmp[offset + i] = msg[i];
	}
	sendComm(ata, LBA_WRITE, sector);
	// Write updated sector
	for (i = 0; i < SECTOR_SIZE; i += 2) {
		writeDataToRegister(ata, tmp[i + 1], tmp[i]);
	}
}

void writeDataToRegister(int ata, char upper, char lower) {
	mt_cli();
	unsigned short out;
	// Wait for driver's ready signal.
	while (!(inw(ata + WIN_REG7) & BIT(3)))
		;
	out = upper << 8 | (lower & 0xff);
	outw(ata + WIN_REG0, out);
	mt_sti();
}

unsigned short getDataRegister(int ata) {
	mt_cli();
	unsigned short ans;
	// Wait for driver's ready signal.
	while (!(inw(ata + WIN_REG7) & BIT(3)))
		;
	ans = inw(ata + WIN_REG0);
	mt_sti();
	return ans;
}

unsigned short getErrorRegister(int ata) {
	unsigned short rta = inb(ata + WIN_REG1) & 0x00000FFFF;
	return rta;
}

void sendComm(int ata, int rdwr, unsigned short sector) {
	outb(ata + WIN_REG1, 0);
	outb(ata + WIN_REG2, 1); // Set count register sector in 1

	outb(ata + WIN_REG3, (unsigned char) sector); // LBA low
	outb(ata + WIN_REG4, (unsigned char) (sector >> 8)); // LBA mid
	outb(ata + WIN_REG5, (unsigned char) (sector >> 16)); // LBA high
	outb(ata + WIN_REG6, 0xE0 | (ata << 4) | ((sector >> 24) & 0x0F)); // Set LBA bit in 1 and the rest in 0
	// Set command
	outb(ata + WIN_REG7, rdwr);
}

unsigned short ata_getStatusRegister(int ata) {
	unsigned short rta;
	rta = inb(ata + WIN_REG7) & 0x00000FFFF;
	return rta;
}

void identifyDevice(int ata) {
	outb(ata + WIN_REG6, 0);
	outb(ata + WIN_REG7, WIN_IDENTIFY);
}

/*Checks disk features*/
void ata_checkDrive(int ata) {
	printk("Identificando dispositivo...\n ");
	switch (ata) {
	case ATA0:
		printk("ATA0...\n");
		break;
	case ATA1:
		printk("ATA1...\n");
		break;
	}
	printk("\n");
	identifyDevice(ata);
	unsigned short data = 0;
	int i;
	for (i = 0; i < 255; i++) {
		data = getDataRegister(ata);
		switch (i) {
		case 0:
			IS_REMOVABLE(data);
			IS_ATA_DRIVE(data);
			break;
		case 49:
			DMA_SUP(data);
			LBA_SUP(data);
			break;
		case 83:
			DMA_QUEUED_SUP(data);
			break;
		}
	}
}
