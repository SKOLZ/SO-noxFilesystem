#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#define FILE_NAME_SIZE				32
#define DEPENDENCIES_SIZE			58
#define FILE_TABLE_SIZE				1024
#define PATH_SIZE					32
#define TRUE						1
#define FALSE 						0
#define bool						int
#define FILE_SIZE 					116
#define INITIAL_DEPENDENCIES_AMOUNT	10
#define FILE_SYS_SIZE				FILE_NAME_SIZE*sizeof(char) + sizeof(bool) + 4*sizeof(int)
#define FILE_STRUCT_SIZE			FILE_NAME_SIZE*sizeof(char) + 3*sizeof(int) + FILE_SIZE
#define SERIALIZED_FILE_SIZE		FILE_NAME_SIZE*sizeof(char) + 2*sizeof(int) + FILE_SIZE + sizeof(bool)
#define SERIALIZED_DIRECTORY_SIZE	FILE_NAME_SIZE*sizeof(char) + DEPENDENCIES_SIZE*sizeof(int) + sizeof(int) + sizeof(bool)
#define SECTOR_STEP					8192	// 4 MB
#define FREE 						'0'
#define OCCUPIED					'1'
#define CHKDSK_1					1234	//	CHKDSK_1 y CHKDSK_2 sirven para leer del sector 0 y 1 del disco y comparar los resultados
#define CHKDSK_2					4321	//	obtenidos con dichos valores, y asi saber que nuestro sistema operativo escribio
#define CHKDSK_1_SECTOR				0		//	anteriormente sobre el disco. Si sabemos esto, podemos saber de donde leer la tabla de
#define CHKDSK_2_SECTOR				1 		//	sectores ocupados y libres del disco, y asi poder empezar a levantar los directorios y
#define TABLE_SIZE_SECTOR			3 		//	demas archivos...
#define	TABLE_DISC_SECTOR			4
#define FIRST_SECTORS_OCCUPIED		300
#define SECTOR_TABLE_SIZE			131072

typedef struct file_sys {
	char file_name[FILE_NAME_SIZE];
	bool is_file;
	struct file_sys *father_directory;
	int sector;
	struct file_sys *dependencies[DEPENDENCIES_SIZE];
	struct file_sys *next;
	int next_sector;
	int dependencies_amount;
	struct file *content;
}file_sys_t;

typedef struct file {
	char file_name[FILE_NAME_SIZE];
	int sector;
	int next_sector;
	char content[FILE_SIZE];
	struct file *next;
}file_t;

typedef struct serialized_file {
	char name[FILE_NAME_SIZE];
	int sector;
	bool is_file;
	char content[FILE_SIZE];
	int dependencies[DEPENDENCIES_SIZE];
	int dependencies_amount;
	int next_sector;
}serialized_file_t;

void setNewDependency(file_sys_t *current_directory, char *sector_table, file_sys_t *dependency);
void ls(file_sys_t *directory);
file_sys_t* initializeFileSystem(char *sector_table, int table_size);
file_sys_t* mkdir(file_sys_t *current_directory, file_sys_t *father_directory, char *sector_table, char file_name[]);
int getFreeDiscSector(char *sector_table);
int initializeSectorTableSize(char *sector_table);
file_sys_t* getFatherDirectory(file_sys_t *current_directory);
void rm(file_sys_t *current_directory, char *sector_table, char *flag, char fileName[]);
void removeDependencies(file_sys_t *file);
bool fileExists(file_sys_t *current_directory, char fileName[], bool isDir);
serialized_file_t serializeFile(file_sys_t *file);
bool checkDisk(void);
void enlargeDirectory(file_sys_t *current_directory, char *sector_table, file_sys_t *dependency);
file_sys_t* deserializeFile(file_sys_t *father_directory, serialized_file_t file);
file_sys_t* restoreFileSystem(file_sys_t *father_directory, int sector);
file_sys_t* cd(file_sys_t *root, file_sys_t *current_directory, char *file_name);
void cat(file_sys_t *current_directory, char *file_name);
file_t* enlargeFile(file_sys_t *current_directory, char *sector_table, char *content, int multiplier);
file_sys_t* mkfile(file_sys_t *current_directory, file_sys_t *father_directory, char *sector_table, char *file_name, char *content);
void update(file_sys_t *current_directory, char *sector_table);
void printRmMessageError(bool isFile);
void printRmMessageSuccess(bool isFile, char *file_name);

#endif