#include <stdio.h>
#include <string.h>
#include <pcre.h>

#define FB_MINREAD (3 << 16)

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

int main(int argc, char **argv)
{
	//1º
	/* read all of a redirected FASTA format file from stdin, and record the sequence length */

	char *fasta = malloc(sizeof(char));
	fasta[0] = '\0';
	int fasta_alloc = 1;
	char *line = malloc(FB_MINREAD * sizeof(char));
	while (fgets(line, FB_MINREAD, stdin))
	{
		if (strlen(fasta) + strlen(line) + 1 > fasta_alloc)
		{
			fasta_alloc = strlen(fasta) + strlen(line) + 1;
			if (!(fasta = realloc(fasta, fasta_alloc * sizeof(char))))
			{
				printf("error in realloc");
				exit(1);
			}
		}

		strcat(fasta, line);
	}

	int fasta_len = strlen(fasta) + 1; //add 1, ele considera o final como newline =|

	//2º
	/* use the same simple regex pattern match-replace to remove FASTA sequence descriptions
	   and all linefeed characters, and record the sequence length */

	char *proc_fasta = replace(fasta, ">.*|\n", ""); //remover os comentários e o char newline
	free(fasta);
	fasta = proc_fasta;
	int proc_fasta_len = strlen(fasta);

	//3º
	/* use the same simple regex patterns -
	   agggtaaa|tttaccct
	   [cgt]gggtaaa|tttaccc[acg]
	   a[act]ggtaaa|tttacc[agt]t
	   ag[act]gtaaa|tttac[agt]ct
	   agg[act]taaa|ttta[agt]cct
	   aggg[acg]aaa|ttt[cgt]ccct
	   agggt[cgt]aa|tt[acg]accct
	   agggta[cgt]a|t[acg]taccct
	   agggtaa[cgt]|[acg]ttaccct
       - representing DNA 8-mers and their reverse complement (with a wildcard in one position), 
       and (one pattern at a time) count matches in the redirected file */

	for (const char **variants_ptr = variants; *variants_ptr; variants_ptr++)
	{
		int variant_count = count(fasta, *variants_ptr);
		printf("%s %d\n", *variants_ptr, variant_count);
	}

	//4º
	/*use the same magic regex patterns -
      tHa[Nt]
      aND|caN|Ha[DS]|WaS
      a[NSt]|BY
      <[^>]*>
      \\|[^|][^|]*\\|
      - to (one pattern at a time, in the same order) match-replace the pattern in the redirected file with -

      <4>
      <3>
      <2>
      |
      -
      - and record the sequence length*/

	for (const char **subs_ptr = subst; *subs_ptr; subs_ptr += 2)
	{
		char *repl_fasta = replace(fasta, *subs_ptr, *(subs_ptr + 1));
		free(fasta);
		fasta = repl_fasta;
	}

	//4º
	/*write the 3 recorded sequence lengths*/

	int final_fasta_len = strlen(fasta);

	printf("\n");
	printf("%d\n%d\n%d\n", fasta_len, proc_fasta_len, final_fasta_len);
	return 0;
}
