#include<stdio.h>
#include <string.h>
#include <stdlib.h>
#include <bitset> 

long* build_freq_table(const char* file_name, const int buffer_size)
{
	FILE* f = fopen(file_name, "rb");
	if (f == NULL)
	{
		printf("Error opening file");
		return NULL;
	}

	long* freq_table = new long[256] {0};

	unsigned char* buffer = new unsigned char[buffer_size];

	int bytesRead;
	while ((bytesRead = fread(buffer, 1, buffer_size, f)) > 0)
	{
		for (int i = 0; i < bytesRead; i++)
		{
			freq_table[buffer[i]]++;
		}
	}

	fclose(f);
	return freq_table;
}

struct tree_node
{
	unsigned char c;
	long freq;
	tree_node* left;
	tree_node* right;
	tree_node* next;

	tree_node(unsigned char c = {}, long freq = 0)
	{
		this->c = c;
		this->freq = freq;
		left = NULL;
		right = NULL;
		next = NULL;
	}
};

class priority_queue
{
	tree_node* front;
	tree_node* back;

public:

	priority_queue()
	{
		front = NULL;
		back = NULL;
	}
	~priority_queue()
	{
		tree_node* it = front;
		while (it != NULL)
		{
			tree_node* tmp = it->next;
			delete (it);
			it = tmp;
		}
	}

	void enqueue_sorted(tree_node* n)
	{
		if (front == NULL)
		{
			front = back = n;
		}
		else if (front->freq > n->freq)
		{
			n->next = front;
			front = n;
		}
		else if (back->freq <= n->freq)
		{
			back->next = n;
			back = n;
		}
		else  
		{
			tree_node* current = front;
			while (current->next != NULL && current->next->freq <= n->freq)
			{
				current = current->next;
			}
			n->next = current->next;
			current->next = n;
		}
	}

	tree_node* dequeue()
	{
		if (front == NULL)
			return NULL;
		tree_node* tmp = front;
		front = front->next;
		tmp->next = NULL;
		return tmp;

	}

	tree_node* peek()
	{
		return front;
	}

};

tree_node* huffmen_tree(priority_queue* q)
{
	if (q == NULL || q->peek() == NULL)
		return NULL;

	while (true)
	{
		if (q->peek()->next == NULL)
			return q->dequeue();

		tree_node* left = q->dequeue();
		tree_node* right = q->dequeue();

		tree_node* root = new tree_node('*', left->freq + right->freq);
		root->left = left;
		root->right = right;
		q->enqueue_sorted(root);
	}
}

void delete_tree(tree_node* root)
{
	if (root == NULL)
		return;

	delete_tree(root->left);
	delete_tree(root->right);

	delete root;
}

void generate_code(tree_node* root, char* code, char** code_table, int depth)  
{
	if (root == NULL)
		return;
	if (root->left == NULL && root->right == NULL)
	{
		code[depth] = '\0';
		strcpy(code_table[root->c], code);
		return;
	}
	code[depth] = '0';
	generate_code(root->left, code, code_table, depth + 1);
	code[depth] = '1';
	generate_code(root->right, code, code_table, depth + 1);
}

char** build_code_table(tree_node* root)
{
	char* code = new char[256] {};
	char** code_table = new char* [256] {};
	for (int i = 0; i < 256; i++)
	{
		code_table[i] = new char[256] {};
	}
	generate_code(root, code, code_table, 0);
	delete[] code;
	return code_table;
}

