#include "../include/filesystem.h"
#include "../include/lib.h"
#include "../include/disk.h"

enum COLORS
{
	/* oscuros */
	BLACK,
	BLUE,
	GREEN,
	CYAN,
	RED,
	MAGENTA,
	BROWN,
	LIGHTGRAY,

	/* claros */
	DARKGRAY,
	LIGHTBLUE,
	LIGHTGREEN,
	LIGHTCYAN,
	LIGHTRED,
	LIGHTMAGENTA,
	YELLOW,
	WHITE
};

void
setNewDependency(file_sys_t *current_directory, char *sector_table, file_sys_t *dependency) {
	int i = 0;
	if (current_directory->dependencies_amount == DEPENDENCIES_SIZE) {
		enlargeDirectory(current_directory, sector_table, dependency);
	} else {
		for (i = 0; i < DEPENDENCIES_SIZE; i++) {
			if (current_directory->dependencies[i] == NULL) {
				current_directory->dependencies[i] = dependency;
				current_directory->dependencies_amount++;
				return;
			}
		}
	}
}

file_sys_t *
cd(file_sys_t *root, file_sys_t *current_directory, char *file_name) {

	if (strcmp(file_name, "..") == 0) {
		if (current_directory->father_directory != NULL) {
			return getFatherDirectory(current_directory);
		} else {
			return current_directory;
		}
	} else if (strcmp(file_name, "~") == 0) {
		return root;
	} else {
		int i;

		for (i = 0; i < current_directory->dependencies_amount; ++i) {
			if (strcmp(current_directory->dependencies[i]->file_name, file_name) == 0 && !(current_directory->dependencies[i]->is_file)) {
				//return restoreFileSystem(current_directory, current_directory->dependencies[i]->sector);
				return current_directory->dependencies[i];
			}
		}
		cprintk(LIGHTRED, BLACK, "No such directory\n");
		return current_directory;
	}
}

void
ls(file_sys_t *directory) {
	cprintk(WHITE, BLACK, "\tDependencies in %s: %d\n", directory->file_name, directory->dependencies_amount);
	if (directory->dependencies_amount == 0) {
		return;
	} else {
		int i;
		for (i = 0; i < DEPENDENCIES_SIZE ; i++) {
			if (directory->dependencies[i] != NULL) {
				if (directory->dependencies[i]->is_file) {
					cprintk(MAGENTA, BLACK, "\t\tfile:      ");
				} else {
					cprintk(LIGHTBLUE, BLACK, "\t\tdirectory: ");
				}
				cprintk(WHITE, BLACK, "%s\n", directory->dependencies[i]->file_name);
			}
		}
		if (directory->dependencies_amount > DEPENDENCIES_SIZE) {
			ls(directory->next);
		}
	}
	printk("\n");
	return;
}

bool
checkDisk(void) {
	int chk1, chk2;
	ata_read(ATA0, &chk1, sizeof(CHKDSK_1), CHKDSK_1_SECTOR, 0);
	ata_read(ATA0, &chk2, sizeof(CHKDSK_2), CHKDSK_2_SECTOR, 0);
	return chk1 == CHKDSK_1 && chk2 == CHKDSK_2;
}

file_sys_t*
initializeFileSystem(char *sector_table, int table_size) {
	int i;
	file_sys_t * root;

	for (i = 0; i < table_size; i++) {
		sector_table[i] = FREE;
	}
	root = mkdir(NULL, NULL, sector_table, "root");
	return root;
}

int
initializeSectorTableSize(char *sector_table) {
	char send[5] = "test";
	int sectors = 0;
	
	while(getErrorRegister(ATA0)) {
		ata_write(ATA0, send, sizeof(send), sectors, 0);
		sectors += SECTOR_STEP;
	}
	sector_table = malloc(sectors*sizeof(char));

	return sectors;	
}

