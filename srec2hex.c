/*
	Authored 2019, Ryan Voo.
	All rights reserved.
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:
	*	Redistributions of source code must retain the above
		copyright notice, this list of conditions and the following
		disclaimer.
	*	Redistributions in binary form must reproduce the above
		copyright notice, this list of conditions and the following
		disclaimer in the documentation and/or other materials
		provided with the distribution.
	*	Neither the name of the author nor the names of its
		contributors may be used to endorse or promote products
		derived from this software without specific prior written
		permission.
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
	BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
	ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void subString(char *  inputLine, int  start, size_t  n, char *  dest)
{
	char *	src = &inputLine[start];

	strncpy(dest, src, n);
	dest[start+n] = '\0';
}

int
main(int argc, char * argv[])
{
	const int	maxSize = 47;
	FILE *		fp = fopen(argv[1], "r");
	FILE *		progFile = fopen("program.hex", "w");
	FILE *		dataFile = fopen("data.hex", "w");
	char *		inputLine = (char*)malloc(maxSize*sizeof(char));
	char *		recordType = (char*)malloc(2*sizeof(char));
	char *		byteCount = (char*)malloc(2*sizeof(char));
	char *		addr_str = (char*)malloc(8*sizeof(char));
	char *		instr_hex;
	int		i;

	if (fp==NULL)
	{
		perror("Couldn't open file for reading.");
		exit(1);
	}
	if (progFile==NULL)
	{
		perror("Couldn't open file for reading.");
		exit(1);
	}
	if (dataFile==NULL)
	{
		perror("Couldn't open file for reading.");
		exit(1);
	}

	for (i=0; i<1024; i++)
	{
		fprintf(dataFile, "%s", "00000000\n");
	}

	while (fgets(inputLine, maxSize, fp)!=NULL)
	{
		size_t	addr_size;
		long	byteCount_long;
		char *	end = NULL;
		char	instruction[9];

		subString(inputLine, 0, 2, recordType);
		subString(inputLine, 2, 2, byteCount);

		switch (recordType[1])
		{
			case '0':
			case '1':
			case '5':
			case '9':
				/*
				 *	16 bits = 4 bytes (hex digits)
				 */
				addr_size = 16 >> 2;
				break;

			case '2':
			case '6':
			case '8':
				addr_size = 24 >> 2;
				break;

			case '3':
			case '7':
				addr_size = 32 >> 2;
				break;

			case '4':
			default:
				break;
		}

		subString(inputLine, 4, addr_size, addr_str);

		/*
		 *	2 bytes addr and 1 byte checksum
		 */
		byteCount_long = strtol(byteCount, &end, 16) - 3;

		instr_hex = (char*)malloc((2*byteCount_long)*sizeof(char));
		subString(inputLine, 4+addr_size, 2*byteCount_long, instr_hex);

		if (recordType[1] == '1')
		{
				for(i=0; i<2*byteCount_long; i+=8)
				{
					/*
					 *	print out every 32 bits
					 */
					instruction[0] = instr_hex[i+6];
					instruction[1] = instr_hex[i+7];
					instruction[2] = instr_hex[i+4];
					instruction[3] = instr_hex[i+5];
					instruction[4] = instr_hex[i+2];
					instruction[5] = instr_hex[i+3];
					instruction[6] = instr_hex[i];
					instruction[7] = instr_hex[i+1];
					instruction[8] = '\0';

					printf("%s\n", instruction);
					fprintf(progFile, "%s\n", instruction);
				}
		}
		free(instr_hex);
	}

	fclose(fp);
	fclose(dataFile);
	fclose(progFile);
}
