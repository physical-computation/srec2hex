/*
	Authored 2019, Phillip Stanley-Marbell.
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
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <float.h>
#include <signal.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <getopt.h>
#include <inttypes.h>

static void	processFile(uint64_t codeAddress, char *  fileName);
static void	usage(void);
static void	version(void);

enum
{
	kMaxSrecordLineLength	=	1024,
};

const char *	kSrec2hexVersion = "0.2";



int
main(int argc, char *  argv[])
{
	uint64_t	codeAddress;


	while (1)
	{
		char			tmp;
		char *			ep = &tmp;
		int			optionIndex	= 0, c;
		static struct option	options[]	=
		{
			{"help",		no_argument,		0,	'h'},
			{"version",		no_argument,		0,	'V'},
			{"base",		required_argument,	0,	'b'},
			{0,			0,			0,	0}
		};

		c = getopt_long(argc, argv, "hVb:", options, &optionIndex);

		if (c == -1)
		{
			break;
		}

		switch (c)
		{
			case 0:
			{
				/*
				 *	Not sure what the expected behavior for getopt_long is here...
				 */
				break;
			}

			case 'h':
			{
				usage();
				exit(EXIT_SUCCESS);

				/*	Not reached 	*/
				break;
			}

			case 'V':
			{
				version();
				exit(EXIT_SUCCESS);

				/*	Not reached 	*/
				break;
			}

			case 'b':
			{
				/*
				 *	TODO: Rather than accepting the raw enum value as integer,
				 *	accept string and compare to table of options
				 */
				uint64_t tmpInt = strtoul(optarg, &ep, 0);
				if (*ep == '\0')
				{
					codeAddress = tmpInt;
				}
				else
				{
					usage();
					exit(EXIT_FAILURE);
				}

				break;
			}

			case '?':
			{
				/*
				 *	getopt_long() should have already printed an error message.
				 */
				usage();
				exit(EXIT_FAILURE);

				break;
			}

			default:
			{
				usage();
				exit(EXIT_FAILURE);
			}
		}
	}

	if (optind < argc)
	{
		while (optind < argc)
		{
			processFile(codeAddress, argv[optind++]);
		}
	}
	else
	{
		fprintf(stderr, "\nError: No input.\n");
		usage();
		exit(EXIT_FAILURE);
	}
}