bool
fileExists(file_sys_t *current_directory, char fileName[], bool isFile) {
	if (current_directory == NULL) {
		return FALSE;
	} else if (current_directory->dependencies_amount == 0) {
			return FALSE;
	} else {
		int i;
		for (i = 0; i < DEPENDENCIES_SIZE; i++) {
			if (current_directory->dependencies[i] != NULL) {
				if (strcmp(current_directory->dependencies[i]->file_name, fileName) == 0 && current_directory->dependencies[i]->is_file == isFile) {
					cprintk(LIGHTRED, BLACK, "ERROR: File already exists\n");
					return TRUE;
				}
			}
		}
		return FALSE;
	}
}

file_sys_t*
mkdir(file_sys_t *current_directory, file_sys_t *father_directory, char *sector_table, char file_name[]) {
	if (fileExists(current_directory, file_name, FALSE)) {
		return NULL;
	} else if (strlen(file_name) == 0) {
		cprintk(LIGHTRED, BLACK, "\tmkdir: Cannot create a directory with no name.\n\n");
		return NULL;
	} else {
		file_sys_t *ans = (file_sys_t *)malloc(sizeof(file_sys_t));
		serialized_file_t serialized_ans;

		strncpy(ans->file_name, file_name, FILE_NAME_SIZE);
		ans->is_file = FALSE;
		ans->father_directory = current_directory;
		ans->sector = getFreeDiscSector(sector_table);
		if (current_directory != NULL) {
			setNewDependency(current_directory, sector_table, ans);
		}
		cprintk(YELLOW, BLACK, "\tCreating directory %s...", file_name);
		serialized_ans = serializeFile(ans);
		cprintk(YELLOW, BLACK, "Writing in sector %d...", ans->sector);
		ata_write(ATA0, &serialized_ans, sizeof(serialized_file_t), ans->sector, 0);
		ata_write(ATA0, sector_table, 131072, TABLE_DISC_SECTOR, 0);
		cprintk(LIGHTGREEN, BLACK, "DONE\n");
		
		if (current_directory != NULL) { //ESTA PARTE DEL CODIGO ACTUALIZA AL DIRECTORIO ACTUAL...
			update(current_directory, sector_table);
		}
		return ans;
	}
}

int
getFreeDiscSector(char *sector_table) {
	int i;

	for (i = 0; i < FILE_TABLE_SIZE; i++) {
		if (sector_table[i] == FREE) {
			sector_table[i] = OCCUPIED;
			return i + FIRST_SECTORS_OCCUPIED;
		}
	}
	return -1;
	cprintk(LIGHTRED, BLACK, "FATAL: There's no free disk space left!\n");
}

file_sys_t*
getFatherDirectory(file_sys_t *current_directory) {
	if (current_directory->father_directory != NULL) {
		return current_directory->father_directory;
	} else {
		return current_directory;
	}
}

void
cat(file_sys_t *current_directory, char *file_name) {
	if (strlen(file_name) == 0) {
		cprintk(LIGHTRED, BLACK, "cat: File doesn't exist\n");
		return;
	} else {
		int i;
		for (i = 0; i < current_directory->dependencies_amount; ++i) {
			if (current_directory->dependencies[i] != NULL && current_directory->dependencies[i]->is_file) {
				if (strcmp(current_directory->dependencies[i]->file_name, file_name) == 0) {
					cprintk(WHITE, BLACK, "cat: %s\n", current_directory->dependencies[i]->content->content);
					return;
				}
			}
		}
		cprintk(LIGHTRED, BLACK, "cat: File doesn't exist\n");
	}
}

