/* www.piotrbania.com/all/ti89/89z_viewer.cpp */
/*




	TI89 File Format (.89z) Viewing Utility
	---------------------------------------
	by Piotr Bania <bania.piotr@gmail.com>
	http://www.piotrbania.com


 	0.   DISCLAIMER
       

	Author takes no responsibility for any actions with provided informations or 
	codes. The copyright for any material created by the author is reserved. Any 
	duplication of codes or texts provided here in electronic or printed 
	publications is not permitted without the author's agreement. 


	1. ABOUT THIS FILE


	This file is able to view internal structures of the .89z executable file.
	It also can repair the checksum field, this is really important when you are
	modifing your binaries and then trying to resend them to the calc.
	If the checksum is wrong, calculator will refuse the file transfer.


	Usage:	89z_viewer.exe <file.89z>

	EOT.


*/








#include <stdio.h>
#include <conio.h>
#include <windows.h>



/* thanks for Patrick Pelissier for this and his PEDROM */
typedef struct
{
	char			signature[8];				// +0x0  | should be always: **TI89**
	char			further_signature[2];		// +0x8  | always: 0x01 0x00
	char			folder_name[8];				// +0xA  | default folder name (If the first byte of the default folder name is 0, then the default folder is the current folder on the receiving unit.)
	char			comment[40];				// +0x12 | comments 
	unsigned short	num_of_vars;				// +0x3A | number of variable and folder entries in the variable table.
} TI89_MAIN_VARIABLE;


typedef struct 
{
	unsigned long	offset2data;					// +0x0  | Offset to data
	char			var_name[8];					// +0x4	 | Name of variable
	char			type_id;						// +0xC  | Type ID of the variable. - if this is 1Fh this is a folder entry
	char			attribute;						// +0xD	 | Attribute (0: none, 1: locked, 2: archived)
	char			unused[2];						// +0xE	 | Unused bytes (both null)
} TI89_VARIABLE_ENTRY;

typedef struct 
{
	unsigned long	offset2data;					// +0x0  | Offset to data
	char			var_name[8];					// +0x4	 | Name of variable
	char			type_id;						// +0xC  | Type ID of the variable. - if this is 1Fh this is a folder entry
	char			unused;							// +0xD  | Null byte
	unsigned short	num_of_vars;					// +0xE	 | Num of variables in this folder
} TI89_FOLDER_ENTRY;

typedef struct
{
	unsigned long	file_size;						// +0x0	 | Filesize in bytes (from Signature to Checksum)
	char			signature[2];					// +0x4	 | Signature: always {A5h, 5Ah}
} TI89_DATA_SECTION;


typedef struct {
	unsigned short	crc;							// checksum
} TI89_CHECKSUM;


long filesize(FILE *stream)
{
   long curpos, length;

   curpos = ftell(stream);
   fseek(stream, 0L, SEEK_END);
   length = ftell(stream);
   fseek(stream, curpos, SEEK_SET);
   return length;
}

unsigned short get_checksum(unsigned char *ptr, int size)
{
	int i;
	unsigned char *s = ptr;
	unsigned short crc = 0;

	for (i = 0; i < size; s++,i++)
		crc += *s;

	return crc;
}


void show_hexa(unsigned char *ptr,int count)
{
	unsigned char *s = ptr;

	printf("\n ---------------------- HEX DUMP %d bytes -------------------- \n",count);
	for (int i = 0; i < count; i++)
	{
		printf("\t");
		for (int k = 0; k < 16; k++)
		{
			printf("%.02x ",*s);
			s++;
		}
		printf("\n");
	}
	printf("\n ---------------------- END OF HEX DUMP ------------------------\n");
}





int main(int argc, char *argv[])
{
	unsigned long f_size;
	unsigned char *data;

	unsigned char *var_data;
	unsigned long var_data_size;
	unsigned short crc32;

	TI89_MAIN_VARIABLE		*ti89_img;
	TI89_VARIABLE_ENTRY		*ti89_ve;
	TI89_DATA_SECTION		*ti89_datasection;
	TI89_CHECKSUM			*chk;


	FILE *rom = fopen(argv[1],"rb+");

	printf("Trying to open: \"%s\" \n",argv[1]);

	if (!rom)
	{
		printf("Failed to open file!\n");
		return -1;
	}

	f_size = filesize(rom);
	data = (unsigned char *)malloc(f_size+1);

	if (!data)
	{
		printf("Failed to allocate memory\n");
		fclose(rom);
		return -1;
	}


	memset((void*)data,0,f_size+1);
	fread((void*)data,1,f_size,rom);

	ti89_img = (TI89_MAIN_VARIABLE*)data;
	ti89_ve = (TI89_VARIABLE_ENTRY*)(data + sizeof(TI89_MAIN_VARIABLE));
	ti89_datasection = (TI89_DATA_SECTION*)(data + sizeof(TI89_MAIN_VARIABLE) + sizeof(TI89_VARIABLE_ENTRY));
	var_data = (unsigned char*)(data + ti89_ve->offset2data);
	chk = (TI89_CHECKSUM*)(data + ti89_datasection->file_size - sizeof(WORD));

	var_data_size = (unsigned long)chk - (unsigned long)var_data;
	crc32 = get_checksum((unsigned char*)(var_data),var_data_size);
	
	

	printf("File size: 0x%.08x (%d)\n",f_size,f_size);
	printf("Folder name: \"%s\" \n",ti89_img->folder_name);
	printf("Number of variables: %d\n",ti89_img->num_of_vars);
	printf("Variable name: \"%s\" \n",ti89_ve->var_name);
	printf("Variable entry type_id: 0x%.02x\n",ti89_ve->type_id);
	printf("Offset to data: 0x%.08x \n",ti89_ve->offset2data);
	printf("Variable attribs: 0x%x \n",ti89_ve->attribute);
	printf("File size is: 0x%x (%d) \n",ti89_datasection->file_size,ti89_datasection->file_size);
	printf("Var data size is: 0x%x (%d)\n",var_data_size,var_data_size);
	printf("Checksum is: 0x%.08x\n",chk->crc);

	if (chk->crc != crc32)
	{
		printf("Checksum was broken, repaired to: %.08x\n",crc32);
		chk->crc = crc32;
	}




	show_hexa((unsigned char*)var_data,26);
	fclose(rom);

	rom = fopen(argv[1],"wb+");
	fseek(rom,0,SEEK_SET);
	fwrite((void*)data,f_size,1,rom);
	fclose(rom);
	getch();


	free(data);
	
}