void
processFile(uint64_t codeAddress, char *  fileName)
{
	struct stat	sb;
	char *		fileBuf;
	char		buf[kMaxSrecordLineLength];
	int 		fileSize, filePosition, inputFd, i, n;
	uint32_t	recordAddress;
	int		pcset, recordType, recordLength;
	char *		line;
	FILE *		programFILE;
	FILE *		dataFILE;
	bool		isData = false;


	if ((inputFd = open(fileName, O_RDONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) < 0)
	{
		fprintf(stderr, "Open of \"%s\" failed...\n", fileName);
		exit(EXIT_FAILURE);
	}

	programFILE	= fopen("program.hex", "w");
	if (programFILE == NULL)
	{
		fprintf(stderr, "Open of \"program.hex\" failed...\n");
		exit(EXIT_FAILURE);
	}
	dataFILE	= fopen("data.hex", "w");
	if (dataFILE == NULL)
	{
		fprintf(stderr, "Open of \"data.hex\" failed...\n");
		exit(EXIT_FAILURE);
	}

	if (fstat(inputFd, &sb) < 0)
	{
		fprintf(stderr, "Determining size of \"%s\" failed...\n\n", fileName);
		exit(EXIT_FAILURE);
	}
	fileSize = sb.st_size;

	fileBuf = (char *)calloc(fileSize, sizeof(char));
	if (fileBuf == NULL)
	{
		fprintf(stderr, "Could not allocate memory for fileBuf in main.c");
		exit(EXIT_FAILURE);
	}
	
	if ((n = read(inputFd, fileBuf, fileSize)) != fileSize)
	{
		fprintf(stderr, "Expected [%d] bytes in [%s], read [%d]", fileSize, fileName, n);
		exit(EXIT_FAILURE);
	}

	fprintf(stderr, "\nsrec2hex assumes the S-record file contains S1 or S3 records.\n\n"
			"S1 will be generated by default by objdump if the address range can fit in 16 bits.\n"
			"S3 records are generated by objdump if the address range will only fit in 32 bits.\n\n");

	pcset = 0;
	filePosition = 0;
	for (;;)
	{
		i = 0;

		if (filePosition == fileSize)
		{
			return;
		}

		while (i < kMaxSrecordLineLength && filePosition < fileSize && fileBuf[filePosition] != '\n')
		{
			buf[i++] = fileBuf[filePosition++];
		}
		filePosition++;

		if (i == kMaxSrecordLineLength)
		{
			fprintf(stderr, "Absurdly long SREC line !\n");
			exit(EXIT_FAILURE);
		}

		buf[i] = '\0';

		line = &buf[0];
		recordType = line[1] - '0';

		switch (recordType)
		{
			case 0:
			{
				/*	Optional starting record. Skip	*/
				break;
			}

			case 1:
			{
				/*	Data record with 16bit address	*/
				char	*tptr, tmp[8+1];


				memmove(&tmp[0], &line[2], 2);
				tmp[2] = '\0';
				recordLength = strtoul(&tmp[0], NULL, 16);

				memmove(&tmp[0], &line[4], 4);
				tmp[4] = '\0';
				recordAddress = strtoul(&tmp[0], NULL, 16);

				if (!pcset)
				{
					fprintf(stderr,
						"Splitting S-RECORD which was originally targeted at memory address 0x%"PRIX32",\n"
						"Assuming code ends at 0x%08"PRIX64" and data start is aligned on a 32-byte (S-record line) boundary...\n"
						"(Generated .hex files have no explicit addresses in them)\n\n",
						recordAddress, codeAddress);
					pcset = 1;
				}

				/*	recordLength includes length of addr and chksum	*/
				recordLength -= 3;

				/*
				 *	Bug: This assumes the recordLength is a multiple of 4
				 */
				for (int bytesInRecordSeen = 0; bytesInRecordSeen < recordLength; bytesInRecordSeen += 4)
				{
					if (recordAddress+bytesInRecordSeen >= codeAddress)
					{
						fprintf(stderr, "\ndata:\t");
						isData = true;
					}
					else
					{
						fprintf(stderr, "\ninstr:\t");
						isData = false;
					}

					tptr = &line[8+(bytesInRecordSeen*2)+6];
					memmove(&tmp[0], tptr, 2);
					tmp[2] = '\0';
					fprintf(stderr, "%02lX ", strtoul(&tmp[0], NULL, 16));
					if (isData)
					{
						fprintf(dataFILE, "%02lX", strtoul(&tmp[0], NULL, 16));
					}
					else
					{
						fprintf(programFILE, "%02lX", strtoul(&tmp[0], NULL, 16));
					}

					tptr = &line[8+(bytesInRecordSeen*2)+4];
					memmove(&tmp[0], tptr, 2);
					tmp[2] = '\0';
					fprintf(stderr, "%02lX ", strtoul(&tmp[0], NULL, 16));
					if (isData)
					{
						fprintf(dataFILE, "%02lX", strtoul(&tmp[0], NULL, 16));
					}
					else
					{
						fprintf(programFILE, "%02lX", strtoul(&tmp[0], NULL, 16));
					}

					tptr = &line[8+(bytesInRecordSeen*2)+2];
					memmove(&tmp[0], tptr, 2);
					tmp[2] = '\0';
					fprintf(stderr, "%02lX ", strtoul(&tmp[0], NULL, 16));
					if (isData)
					{
						fprintf(dataFILE, "%02lX", strtoul(&tmp[0], NULL, 16));
					}
					else
					{
						fprintf(programFILE, "%02lX", strtoul(&tmp[0], NULL, 16));
					}

					tptr = &line[8+(bytesInRecordSeen*2)+0];
					memmove(&tmp[0], tptr, 2);
					tmp[2] = '\0';
					fprintf(stderr, "%02lX ", strtoul(&tmp[0], NULL, 16));
					if (isData)
					{
						fprintf(dataFILE, "%02lX", strtoul(&tmp[0], NULL, 16));
					}
					else
					{
						fprintf(programFILE, "%02lX", strtoul(&tmp[0], NULL, 16));
					}

					if (isData)
					{
						fprintf(dataFILE, "\n");
					}
					else
					{
						fprintf(programFILE, "\n");
					}
				}
				fprintf(stderr, "\n");

				break;
			}

			case 2:
			{
				/*	Data record with 24bit address	*/
				break;
			}

			/*								*/
			/*	TODO:    We do not verify checksum on SREC records	*/
			/*								*/
			case 3:
			{
				/*	Data record with 32bit addr	*/
				char	*tptr, tmp[8+1];
				
				memmove(&tmp[0], &line[2], 2);
				tmp[2] = '\0';
				recordLength = strtoul(&tmp[0], NULL, 16);

				memmove(&tmp[0], &line[4], 8);
				tmp[8] = '\0';
				recordAddress = strtoul(&tmp[0], NULL, 16);

				if (!pcset)
				{
					fprintf(stderr,
						"Splitting S-RECORD which was originally targeted at memory address 0x%"PRIX32",\n"
						"assuming code ends at 0x%08"PRIX64" and data start is aligned on a 32-byte (S-record line) boundary...\n"
						"(generated .hex files have no explicit addresses in them)\n\n",
						recordAddress, codeAddress);
					pcset = 1;
				}

				/*	recordLength includes length of addr and chksum	*/
				recordLength -= 5;

				/*
				 *	Bug: This assumes the recordLength is a multiple of 4
				 */
				for (int bytesInRecordSeen = 0; bytesInRecordSeen < recordLength; bytesInRecordSeen += 4)
				{
					if (recordAddress+bytesInRecordSeen >= codeAddress)
					{
						fprintf(stderr, "\ndata:\t");
						isData = true;
					}
					else
					{
						fprintf(stderr, "\ninstr:\t");
						isData = false;
					}

					tptr = &line[12+(bytesInRecordSeen*2)+6];
					memmove(&tmp[0], tptr, 2);
					tmp[2] = '\0';
					fprintf(stderr, "%02lX ", strtoul(&tmp[0], NULL, 16));
					if (isData)
					{
						fprintf(dataFILE, "%02lX", strtoul(&tmp[0], NULL, 16));
					}
					else
					{
						fprintf(programFILE, "%02lX", strtoul(&tmp[0], NULL, 16));
					}

					tptr = &line[12+(bytesInRecordSeen*2)+4];
					memmove(&tmp[0], tptr, 2);
					tmp[2] = '\0';
					fprintf(stderr, "%02lX ", strtoul(&tmp[0], NULL, 16));
					if (isData)
					{
						fprintf(dataFILE, "%02lX", strtoul(&tmp[0], NULL, 16));
					}
					else
					{
						fprintf(programFILE, "%02lX", strtoul(&tmp[0], NULL, 16));
					}

					tptr = &line[12+(bytesInRecordSeen*2)+2];
					memmove(&tmp[0], tptr, 2);
					tmp[2] = '\0';
					fprintf(stderr, "%02lX ", strtoul(&tmp[0], NULL, 16));
					if (isData)
					{
						fprintf(dataFILE, "%02lX", strtoul(&tmp[0], NULL, 16));
					}
					else
					{
						fprintf(programFILE, "%02lX", strtoul(&tmp[0], NULL, 16));
					}

					tptr = &line[12+(bytesInRecordSeen*2)+0];
					memmove(&tmp[0], tptr, 2);
					tmp[2] = '\0';
					fprintf(stderr, "%02lX ", strtoul(&tmp[0], NULL, 16));
					if (isData)
					{
						fprintf(dataFILE, "%02lX", strtoul(&tmp[0], NULL, 16));
					}
					else
					{
						fprintf(programFILE, "%02lX", strtoul(&tmp[0], NULL, 16));
					}

					if (isData)
					{
						fprintf(dataFILE, "\n");
					}
					else
					{
						fprintf(programFILE, "\n");
					}
				}
				fprintf(stderr, "\n");

				break;

			}

			case 4:
			{
				/*	Symbol record (LSI extension)	*/
				break;
			}

			case 5:
			{
				/*	# records in preceding block	*/
				break;
			}

			case 6:
			{
				/*		Unused			*/
				break;
			}

			case 7:
			{
				/*	End record for S3 records	*/
				break;
			}

			case 8:
			{
				/*	End record for S2 records	*/
				break;
			}

			case 9:
			{
				/*	End record for S1 records	*/
				break;
			}
	
			default:
			{
				fprintf(stderr, "Seen unknown SRECORD type.\n");
				fprintf(stderr, "Aborting SRECL.\n");
				exit(EXIT_FAILURE);
			}
		}
	}
	fflush(programFILE);
	fflush(dataFILE);
	close(inputFd);


	return;
}

void
version(void)
{
	fprintf(stderr, "\nsrec2hex version %s.\n\n", kSrec2hexVersion);
}


void
usage(void)
{
	version();
	fprintf(stderr,	"Usage:    srec2hex\n"
			"                [ (--help, -h)                                               \n"
			"                | (--version, --V)                                           \n"
			"                | (--base <hexadecimal base address>, -b <hexadecimal base address>)                           \n"
			"                                                                             \n"
			"              <filename>\n\n");
}