void encode(const char* input_file, const char* output_file, long* freq_table, char** code_table, const int buffer_size)
{

	FILE* input = fopen(input_file, "rb");
	FILE* output = fopen(output_file, "wb");

	if (input == NULL || output == NULL)
	{
		if (input)
			fclose(input);
		if (output)
			fclose(output);
		return;
	}

	//print frequency table in output for decompression
	fwrite(freq_table, sizeof(long), 256, output);

	unsigned char* input_buffer = new unsigned char[buffer_size] {};
	unsigned char* output_buffer = new unsigned char[buffer_size] {};
	int output_index = 0;

	unsigned char bit_buffer[9] = {};
	int bit_count = 0;

	int bytes_read;
	while ((bytes_read = fread(input_buffer, 1, buffer_size, input)) > 0)
	{
		for (int i = 0; i < bytes_read; i++)
		{
			unsigned char c = input_buffer[i];
			const char* code = code_table[c];

			for (int j = 0; code[j] != '\0'; j++)     
			{
				bit_buffer[bit_count++] = code[j];

				if (bit_count == 8)
				{
					bit_buffer[8] = '\0';
					unsigned char byte = strtol((const char*)bit_buffer, NULL, 2);
					output_buffer[output_index++] = byte;
					bit_count = 0;

					if (output_index == buffer_size)
					{
						fwrite(output_buffer, 1, buffer_size, output);
						output_index = 0;
					}
				}
			}
		}
	}

	//padding
	if (bit_count > 0)
	{
		for (int i = bit_count; i < 8; i++)
		{
			bit_buffer[i] = '0';  //Pad with zeros
		}

		bit_buffer[8] = '\0';
		unsigned char byte = strtol((const char*)bit_buffer, NULL, 2);
		output_buffer[output_index++] = byte;
	}

	if (output_index > 0)
	{
		fwrite(output_buffer, 1, output_index, output);
	}

	delete[] input_buffer;
	delete[] output_buffer;

	fclose(input);
	fclose(output);
}

void compression(const char* input_file, const int buffer_size)
{
	long* freq_table = build_freq_table(input_file, buffer_size);
	if (freq_table == NULL)
		return;

	priority_queue* q = new priority_queue();
	for (int i = 0; i < 256; i++)
	{
		if (freq_table[i] > 0)
		{
			tree_node* n = new tree_node((unsigned char)i, freq_table[i]);
			q->enqueue_sorted(n);
		}
	}

	tree_node* root = huffmen_tree(q);
	if (root == NULL)
	{
		delete q;
		delete[] freq_table;
		return;
	}

	char** code_table = build_code_table(root);

	char output_file[256];
	strcpy(output_file, input_file);
	strcat(output_file, ".ece2103");

	encode(input_file, output_file, freq_table, code_table, buffer_size);

	delete_tree(root);
	delete q;
	delete[] freq_table;
	for (int i = 0; i < 256; i++)
	{
		delete[] code_table[i];
	}
	delete[] code_table;
}

void decode(FILE* input, const char* output_file, tree_node* root, int buffer_size)
{

	FILE* output = fopen(output_file, "wb");
	if (output == NULL)
		return;

	unsigned char* input_buffer = new unsigned char[buffer_size] {};
	unsigned char* output_buffer = new unsigned char[buffer_size] {};
	int output_index = 0;
	char bit_buffer[9] = {};
	tree_node* current = root;

	int bytes_read;
	while ((bytes_read = fread(input_buffer, 1, buffer_size, input)) > 0)
	{
		for (int i = 0; i < bytes_read; i++)
		{
			unsigned char byte = input_buffer[i];
			std::bitset<8>bits(byte); //from chat GPT
			strcpy(bit_buffer, bits.to_string().c_str());//from chat GPT

			for (int j = 0; j < 8; j++)
			{
				if (bit_buffer[j] == '0')
					current = current->left;
				else if (bit_buffer[j] == '1')
					current = current->right;

				if (current->left == NULL && current->right == NULL)
				{
					output_buffer[output_index++] = current->c;
					current = root;
					if (output_index == buffer_size)
					{
						fwrite(output_buffer, 1, output_index, output);
						output_index = 0;
					}
				}
			}
		}
	}

	if (output_index > 0)
	{
		fwrite(output_buffer, 1, output_index, output);
	}

	delete[] input_buffer;
	delete[] output_buffer;

	fclose(output);
	fclose(input);
}

