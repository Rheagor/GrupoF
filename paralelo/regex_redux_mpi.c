#include <stdio.h>
#include <string.h>
#include <pcre.h>
#include <mpi.h>

#define FB_MINREAD (3 << 16)
#define MIN(a,b) (((a)<(b))?(a):(b))

const char *variants[] = 
{
	"agggtaaa|tttaccct",         "[cgt]gggtaaa|tttaccc[acg]",
	"a[act]ggtaaa|tttacc[agt]t", "ag[act]gtaaa|tttac[agt]ct",
	"agg[act]taaa|ttta[agt]cct", "aggg[acg]aaa|ttt[cgt]ccct",
	"agggt[cgt]aa|tt[acg]accct", "agggta[cgt]a|t[acg]taccct",
	"agggtaa[cgt]|[acg]ttaccct", NULL
};

const char *subst[] = 
{
	"tHa[Nt]", "<4>",
	"aND|caN|Ha[DS]|WaS", "<3>",
	"a[NSt]|BY", "<2>",
	"<[^>]*>", "|",
	"\\|[^|][^|]*\\|", "-",
	NULL
};

char *replace(const char *input, const char *exp, const char *replacement)
{
	pcre *re;
	const char *errorcode;
	int erroffset;
	if (!(re = pcre_compile(exp, 0, &errorcode, &erroffset, NULL)))
	{
		printf("error in pcre compile\n");
		exit(1);
	}

	char *output = malloc(sizeof(char));
	output[0] = '\0';

	pcre_extra *re_extra = pcre_study(re, PCRE_STUDY_JIT_COMPILE, &errorcode);
	int pos = 0;
	int m[3];
	int input_len = strlen(input);
	int clen;
	int rlen = strlen(replacement);
	int count = 0;
	while (pcre_exec(re, re_extra, input, input_len, pos, 0, m, 3) >= 0)
	{
		clen = m[0] - pos;

		if (!(output = realloc(output, (strlen(output) + clen + rlen + 1) * sizeof(char))))
		{
			printf("error in realloc");
			exit(1);
		}

		int output_len = strlen(output);
		memcpy(&output[output_len], &input[pos], clen);
		output_len += clen;
		output[output_len] = '\0';
		strcat(output, replacement);

		pos = m[1];
	}
	clen = input_len - pos;
	if (!(output = realloc(output, (strlen(output) + clen + 1) * sizeof(char))))
	{
		printf("error in realloc");
		exit(1);
	}
	int output_len = strlen(output);
	memcpy(&output[output_len], &input[pos], clen);
	output_len += clen;
	output[output_len] = '\0';

	return output;
}

int count(const char *input, const char *exp)
{
	pcre *re;
	const char *errorcode;
	int erroffset;
	if (!(re = pcre_compile(exp, 0, &errorcode, &erroffset, NULL)))
	{
		printf("error in pcre compile\n");
		exit(1);
	}

	pcre_extra *re_extra = pcre_study(re, PCRE_STUDY_JIT_COMPILE, &errorcode);
	int count = 0;
	int pos = 0;
	int m[3];
	int input_len = strlen(input);
	while (pcre_exec(re, re_extra, input, input_len, pos, 0, m, 3) >= 0)
	{
		count++;
		pos = m[1];
	}
	return count;
}

int main()
{
	int comm_sz, my_rank;

	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

//1º
	char *fasta;
	int fasta_alloc;
	int fasta_len, proc_fasta_len;

	if (my_rank == 0)
	{
		fasta = malloc(sizeof(char));
		fasta[0] = '\0';
		fasta_alloc = 1;
		char *line = malloc(FB_MINREAD * sizeof(char));
		while (fgets(line, FB_MINREAD, stdin))
		{
			if (strlen(fasta) + strlen(line) + 1 > fasta_alloc)
			{
				fasta_alloc += FB_MINREAD;
				if (!(fasta = realloc(fasta, fasta_alloc * sizeof(char))))
				{
					printf("error in realloc");
					exit(1);
				}
			}

			strcat(fasta, line);
		}

		fasta_len = strlen(fasta) + 1; //add 1, ele considera o final como newline =|

//2º
		char *proc_fasta = replace(fasta, ">.*|\n", ""); //remover os comentários e o char newline
		free(fasta);
		fasta = proc_fasta;
		proc_fasta_len = strlen(fasta);

		MPI_Bcast(&proc_fasta_len, 1, MPI_INT, 0, MPI_COMM_WORLD);
		MPI_Bcast(fasta, proc_fasta_len + 1, MPI_CHAR, 0, MPI_COMM_WORLD);
	} else {
		MPI_Bcast(&proc_fasta_len, 1, MPI_INT, 0, MPI_COMM_WORLD);
		fasta = malloc(proc_fasta_len * sizeof(char));
		MPI_Bcast(fasta, proc_fasta_len + 1, MPI_CHAR, 0, MPI_COMM_WORLD);
	}

//3º
	int variants_len = (sizeof(variants) / sizeof(char *)) - 1;
	int local_n = variants_len / comm_sz;
	int local_a = my_rank * local_n;
	int remainder = variants_len % comm_sz;
	if (remainder)
	{
		if (my_rank < remainder)
			local_n++;

		local_a += MIN(remainder, my_rank);
	}
	int local_b = local_a + local_n;

	int count_arr[variants_len];
	int local_count_arr[local_n];
	for (int i = local_a; i < local_b; i++)
	{
		local_count_arr[i - local_a] = count(fasta, variants[i]);
	}

	int displs[comm_sz];
	int rec_counts[comm_sz];
	for (int i = 0; i < comm_sz; i++)
	{
		int n = variants_len / comm_sz;
		int a = i * n;
		int remainder = variants_len % comm_sz;
		if (remainder)
		{
			if (i < remainder)
				n++;

			a += MIN(remainder, i);
		}
		displs[i] = a;
		rec_counts[i] = n;
	}
	MPI_Gatherv(local_count_arr, local_n, MPI_INT, count_arr, rec_counts, displs, MPI_INT, 0, MPI_COMM_WORLD);


	if (my_rank == 0)
	{
		for (int i = 0; i < variants_len; i++)
		{
			printf("%s %d\n", variants[i], count_arr[i]);
		}
//4
		for (const char **subs_ptr = subst; *subs_ptr; subs_ptr += 2)
		{
			char *repl_fasta = replace(fasta, *subs_ptr, *(subs_ptr + 1));
			free(fasta);
			fasta = repl_fasta;
		}

//5º
		int final_fasta_len = strlen(fasta);

		printf("\n");
		printf("%d\n%d\n%d\n", fasta_len, proc_fasta_len, final_fasta_len);
	}

	MPI_Finalize();

	return 0;
}