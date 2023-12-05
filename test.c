#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main()
{
    int lines = 0;
    int ch;
    struct player
    {
        char name[20];
        int score;
    };
    
    FILE* pt = fopen("leader_board.txt", "r");
    if (pt == NULL) {
        printf("no such file.\n");
        return 0;
    }
 
    /* Assuming that test.txt has content 
       in below format
    NAME AGE CITY
    abc     12 hyderbad
    bef     25 delhi
    cce     65 bangalore */
    char buf[100];
    char buf_2[100];
    char buf_3[100];
    while(!feof(pt))
    {
    ch = fgetc(pt);
    if(ch == '\n')
    {
        lines++;
    }
    }
    fclose(pt);
    FILE* ptr = fopen("leader_board.txt", "r+");
    if (ptr == NULL) {
        printf("no such file.\n");
        return 0;
    }
    printf("%d\n",lines);
    struct player p[lines + 1];
    int i = 0;
    fscanf(ptr, "%s",buf);
    fscanf(ptr, "%s",buf);
    while (fscanf(ptr, "%s",buf)== 1){
        fscanf(ptr, "%s",buf_2);
        strcpy(p[i].name,buf);
        p[i].score = atoi(buf_2);
        //printf("%s %d \n", p[i].name,p[i].score);
        i++;
    }    
    char name[20] = "test";
    int score = 80;
    strcpy(p[lines - 1].name,name);
    p[lines - 1].score = score;
    //fprintf(ptr, "%s %s\n", "NAME","SCORE")
    for(int j = 0; j < lines; j++){
        for(int k = 0; k < lines - 1; k++){
            if(p[k].score < p[k + 1].score){
                struct player temp = p[k];
                p[k] = p[k + 1];
                p[k + 1] = temp;
            }
        }
    }
    for(int j = 0; j < lines; j++){
        //fprintf(ptr, "%s %d\n", p[i].name,p[i].score);
        printf("%s %d \n", p[j].name,p[j].score);
    }
    fclose(ptr);

    FILE* ptt = fopen("leader_board.txt", "w");
    if (ptt == NULL) {
        printf("no such file.\n");
        return 0;
    }
    fprintf(ptt, "%s %s\n", "NAME","SCORE");
    for(int j = 0; j < lines; j++){
        fprintf(ptt, "%s %d\n", p[j].name,p[j].score);
        //printf("%s %d \n", p[j].name,p[j].score);
    }
    
    return 0;
}