void decompression(const char* input_file, const int buffer_size)
{
	FILE* input = fopen(input_file, "rb");
	if (input == NULL)
		return;

	long freq_table[256] = {};
	fread(freq_table, sizeof(long), 256, input);

	priority_queue* q = new priority_queue();
	for (int i = 0; i < 256; i++)
	{
		if (freq_table[i] > 0)
		{
			tree_node* n = new tree_node((unsigned char)i, freq_table[i]);
			q->enqueue_sorted(n);
		}
	}

	tree_node* root = huffmen_tree(q);
	if (root == NULL)
	{
		delete q;
		return;
	}

	char** code_table = build_code_table(root);

	char output_file[256];
	strcpy(output_file, input_file);
	int length = strlen(output_file) - 8;
	output_file[length] = '\0';

	decode(input, output_file, root, buffer_size);

	delete_tree(root);
	delete q;
	for (int i = 0; i < 256; i++)
	{
		delete[] code_table[i];
	}
	delete[] code_table;

}


int main(int argc, char* argv[])
{

	int buffer_size = 8192;

	if (argc < 2)
	{
		printf("\n");
		printf("                ===============================================================================\n");
		printf("                |                           FILE COMPRESSOR TOOL                              |\n");
		printf("                |                         --------------------------                          |\n");
		printf("                |                    [ Usage Instructions & Help Guide ]                      |\n");
		printf("                ===============================================================================\n");
		printf("                | Description:                                                                |\n");
		printf("                |   This tool allows you to compress or decompress files                      |\n");
		printf("                |   with an optional custom buffer size.                                      |\n");
		printf("                -------------------------------------------------------------------------------\n");
		printf("                | Modes of Operation:                                                         |\n");
		printf("                |   > -c <input_file> <output_file>        Compress the input file            |\n");
		printf("                |   > -d <input_file> <output_file>        Decompress the input file          |\n");
		printf("                |   > -b <size> -c|-d <input> <output>     Set buffer size before action      |\n");
		printf("                -------------------------------------------------------------------------------\n");
		printf("                | Examples:                                                                   |\n");
		printf("                |   Compress a file:                                                          |\n");
		printf("                |     ./your_program -c input.FileType output.exe2103                         |\n");
		printf("                |                                                                             |\n");
		printf("                |   Decompress a file:                                                        |\n");
		printf("                |     ./your_program -d input.exe2103 output.FileType                         |\n");
		printf("                |                                                                             |\n");
		printf("                |   Compress with buffer size:                                                |\n");
		printf("                |     ./your_program -b 16384 -c input.FileType output.exe2103                |\n");
		printf("                |                                                                             |\n");
		printf("                |   Decompress with buffer size:                                              |\n");
		printf("                |     ./your_program -b 16384 -d input.exe2103 output.FileType                |\n");
		printf("                -------------------------------------------------------------------------------\n");
		printf("                | NOTE: Default buffer size is 8192 bytes if not specified                    |\n");
		printf("                ===============================================================================\n");
		printf("\n");

		return 1;
	}


	if (strcmp(argv[1], "-c") == 0)
	{
		if (argc != 4)
		{

			printf("                -------------------------------------------------------------------------------\n");
			printf("                |                ERORR: INVALID NUMBER OF ARGUMENTS FOR COMPRESSION           |\n");
			printf("                -------------------------------------------------------------------------------\n");
			return 1;
		}
		printf("                -------------------------------------------------------------------------------\n");
		printf("                |                               Loading...                                    |\n");
		printf("                -------------------------------------------------------------------------------\n");
		compression(argv[2], buffer_size);
		printf("                -------------------------------------------------------------------------------\n");
		printf("                |                   COMPRESSION COMPLETED SUCCESSFULLY                        |\n");
		printf("                -------------------------------------------------------------------------------\n");
		printf("                -------------------------------------------------------------------------------\n");
		printf("                 OUTPUT FILE : %s                                 \n", argv[3]);
		printf("                -------------------------------------------------------------------------------\n");



	}
	else if (strcmp(argv[1], "-d") == 0)
	{
		if (argc != 4)
		{

			printf("                -------------------------------------------------------------------------------\n");
			printf("                |                ERORR: INVALID NUMBER OF ARGUMENTS FOR DECOMPRESSION         |\n");
			printf("                -------------------------------------------------------------------------------\n");
			return 1;
		}
		printf("                -------------------------------------------------------------------------------\n");
		printf("                |                                Loading...                                   |\n");
		printf("                -------------------------------------------------------------------------------\n");
		decompression(argv[2], buffer_size);
		printf("                -------------------------------------------------------------------------------\n");
		printf("                |                   DECOMPRESSION COMPLETED SUCCESSFULLY                      |\n");
		printf("                -------------------------------------------------------------------------------\n");
		printf("                -------------------------------------------------------------------------------\n");
		printf("                 OUTPUT FILE : %s                                 \n", argv[3]);
		printf("                -------------------------------------------------------------------------------\n");

	}
	else if (strcmp(argv[1], "-b") == 0)
	{
		if (argc != 6)
		{

			printf("                -------------------------------------------------------------------------------\n");
			printf("                |                ERORR: INVALID NUMBER OF ARGUMENTS FOR BUFFER SIZE           |\n");
			printf("                -------------------------------------------------------------------------------\n");
			return 1;
		}
		buffer_size = atoi(argv[2]);
		if (buffer_size <= 0)
		{

			printf("                -------------------------------------------------------------------------------\n");
			printf("                |                ERORR:BUFFER SIZE MUST BE A POSTIVE INTEGER                  |\n");
			printf("                -------------------------------------------------------------------------------\n");
			return 1;
		}

		printf("                -------------------------------------------------------------------------------\n");
		printf("                |                               BUFFER SIZE SET TO %d                       \n", buffer_size);
		printf("                -------------------------------------------------------------------------------\n");
		if (strcmp(argv[3], "-c") == 0)
		{
			printf("                -------------------------------------------------------------------------------\n");
			printf("                |                                Loading...                                   |\n");
			printf("                -------------------------------------------------------------------------------\n");
			compression(argv[4], buffer_size);
			printf("                -------------------------------------------------------------------------------\n");
			printf("                |                   COMPRESSION COMPLETED SUCCESSFULLY                        |\n");
			printf("                -------------------------------------------------------------------------------\n");
			printf("                -------------------------------------------------------------------------------\n");
			printf("                 OUTPUT FILE : %s                                 \n", argv[5]);
			printf("                -------------------------------------------------------------------------------\n");

		}
		else if (strcmp(argv[3], "-d") == 0)
		{
			printf("                -------------------------------------------------------------------------------\n");
			printf("                |                                Loading...                                   |\n");
			printf("                -------------------------------------------------------------------------------\n");
			decompression(argv[4], buffer_size);
			printf("                -------------------------------------------------------------------------------\n");
			printf("                |                   DECOMPRESSION COMPLETED SUCCESSFULLY                      |\n");
			printf("                -------------------------------------------------------------------------------\n");
			printf("                -------------------------------------------------------------------------------\n");
			printf("                 OUTPUT FILE : %s                                 \n", argv[5]);
			printf("                -------------------------------------------------------------------------------\n");

		}
		else
		{

			printf("                -------------------------------------------------------------------------------\n");
			printf("                |                   ERORR:UNKOWN ACTION '%s'. USE -c or -d.                   |\n", argv[3]);
			printf("                -------------------------------------------------------------------------------\n");
			return 1;
		}
	}
	else
	{

		printf("                -------------------------------------------------------------------------------\n");
		printf("                |                          ERORR:UNKOWN OPTION '%s'.                           |\n", argv[1]);
		printf("                -------------------------------------------------------------------------------\n");
		return 1;
	}

	return 0;
}