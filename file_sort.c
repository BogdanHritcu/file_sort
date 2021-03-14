/*
A10. (6 puncte) Scrieti un program care sa sorteze prin interclasare un fisier
 de caractere in maniera descrisa mai jos.
  Sortarea prin interclasare presupune impartirea sirului in doua jumatati,
 sortarea fiecarei parti prin aceeasi metoda (deci recursiv), apoi 
 interclasarea celor doua parti (care sunt acum sortate).
  Programul va lucra astfel: se imparte sirul in doua, se genereaza doua 
 procese fiu (fork) care sorteaza cele doua jumatati in paralel, tatal ii 
 asteapta sa se termine (wait sau waitpid), apoi interclaseaza jumatatile.
 Nu se vor folosi fisiere auxiliare.
*/
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

void merge(int d, off_t len, off_t l, off_t r);

int main(int argc, char** argv)
{
    char* file_name = NULL;

    int i;
    int opt_newline = 0;
    for(i = 1; i < argc; i++)
    {
        if(argv[i][0] == '-')
        {
            if(argv[i][1] == 'n')
            {
                opt_newline = 1;
            }
            else
            {
                fprintf(stderr, "Utilizare: %s [-n] fisier\n", argv[0]);
                exit(1);
            }
        }
        else
        {
            if(!file_name)
            {
                file_name = argv[i];
            }
            else
            {
                fprintf(stderr, "Utilizare: %s [-n] fisier\n", argv[0]);
                exit(1);
            }
            
        }
    }

    if(!file_name)
    {
        fprintf(stderr, "Utilizare: %s [-n] fisier\n", argv[0]);
        exit(1);
    }

    int d;
    if((d = open(file_name, O_RDWR | O_SYNC)) == -1)  //deschis doar pentru a afla lungimea sirului initial
    {
        perror(file_name);
        exit(1);
    }
    off_t l = 0;
    off_t m;
    off_t r;
    off_t len;
    
    if(lseek(d, (off_t)0, SEEK_END) == 0) //offsetul este -1 ca sa putem citi ultimul caracter
    {
    	close(d);
        return 0;
    }
    
    if((r = lseek(d, (off_t)-1, SEEK_END)) == -1) //offsetul este -1 ca sa putem citi ultimul caracter
    {
        perror("main(lseek)");
        exit(1);
    }
    len = r + 1; //lungimea fisierului

    char c;
    if(read(d, &c, 1) == -1)
    {
        perror("main(read)");
        exit(1);
    }

    if(c == '\n' && !opt_newline) //nu se ia in considerare \n adaugat de editoarele text la sfarsitul fisierului
    {
        r--;
    }
    opt_newline = len - r; //folosit mai tarziu pentru a verifica daca este procesul initial

    if(close(d) == -1)
    {
        perror("main(close)");
        exit(1);
    }

    while(l < r)
    {
        if((d = open(file_name, O_RDWR | O_SYNC)) == -1) //obtine un descriptor pentru fisier
        {
            perror(file_name);
            exit(1);
        }

        m = l + (r - l) / 2;
        
        if(fork())
        {
            if(fork())
            {
                //asteapta cei doi fii, apoi interclaseaza subsirurile
                wait(NULL);
                wait(NULL);
                merge(d, len, l, r);
                if(l == 0 && r + opt_newline == len) //verifica daca este procesul initial
                {
                    ftruncate(d, len);
                }
                close(d); //inchide descriptorul
                break;
            }
            else
            {
                if(close(d) == -1) //inchide descriptorul mostenit de la tata
                {
                    perror("fiu drept");
                    exit(1);
                }

                l = m + 1;
            } 
        }
        else
        {
            if(close(d) == -1) //inchide descriptorul mostenit de la tata
            {
                perror("fiu stang");
                exit(1);
            }

            r = m;
        }
    }

    return 0;
}

void merge(int d, off_t len, off_t l, off_t r)
{
    off_t m = l + (r - l) / 2;

    off_t n1 = m - l + 1; //nr de caractere din jumatatea stanga
    off_t n2 = r - m; //nr de caractere din jumatatea dreapta

    off_t i;

    const size_t buff_len = 1; //modifica dimensiunea bufferului de citire/scriere
    char buffer[buff_len];
    
    size_t read_len = buff_len;

    for(i = 0; i < n1 + n2; i += read_len)
    {
        if(lseek(d, l + i, SEEK_SET) == -1) //muta indicatorul la pozitia curenta de unde trebuie citit
        {
            perror("merge(lseek)");
            exit(1);    
        }

        if((read_len = read(d, buffer, buff_len)) == -1) // citeste cate buff_len bytes
        {
            perror("merge(read)");
            exit(1);
        }

        if(lseek(d, len + l + i, SEEK_SET) == -1) //muta indicatorul la pozitia curenta unde trebuie scris (l bytes fata de sfarsitul fisierului)
        {
            perror("merge(lseek)");
            exit(1);
        }

        if(write(d, buffer, read_len) == -1) //scrie cate read_len bytes
        {
            perror("merge(write)");
            exit(1);
        }
    
    }

    off_t j, k;
    i = j = k = 0;
    
    char c1, c2;

    while(i < n1 && j < n2) //interclaseaza cele doua subsiruri
    {
        if(lseek(d, len + l + i, SEEK_SET) == -1)
        {
            perror("merge(lseek)");
            exit(1);
        }

        if(read(d, &c1, 1) == -1)
        {
            perror("merge(read)");
            exit(1);
        }

        if(lseek(d, len + l + n1 + j, SEEK_SET) == -1)
        {
            perror("merge(lseek)");
            exit(1);
        }

        if(read(d, &c2, 1) == -1)
        {
            perror("merge(read)");
            exit(1);
        }

        if(lseek(d, l + k, SEEK_SET) == -1)
        {
            perror("merge(lseek)");
            exit(1);
        }
        
        if(c1 < c2)
        {
            if(write(d, &c1, 1) == -1)
            {
                perror("merge(write)");
                exit(1);
            }
            i++;
        }
        else
        {   
            if(write(d, &c2, 1) == -1)
            {
                perror("merge(write)");
                exit(1);
            }
            j++;
        }
        
        k++;
    }

    //adauga restul de elemente
    while(i < n1)
    {
        if(lseek(d, len + l + i, SEEK_SET) == -1)
        {
            perror("merge(lseek)");
            exit(1);
        }

        if(read(d, &c1, 1) == -1)
        {
            perror("merge(read)");
            exit(1);
        }

        if(lseek(d, l + k, SEEK_SET) == -1)
        {
            perror("merge(lseek)");
            exit(1);
        }

        if(write(d, &c1, 1) == -1)
        {
            perror("merge(write)");
            exit(1);
        }
        i++;
        k++;
    }

    while(j < n2)
    {
        if(lseek(d, len + l + n1 + j, SEEK_SET) == -1)
        {
            perror("merge(lseek)");
            exit(1);
        }
        if(read(d, &c2, 1) == -1)
        {
            perror("merge(read)");
            exit(1);
        }
        
        if(lseek(d, l + k, SEEK_SET) == -1)
        {
            perror("merge(lseek)");
            exit(1);
        }
        if(write(d, &c2, 1) == -1)
        {
            perror("merge(write)");
            exit(1);
        }
        j++;
        k++;
    }
}