file_sys_t*
mkfile(file_sys_t *current_directory, file_sys_t *father_directory, char *sector_table, char *file_name, char *content) {
	if (strlen(file_name) == 0) {
		cprintk(LIGHTRED, BLACK, "rm: File doesn't exist\n");
		return NULL;
	} else if (fileExists(current_directory, file_name, TRUE)) {
		return NULL;
	} else {
		file_sys_t *ans = malloc(sizeof(file_sys_t));
		serialized_file_t serialized_ans;

		strcpy(ans->file_name, file_name);
		ans->is_file = TRUE;
		ans->father_directory = father_directory;
		ans->sector = getFreeDiscSector(sector_table);
		if (ans->sector < 0) {
			return NULL;
		}
		ans->content = enlargeFile(current_directory, sector_table, content, 0);
		setNewDependency(current_directory, sector_table, ans);

		cprintk(YELLOW, BLACK, "\tCreating file %s...", file_name);
		serialized_ans = serializeFile(ans);
		cprintk(YELLOW, BLACK, "Writing in sector %d...", ans->sector);
		ata_write(ATA0, &serialized_ans, sizeof(serialized_file_t), ans->sector, 0);
		ata_write(ATA0, sector_table, 131072, TABLE_DISC_SECTOR, 0);
		cprintk(LIGHTGREEN, BLACK, "DONE\n");

		//ESTA PARTE DEL CODIGO ACTUALIZA AL DIRECTORIO ACTUAL...
		update(current_directory, sector_table);

		return ans;
	}
}

void
update(file_sys_t *current_directory, char *sector_table) {
	serialized_file_t serialized_current_directory;

	serialized_current_directory = serializeFile(current_directory);
	cprintk(YELLOW, BLACK, "\tUpdating %s directory...", current_directory->file_name);
	ata_write(ATA0, &serialized_current_directory, sizeof(serialized_file_t), current_directory->sector, 0);
	cprintk(LIGHTGREEN, BLACK, "DONE\n");
	cprintk(YELLOW, BLACK, "\tUpdating sector table...");
	ata_write(ATA0, sector_table, SECTOR_TABLE_SIZE, TABLE_DISC_SECTOR, 0);
	cprintk(LIGHTGREEN, BLACK, "DONE\n");
}

file_t*
enlargeFile(file_sys_t *current_directory, char *sector_table, char *content, int multiplier) {
	file_t *enlargedFile = malloc(sizeof(file_t));
	enlargedFile->sector = getFreeDiscSector(sector_table);
	if (strlen(content) > 0) {
		strcpy(enlargedFile->content, content);
	}
	int strlength = strlen(content);
	if ((strlength - (multiplier*FILE_SIZE)) > 0) {
		enlargedFile->next = enlargeFile(current_directory, sector_table, content, multiplier+1);
	}
	return enlargedFile;
}

void
rm(file_sys_t *current_directory, char *sector_table, char *flag, char file_name[]) {
	bool isFile;
	if (strcmp(flag, "-f") == 0) {
		isFile = TRUE;
	} else if (strcmp(flag, "-d") == 0) {
		isFile = FALSE;
	} else {
		cprintk(LIGHTRED, BLACK, "rm: Flag value must be '-f' or '-d'\n");
		return;
	}
	if (current_directory->dependencies_amount == 0) {
		printRmMessageError(isFile);
		return;
	} else {
		int i;
		for (i = 0; i < DEPENDENCIES_SIZE; i++) {
			if (strcmp(current_directory->dependencies[i]->file_name, file_name) == 0 && (current_directory->dependencies[i]->is_file == isFile)) {
				sector_table[current_directory->dependencies[i]->sector - FIRST_SECTORS_OCCUPIED] = FREE;
				free(current_directory->dependencies[i]);
				removeDependencies(current_directory->dependencies[i]);
				current_directory->dependencies_amount--;
				printRmMessageSuccess(isFile, current_directory->dependencies[i]->file_name);
				current_directory->dependencies[i] = NULL;
			}
		}

		//ESTA PARTE DEL CODIGO ACTUALIZA AL DIRECTORIO ACTUAL Y LA TABLA DE SECTORES...
		update(current_directory, sector_table);
		return;
	}
	cprintk(LIGHTRED, BLACK, "rm: File doesn't exist\n");
}

void
printRmMessageError(bool isFile) {
	if (isFile) {
		cprintk(LIGHTRED, BLACK, "rm: File doesn't exist\n");
	} else {
		cprintk(LIGHTRED, BLACK, "rm: Directory doesn't exist\n");
	}
}

