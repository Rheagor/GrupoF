## Projeto regex-redux Grupo F

**COMP0397 - Programação Paralela e Concorrente**

Prof. Hendrik Macedo

**Alunos**: 
Eufranio Santos Lima     
David de Almeida Cunha            
João Pedro Santos da Silva       
José Ilmar Cruz Freire Neto          
Matheus Lima dos Santos     
Wesley Araujo Souza



## Tabela de Comparação de Tempos em Paralelo
Para [fasta_50000]():
| Nº de Cores | Rust | C |
| ------------------- | ------------------- | ------------------- |
| 1 | 0.784 s | 1.346 s |
| 2 | 0.730 s | 1.343 s |
| 3 | 0.672 s | 1.276 s |
| 4 | 0.681 s | 1.256 s |
Para [fasta_150000]():
| Nº de Cores | Rust | C |
| ------------------- | ------------------- | ------------------- |
| 1 | 2.176 s | 13.163 s |
| 2 | 1.870 s | 13.145 s |
| 3 | 1.746 s | 13.151 s |
| 4 | 1.761 s | 12.448 s |


## Tabela de Comparação de Tempos Sequencial
Para [fasta_50000]():
| Rust | C |
| ------------------- | ------------------- |
| 0.642 s | 1.226 s |
Para [fasta_150000]():
| Rust | C |
| ------------------- | ------------------- |
| 1.957 s | 12.71 s |


## Códigos-Fonte

[Rust Sequencial](https://raw.githubusercontent.com/Rheagor/GrupoF/master/serial/sequencial.rs?token=AKC5CGAHHFINHQPV4OZZA3K73JSGO)     
[Rust Paralelo](https://raw.githubusercontent.com/Rheagor/GrupoF/master/paralelo/paralelo.rs?token=AKC5CGC5NUKSOMBXWMDPOKC73JSA2)    
[C Paralelo](https://raw.githubusercontent.com/Rheagor/GrupoF/master/paralelo/regex_redux_mpi.c?token=AKC5CGHZN44SWZR4M2TO3P273JSC4)     
[C Sequencial](https://raw.githubusercontent.com/Rheagor/GrupoF/master/serial/regex_redux_serial.c?token=AKC5CGG6LLLTAXIVM6EUJZS73JSGI)    
