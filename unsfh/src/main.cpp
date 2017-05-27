#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define ES32(v)((unsigned int)(((v & 0xFF000000) >> 24) | \
                               ((v & 0x00FF0000) >> 8 ) | \
							   ((v & 0x0000FF00) << 8 ) | \
							   ((v & 0x000000FF) << 24)))

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("unsfh v.0.0.1.\n");
		printf("Usage: unsfh.exe <file_in> [file_out]\n\n");
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
	if (in_size < 0x20 || in_size > 0xFFFFFFFF)
	{
		printf("Error! Invalid input file detected, exiting.\n");
		fclose(in);
		return 0;
	}
	printf("File size : 0x%X bytes\n", in_size);

	//Reading header:
	unsigned char *header = (unsigned char *)malloc(0x10);
	memset(header, 0, 0x10);
	fread(header, 1, 0x10, in);
	unsigned int magic = ES32(*(unsigned int*)&header[0x0]);
	unsigned int data_size = ES32(*(unsigned int*)&header[0x08]);
	free (header);

	//Checking magic:
	if (magic != 0x534648)
	{
		printf("Error! Invalid magic detected, exiting.\n");
		fclose(in);
		return 0;
	}
	//print info:
	printf("Magic : 0x%X \n", magic);
	printf("Size  : 0x%X \n", data_size);

	//writing data:
	unsigned int bytes_to_read = data_size;
	_fseeki64(in, 0x10, SEEK_SET);

	remove(out_fname);
	out = fopen(out_fname, "ab");

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
		unsigned char *chunk_buf = (unsigned char *)malloc(chunk_size);
		_fseeki64(in, (0x10 + (chunk_num * 0x20000)) , SEEK_SET);
		fread(chunk_buf, 1, chunk_size, in);
		fwrite(chunk_buf, 1, chunk_size, out);
		free (chunk_buf);
		chunk_num ++;
	}

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