void
printRmMessageSuccess(bool isFile, char *file_name) {
	if (isFile) {
		cprintk(LIGHTGREEN, BLACK, "rm: File %s successfully removed\n", file_name);
	} else {
		cprintk(YELLOW, BLACK, "rm: Directory %s successfully removed\n", file_name);
	}
}

void
removeDependencies(file_sys_t *file) {
	if (file->is_file) {
		return;
	}
	if(file->dependencies_amount == 0) {
		free(file);
	} else {
		int i;
		for (i = 0; i < DEPENDENCIES_SIZE && file->dependencies[i] != NULL; i++) {
			printk("uuooooo - ");
			removeDependencies(file->dependencies[i]);
			file->dependencies[i] = NULL;
			file->dependencies_amount--;
		}
	}
}

serialized_file_t
serializeFile(file_sys_t *file) {
	serialized_file_t ans;
	int i = 0;

	strcpy(ans.name, file->file_name);
	ans.sector = file->sector;
	ans.is_file = file->is_file;
	if (!ans.is_file) {
		ans.dependencies_amount = file->dependencies_amount;
		for (i = 0; i < DEPENDENCIES_SIZE; i++) {
			if (file->dependencies[i] != NULL)
			{
				ans.dependencies[i] = file->dependencies[i]->sector;
			} else {
				ans.dependencies[i] = -1;
			}
		}
		if (file->dependencies_amount > DEPENDENCIES_SIZE) {
			ans.next_sector = file->next->sector;
			//serializeFile(file->next);
		} else {
			ans.next_sector = -1;
		}
	} else {
		strcpy(ans.content, file->content->content);
		if (file->content->next != NULL) {
			ans.next_sector = file->content->next->sector;
			//serializeFile(file->content->next);
		} else {
			ans.next_sector = -1;
		}
	}
	return ans;
}

file_sys_t *
deserializeFile(file_sys_t *father_directory, serialized_file_t file) {
	file_sys_t *ans;
	int i;

	ans = malloc(sizeof(file_sys_t));
	strcpy(ans->file_name, file.name);
	ans->is_file = file.is_file;
	ans->sector = file.sector;
	ans->father_directory = father_directory;
	if (!ans->is_file) {
		ans->dependencies_amount = file.dependencies_amount;
		for (i = 0; i < ans->dependencies_amount; ++i) {
			if (file.dependencies[i] != -1) {
				ans->dependencies[i] = restoreFileSystem(ans, file.dependencies[i]);
			} else {
				ans->dependencies[i] = NULL;
			}
		}
		if (ans->dependencies_amount > DEPENDENCIES_SIZE) {
			serialized_file_t next;
			ata_read(ATA0, &next, sizeof(serialized_file_t), file.next_sector, 0);
			ans->next = deserializeFile(father_directory, next);
		}
	} else {
		ans->content = malloc(sizeof(file_t));
		strcpy(ans->content->content, file.content);
		if (file.next_sector != -1) {
			serialized_file_t next;
			ata_read(ATA0, &next, sizeof(serialized_file_t), file.next_sector, 0);
			ans->content->next = (deserializeFile(father_directory, next))->content;
		}
	}
	return ans;
}

void
enlargeDirectory(file_sys_t *current_directory, char *sector_table, file_sys_t *dependency) {
	current_directory->next_sector = getFreeDiscSector(sector_table);
	file_sys_t *next = malloc(sizeof(file_sys_t));
	next->dependencies[0] = dependency;
	next->dependencies_amount = 1;
	current_directory->next = next;
}

file_sys_t*
restoreFileSystem(file_sys_t *father_directory, int sector) {
	file_sys_t *ans;
	serialized_file_t read;
	ata_read(ATA0, &read, sizeof(serialized_file_t), sector, 0);
	ans = deserializeFile(father_directory, read);
	return ans;
}
