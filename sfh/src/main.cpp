#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <intrin.h>

#define ES32(v)((unsigned int)(((v & 0xFF000000) >> 24) | \
                               ((v & 0x00FF0000) >> 8 ) | \
							   ((v & 0x0000FF00) << 8 ) | \
							   ((v & 0x000000FF) << 24)))
							   

void vrlb(unsigned char *vD, unsigned char *vA, unsigned char *vB)
{
	int i;
	for (i = 0; i < 0x10; i++)
		vD[i] = _rotl8 (vA[i], vB[i]);
}

void vaddubm(unsigned char *vD, unsigned char *vA, unsigned char *vB)
{
	int i;
	for (i = 0; i < 0x10; i++)
		vD[i] = (vA[i] + vB[i]);
}

void makehash(unsigned char *hash, unsigned char *content, unsigned int size)
{
	
	unsigned char initializekey[0x10] = {0x87, 0x55, 0x07, 0xB5, 0x4B, 0x04, 0xA5, 0xAE, 0xC7, 0x67, 0xBE, 0xCB, 0x01, 0x50, 0x58, 0x44}; //initial hashkey
	unsigned char finalizekey[0x10]   = {0x07, 0x02, 0x05, 0x04, 0x06, 0x03, 0x01, 0x04, 0x02, 0x06, 0x07, 0x03, 0x05, 0x07, 0x01, 0x05}; //hashkey for vrlb
	unsigned char *init = (unsigned char *)malloc(0x10);
	unsigned char *update = (unsigned char *)malloc(0x10);
	unsigned char *finish = (unsigned char *)malloc(0x10);
	memset(init, 0, 0x10);
	memset(update, 0, 0x10);
	memset(finish, 0, 0x10);
	memcpy (init, initializekey, 0x10);
	memcpy (finish, finalizekey, 0x10);
	
	unsigned int i = 0;
	memcpy (update, init, 0x10);
	for (i = 0; i < (size / 0x10); i++)
	{
		vaddubm(update, update, (content + (i*0x10)));
	}
	
	vrlb(hash,update,finish);
}



typedef struct 
{
	unsigned char magic[4];
	unsigned char flags[4];
	unsigned int  encapsulated_data_size;
	unsigned char padding[4];
} SFH_HEADER;

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("sfh v.0.0.1.\n");
		printf("Usage: sfh.exe <file_in> [file_out]\n\n");
		return 0;
	}

	FILE *in = NULL;
	FILE *out = NULL;
	bool IsLastBlock = false;
	unsigned long long in_size;
	unsigned int max_chunk_size = 0x1FFF0;
	unsigned int chunk_size = 0;
	unsigned int chunk_num = 0;
	char out_fname[260]; //max path is 260 for MS Windows.
	char* output_filename;
	char* input_filename = argv[1];

	//add ".tmp" to out file name.
	_snprintf(out_fname, sizeof(out_fname), "%s.tmp", argv[1]);

	in = fopen(input_filename, "rb");

	if (in == NULL)
	{
		printf("Error! Could not open file %s.\n", argv[1]);
		return 0;
	}
	else
		printf("File %s loaded.\n", argv[1]);

	//Obtain size of the input file.
	_fseeki64(in, 0, SEEK_END);
	in_size = _ftelli64(in);
	_fseeki64(in, 0, SEEK_SET);


	//Check for size.
	
	

	if (in_size > 0xFFFFFFFF)
	{
		printf("Error! Invalid input file detected, exiting.\n");
		fclose(in);
		return 0;
	}
	printf("File size : 0x%X bytes\n", in_size);
	
	SFH_HEADER *SFH = new SFH_HEADER();
	
	
	// Forge SFH header.
	unsigned char sfh_magic[4] = {0x00, 0x53, 0x46, 0x48};  //.SFH
	memcpy(SFH->magic, sfh_magic, 4);
	unsigned char sfh_flags[4] = {0x00, 0x01, 0x00, 0x01};  // 0x00010001
	memcpy(SFH->flags, sfh_flags, 4);
	unsigned int encapsulated_data_size_be = ES32(in_size);
	SFH->encapsulated_data_size = encapsulated_data_size_be;
	unsigned char sfh_padding[4] = {0x00, 0x00, 0x00, 0x00};// 0x00000000
	memcpy(SFH->padding, sfh_padding, 4);
	
	
	//writing header:
	remove(out_fname);
	out = fopen(out_fname, "ab");
	
	fwrite(SFH->magic, sizeof(SFH->magic), 1, out);
	fwrite(SFH->flags, sizeof(SFH->flags), 1, out);
	fwrite(&SFH->encapsulated_data_size, sizeof(SFH->encapsulated_data_size), 1, out);
	fwrite(SFH->padding, sizeof(SFH->padding), 1, out);

	//writing data:
	unsigned int data_size = (unsigned int)in_size;
	unsigned int pad_size = (0x10 -(data_size % 0x10)) % 0x10;
	unsigned int bytes_to_read = (unsigned int)in_size;
	
	unsigned char *hash = (unsigned char *)malloc(0x10);
	memset(hash, 0, 0x10);

	_fseeki64(in, 0, SEEK_SET);
	
	printf("Pad size : 0x%X bytes\n", pad_size);
	
	
	while (IsLastBlock == false)
	{
		if (bytes_to_read <= max_chunk_size)
		{
			chunk_size = data_size % 0x1FFF0;
		
			IsLastBlock = true;
		}
		else
			chunk_size = max_chunk_size;

		bytes_to_read = bytes_to_read - chunk_size;
		
		if (IsLastBlock == true)
			chunk_size = chunk_size + pad_size;
		
		unsigned char *chunk_buf = (unsigned char *)malloc(chunk_size);
		memset(chunk_buf, 0, chunk_size);

		_fseeki64(in, (chunk_num * 0x1FFF0) , SEEK_SET);
		fread(chunk_buf, 1, chunk_size, in);
		fwrite(chunk_buf, 1, chunk_size, out);

		makehash(hash, chunk_buf, chunk_size);

		fwrite(hash, 0x10, 1, out);
		free (chunk_buf);
		chunk_num ++;
	}

	//cleaup:
	fclose(out);
	fclose(in);
	
	
	if (argc == 3)
	{
		output_filename = argv[2];
		remove(output_filename);
	}
	else
	{
		output_filename = argv[1];
		remove(input_filename);
	}

	rename(out_fname, output_filename );

	return 0;
}