#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include "header.h"

int global=0;

struct TrieNode* createNode(char c){
    struct TrieNode* node = malloc(sizeof(struct TrieNode));
    node->c = c;
    node->children = NULL;
    node->next = NULL;
    node->isfinal = 0;
    node->p_list = NULL;
    return node;
}

void Worker(int fd,int fdr,FILE *file){
        int c,i,j=0;
        size_t readsize=0;
        struct Pathslist* head,*curr;

        char pathread[128];
        head = NULL;
        while(1){
            int r = read(fd,&readsize,sizeof(size_t));
            if(readsize==0){
                break;
            }
            r = read(fd,pathread,readsize);
            if(r == -1){
                perror("READ");
                exit(-1);
            }
            else if(r == 0){
                printf("eof\n");
            }
            if(head==NULL){
                head = malloc(sizeof(struct Pathslist));
                head->next=NULL;
                head->byte_num = 0;
                head->word_num = 0;
                head->line_num = 0;
                head->path = malloc(strlen(pathread)+1);
                strcpy(head->path,pathread);
                curr = head;
            }
            else{
                curr->next = malloc(sizeof(struct Pathslist));
                curr->next->path = malloc(strlen(pathread)+1);
                curr->next->byte_num = 0;
                curr->next->word_num = 0;
                curr->next->line_num = 0;
                strcpy(curr->next->path,pathread);
                curr->next->next = NULL;
                curr = curr->next;
            }
            j++;
        }
        curr = head;
        //sto head exoume lista apo ta paths tou worker



        struct TrieNode* trie;
        trie = createNode('$');
        FILE* pfile;
        while(curr!=NULL){//gia ka8e arxeio
            pfile = fopen(curr->path,"r");
            if(!pfile)
                printf("FAIL TO READ\n");
            //metrame poses grammes exei to ka8e arxeio
            c=fgetc(pfile);
            int numline=0;
            while(c!=EOF){
                if(c=='\n'){
                    numline++;
                }
                c=fgetc(pfile);
            }
            rewind(pfile);
            //apo8hkeuoume ari8mo xarakthrwn ka8e grammhs
            int *Num_of_lines=malloc(sizeof(int)*numline);
            curr->line_num = curr->line_num + numline;

            c=fgetc(pfile);
            int numchars=1;
            numline = 0;
            while(c!=EOF){
                if(c=='\n'){
                    Num_of_lines[numline]=numchars-1;// -1 gia na mhn paroume pio meta to \n
                    curr->byte_num = curr->byte_num + numchars;
                    numchars=0;
                    numline++;
                }
                c=fgetc(pfile);
                numchars++;
            }

            rewind(pfile);
            int offset=0;
            char *word;
            for(i=0;i<numline;i++){
                char line[Num_of_lines[i]];
                fseek(pfile,offset,SEEK_SET);
                fgets(line,Num_of_lines[i]+1,pfile);
                word = strtok(line," \n\t");
                while(word!=NULL){
                    curr->word_num++;
                    insert(word,trie,i,curr->path);
                    word = strtok(NULL," \t\n");
                }
                offset=offset+Num_of_lines[i]+1;
            }

            free(Num_of_lines);
            fclose(pfile);
            curr = curr->next;
        }
        //FreePath_List(head);
        int check=-1;
        int res;
        //stelnei mhnuma oti teleiwse thn dhmiourgia tou trie
        if((res = write(fdr,&check,sizeof(int))) == -1)
            perror("WRITE Ok");
        fclose(file);

        char comm[7];
        bzero(comm,sizeof(comm));
        int r;
        char logname[20];
        sprintf(logname,"log/Worker_%d",getpid());
        int fdlog;
        time_t rawtime;
        struct tm* timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);

        fdlog = open(logname, O_WRONLY | O_CREAT, 0666);

        while(1){
            //perimenei entolh apo jobExecutor
            if((r = read(fd,comm,7)) == -1){
                perror("Read Command");
            }
            if(!strcmp(comm,"SEARCH")){
                r = read(fd,&readsize,sizeof(size_t));
                char query[readsize];
                bzero(query,sizeof(query));
                r = read(fd,query,readsize);
                search(trie,query,fdr,fdlog);

            }
            else if(!strcmp(comm,"EXIT")){
                FreeTrie(trie);
                FreePath_List(head);
                close(fdlog);
                exit(0);
            }
            else if(!strcmp(comm,"MAXCNT")){
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                write(fdlog,asctime(timeinfo),strlen(asctime(timeinfo))-1);//dont take \n
                write(fdlog," : Maxcount : ",14);

                r = read(fd,&readsize,sizeof(size_t));
                char *query = malloc(readsize);
                r = read(fd,query,readsize);

                write(fdlog,query,readsize-1);

                maxcount(trie,query,fdr,fdlog);
                write(fdlog,"\n",1);
                free(query);

            }
            else if(!strcmp(comm,"MINCNT")){
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                write(fdlog,asctime(timeinfo),strlen(asctime(timeinfo))-1);//dont take \n
                write(fdlog," : Mincount : ",14);

                r = read(fd,&readsize,sizeof(size_t));
                char *query = malloc(readsize);
                r = read(fd,query,readsize);

                write(fdlog,query,readsize-1);

                mincount(trie,query,fdr,fdlog);
                write(fdlog,"\n",1);
                free(query);

            }
            else if(!strcmp(comm,"WC")){
                //sto log o worker grafei mono ta dika tou bytes/words/lines.. o jobExecutor ta pros8etei
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                write(fdlog,asctime(timeinfo),strlen(asctime(timeinfo))-1);
                write(fdlog," : WC : ",8);
                int bytes=0,words=0,lines=0;
                curr = head;
                while(curr!=NULL){
                    bytes = bytes + curr->byte_num;
                    words = words + curr->word_num;
                    lines = lines + curr->line_num;
                    curr = curr->next;
                }
                res = write(fdr,&bytes,sizeof(int));
                write(fdlog,"Bytes ",6);
                char nums[25];
                bzero(nums,sizeof(nums));
                sprintf(nums,"%d",bytes);
                write(fdlog,nums,strlen(nums));
                res = write(fdr,&words,sizeof(int));
                write(fdlog," : Total_Words ",15);
                bzero(nums,sizeof(nums));
                sprintf(nums,"%d",words);
                write(fdlog,nums,strlen(nums));

                res = write(fdr,&lines,sizeof(int));
                write(fdlog," : Total_Lines ",15);
                bzero(nums,sizeof(nums));
                sprintf(nums,"%d",lines);
                write(fdlog,nums,strlen(nums));

                write(fdlog,"\n",1);

            }
        }
        close(fdr);
        exit(0);
}

void insert(char * word,struct TrieNode* trie,int id,char *path){
    struct TrieNode* curr = trie;
    int i;
    if(global==0){//thn prwth fora pou ginetai insert 8a prepei na mpei edw, tis upoloipes oxi

        curr->c = word[0];
        if(strlen(word)==1)
            curr->isfinal=1;
        else
            curr->isfinal=0;

        for(i=1;i<strlen(word);i++){
            curr->children = createNode(word[i]);
            curr = curr->children;
            if(i==strlen(word)-1){
                curr->isfinal=1;
                curr->p_list = malloc(sizeof(struct postinglist));
                if(!curr->p_list)
                    return;
                curr->p_list->contains = malloc(sizeof(struct pathCont));
                curr->p_list->path = malloc(strlen(path)+1);
                strcpy(curr->p_list->path,path);
                curr->p_list->contains->times_shown = 1;
                curr->p_list->contains->text_id = id;
                curr->p_list->contains->next=NULL;
                curr->p_list->next = NULL;
            }
            else
                curr->isfinal=0;
        }
        global=1;
    }
    else{
        i=0;
        int creating=0;
        while(i<strlen(word)){
            if(curr->c==word[i]){
                if(i!=strlen(word)-1){//an h leksh uparxei hdh mhn pas sto children
                    if(curr->children!=NULL)
                        curr = curr->children;
                    else{//an den exei ka8olou children ftiakse
                        creating = 1;
                        i++;
                        break;
                    }
                }
                i++;
            }
            else if(curr->next!=NULL)
                curr = curr->next;
            else{
                struct TrieNode* new;
                new = createNode(word[i]);
                curr->next = new;
                curr = curr->next;
                if(i==strlen(word)-1){//leksh 1os grammatos
                    curr->isfinal=1;
                    curr->p_list = malloc(sizeof(struct postinglist));
                    if(!curr->p_list)
                        return;
                    curr->p_list->contains = malloc(sizeof(struct pathCont));
                    curr->p_list->contains->text_id = id;
                    curr->p_list->contains->times_shown = 1;
                    curr->p_list->path = malloc(strlen(path)+1);
                    strcpy(curr->p_list->path,path);

                    curr->p_list->contains->next = NULL;
                    curr->p_list->next = NULL;
                }
                i++;
                creating=1;
                break;
            }

        }
        if(creating==1){
            while(i<strlen(word)){
                curr->children = createNode(word[i]);
                curr = curr->children;
                if(i==strlen(word)-1){
                    curr->isfinal=1;
                    curr->p_list = malloc(sizeof(struct postinglist));
                    if(!curr->p_list)
                        return;

                    curr->p_list->contains = malloc(sizeof(struct pathCont));
                    curr->p_list->contains->text_id = id;
                    curr->p_list->contains->times_shown = 1;
                    curr->p_list->path = malloc(strlen(path)+1);
                    strcpy(curr->p_list->path,path);

                    curr->p_list->contains->next = NULL;
                    curr->p_list->next = NULL;
                }
                else
                    curr->isfinal=0;
                i++;
            }
        }
        else{//uparxei hdh sto trie h leksh
            curr->isfinal=1;
            if(curr->p_list == NULL){
                curr->p_list = malloc(sizeof(struct postinglist));
                if(!curr->p_list)
                    return;

                curr->p_list->contains = malloc(sizeof(struct pathCont));
                curr->p_list->contains->text_id = id;
                curr->p_list->contains->times_shown = 1;
                curr->p_list->path = malloc(strlen(path)+1);
                strcpy(curr->p_list->path,path);

                curr->p_list->contains->next = NULL;
                curr->p_list->next = NULL;

            }
            else{
                struct postinglist* temp = curr->p_list;
                //to posting list einai diaforetiko apo thn 1h ergasia
                while(temp!=NULL){
                    if(!strcmp(temp->path,path)){
                        struct pathCont *tempcont = temp->contains;
                        struct pathCont *tempconthelper = temp->contains;
                        while(tempcont!=NULL){
                            tempconthelper = tempcont;
                            if(tempcont->text_id == id){
                                tempcont->times_shown++;
                                return;
                            }
                            else
                                tempcont = tempcont->next;
                        }
                        //brhke idio path alla oxi idio id
                        tempconthelper->next = malloc(sizeof(struct pathCont));
                        tempconthelper->next->text_id = id;
                        tempconthelper->next->times_shown = 1;
                        tempconthelper->next->next = NULL;
                        return;
                    }
                    else
                        temp = temp->next;

                }

                temp = curr->p_list;
                while(temp->next!=NULL){
                    temp=temp->next;
                }
                temp->next = malloc(sizeof(struct postinglist));
                if(!temp->next)
                    return;

                temp->next->contains = malloc(sizeof(struct pathCont));
                temp->next->contains->text_id = id;
                temp->next->contains->times_shown = 1;
                temp->next->path = malloc(strlen(path)+1);
                strcpy(temp->next->path,path);

                temp->next->contains->next = NULL;
                temp->next->next = NULL;



            }
        }
    }
}

void search(struct TrieNode* trie,char *query,int fdr,int fdlog){
    struct TrieNode* curr;
    char *q0=NULL;
    int i,fail=-2;
    float deadline;
    int wcount,wcounter=0;
    int updated=0;
    char **words;
    char *querycopy;
    time_t rawtime;
    struct tm* timeinfo;
    querycopy=malloc(strlen(query)+1);
    strcpy(querycopy,query);
    wcount = countwords(query);
    words = malloc(sizeof(char*)*wcount);
    wcounter=0;
    q0 = strtok(querycopy," \t\n");
    //briskoume to deadline
    while(q0!=NULL){
        if(!strcmp(q0,"-d")){
            q0 = strtok(NULL," \t\n");
            if(q0!=NULL){
                sscanf(q0,"%f",&deadline);
                updated = 1;
                break;
            }
        }
        q0 = strtok(NULL," \t\n");
    }
    if(updated==0){
        printf("Wrong -d argument!\n");
        write(fdr,&fail,sizeof(int));
        free(querycopy);
        free(words);
        return;

    }
    free(querycopy);
    clock_t start = clock();
    clock_t end;
    struct postinglist *search_results;
    search_results = malloc(sizeof(struct postinglist));
    search_results->path = NULL;
    search_results->contains = NULL;
    search_results->next = NULL;
    //8a apo8hkeusoume ta apotelesmata apo oles tis lekseis se 1 domh tupou postinglist (path , contains , next) opou contains:lista apo (line,times_shown)

    q0 = strtok(query," \t\n");
    while(q0!=NULL  && strcmp(q0,"-d")){
        words[wcounter] = q0;
        wcounter++;
        i=0;
        curr = trie;
        while(i<strlen(q0)){
            if(curr->c==q0[i]){
                if(i!=strlen(q0)-1){//an einai sto teleutaio gramma mhn metakinh8ei sto children
                    if(curr->children!=NULL){
                        curr = curr->children;
                        i++;
                    }
                    else{
                        //printf("not found '%s' :(\n",q0);
                        time(&rawtime);
                        timeinfo = localtime(&rawtime);
                        write(fdlog,asctime(timeinfo),strlen(asctime(timeinfo))-1);
                        write(fdlog," : Search : ",12);
                        write(fdlog,q0,strlen(q0));
                        write(fdlog,"\n",1);
                        wcounter--;
                        break;
                    }
                }
                else{
                    if(curr->isfinal==0){
                        //printf("not found '%s' :(\n",q0);
                        time(&rawtime);
                        timeinfo = localtime(&rawtime);
                        write(fdlog,asctime(timeinfo),strlen(asctime(timeinfo))-1);
                        write(fdlog," : Search : ",12);
                        write(fdlog,q0,strlen(q0));
                        write(fdlog,"\n",1);
                        wcounter--;
                        break;
                    }
                    //edw briskoume thn leksh q sto trie(to teleutaio ths gramma)
                    struct postinglist *list;
                    list = curr->p_list;

                    time(&rawtime);
                    timeinfo = localtime(&rawtime);
                    write(fdlog,asctime(timeinfo),strlen(asctime(timeinfo))-1);
                    write(fdlog," : Search : ",12);
                    write(fdlog,q0,strlen(q0));

                    while(list!=NULL){
                        write(fdlog," : ",3);
                        write(fdlog,list->path,strlen(list->path));
                        if(search_results->path==NULL){
                            search_results->path = malloc(strlen(list->path)+1);
                            strcpy(search_results->path,list->path);

                            struct pathCont* cont;
                            struct pathCont *temp = search_results->contains;
                            cont = list->contains;
                            while(cont!=NULL){
                                if(search_results->contains==NULL){
                                    search_results->contains = malloc(sizeof(struct pathCont));
                                    search_results->contains->times_shown = cont->times_shown;
                                    search_results->contains->text_id = cont->text_id;
                                    search_results->contains->next = NULL;
                                    temp = search_results->contains;
                                }
                                else{
                                    temp->next = malloc(sizeof(struct pathCont));
                                    temp->next->times_shown = cont->times_shown;
                                    temp->next->text_id = cont->text_id;
                                    temp->next->next = NULL;
                                    temp = temp->next;
                                }
                                cont = cont->next;
                            }
                        }
                        else{
                            struct postinglist *temph,*temp = search_results;
                            struct pathCont* cont = list->contains;
                            int found = 0;
                            while(temp!=NULL){
                                if(!strcmp(temp->path,list->path)){
                                    found=1;
                                //brhkame to idio path
                                    break;
                                }
                                temph=temp;
                                temp = temp->next;
                            }
                            if(found==0){//create new node
                                temph->next = malloc(sizeof(struct postinglist));
                                temph->next->path = malloc(strlen(list->path)+1);
                                temph->next->contains = NULL;
                                temph->next->next = NULL;
                                strcpy(temph->next->path,list->path);
                                struct pathCont *tempcont = temph->next->contains;
                                while(cont!=NULL){
                                    if(temph->next->contains==NULL){
                                        temph->next->contains = malloc(sizeof(struct pathCont));
                                        temph->next->contains->text_id = cont->text_id;
                                        temph->next->contains->times_shown = cont->times_shown;
                                        temph->next->contains->next = NULL;
                                        tempcont = temph->next->contains;
                                    }
                                    else{
                                        tempcont->next = malloc(sizeof(struct pathCont));
                                        tempcont->next->times_shown = cont->times_shown;
                                        tempcont->next->text_id = cont->text_id;
                                        tempcont->next->next = NULL;
                                        tempcont = tempcont->next;
                                    }
                                    cont = cont->next;
                                    //tempcont = tempcont->next;
                                }

                            }
                            else if(found==1){//an brhkame to idio path update
                                struct pathCont *tempconth,*tempcont=temp->contains;
                                int found=0;
                                while(cont!=NULL){
                                    while(tempcont!=NULL){
                                        if(tempcont->text_id == cont->text_id){
                                            //do nothing
                                            tempcont = temp->contains;
                                            found = 1;
                                            break;
                                        }
                                        tempconth = tempcont;
                                        tempcont = tempcont->next;
                                    }
                                    if(found==0){
                                        tempconth->next=malloc(sizeof(struct pathCont));
                                        tempconth->next->text_id = cont->text_id;
                                        tempconth->next->times_shown = cont->times_shown;
                                        tempconth->next->next=NULL;
                                    }
                                    cont=cont->next;
                                }
                            }
                        }
                        end = clock();
                        float seconds = (float)(end-start) / CLOCKS_PER_SEC;
                        if(seconds>deadline){
                            //check an einai mesa ston xrono deadline
                           //printf("Not In Time..\n");
                           write(fdlog,"\nTimeout\n",9);
                           write(fdr,&fail,sizeof(int));
                           FreePostList(search_results);
                           free(words);
                           return;
                        }
                        list = list->next;
                    }
                    write(fdlog,"\n",1);
                    list = curr->p_list;
                    i++;
                    break;
                }
            }
            else if(curr->next!=NULL){
                curr = curr->next;
            }
            else{
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                write(fdlog,asctime(timeinfo),strlen(asctime(timeinfo))-1);
                write(fdlog," : Search : ",12);
                write(fdlog,q0,strlen(q0));
                write(fdlog,"\n",1);
                wcounter--;
                break;
            }
        }
        q0 = strtok(NULL," \t\n");
        curr = trie;
        if(wcounter==10){
            printf("Can't search more than 10 words! Search for the first 10 that exists\n");
            break;
        }
    }

    if(wcounter==0){
        //printf("No words found from the query\n");
        write(fdr,&fail,sizeof(int));
        free(words);
        FreePostList(search_results);
        return;
    }


    //prolabe na brei apotelesmata mesa sto deadline.. ta stelnei
    int success=1;
    int stop=0;
    int changepath=-1;
    //fail == -2
    size_t readsize;
    int res = write(fdr,&success,sizeof(int));//stelnei oti prolabe
    struct postinglist* printer=search_results;
    struct pathCont* printercont;
    while(printer!=NULL){ //stelnei ta apotelesmata ths search pou apo8hkeusame
        readsize = strlen(printer->path)+1;
        res = write(fdr,&readsize,sizeof(size_t));
        res = write(fdr,printer->path,readsize);
        printercont = printer->contains;
        while(printercont!=NULL){
            res = write(fdr,&(printercont->text_id),sizeof(int));
            printercont = printercont->next;
        }
        res = write(fdr,&changepath,sizeof(int));
        printer = printer->next;
    }
    res = write(fdr,&stop,sizeof(int));
    FreePostList(search_results);
    //free words
    free(words);
    return;

}

char * findText(char * path,int text_id){
    FILE *file = fopen(path,"r");
    size_t length=0;
    char *arr=NULL;
    int id=0;
    while(getline(&arr,&length,file)!= -1){
        arr[strlen(arr)-1] = '\0';
        if(id == text_id){
            fclose(file);
            return arr;
        }
        id++;
    }
    free(arr);
    return NULL;
}

int countwords(char *sentence){
    int i=0;
    int countwords=0;
    while(i<strlen(sentence)){
        if(sentence[i]!=' ' && sentence[i]!='\t'){
            countwords++;
            while(sentence[i]!=' ' && sentence[i]!='\t'){
                i++;
                if(i==strlen(sentence))
                    break;

            }
        }
        else{
            i++;
        }

    }
    return countwords;
}

void maxcount(struct TrieNode* trie,char *q0,int fdr,int fdlog){
    struct TrieNode* curr;
    int i,j,max;
    i=0;
    curr = trie;
    while(i<strlen(q0)){
        if(curr->c==q0[i]){
            if(i!=strlen(q0)-1){//an einai sto teleutaio gramma mhn metakinh8ei sto children
                if(curr->children!=NULL){
                    curr = curr->children;
                    i++;
                }
                else{
                    //printf("not found '%s' :(\n",q0);
                    max = -1;
                    int res = write(fdr,&max,sizeof(int));
                    return;
                }
            }
            else{
                if(curr->isfinal==0){
                    //printf("not found '%s' :(\n",q0);
                    max = -1;
                    int res = write(fdr,&max,sizeof(int));

                    return;
                }
                //edw briskoume thn leksh q sto trie(to teleutaio ths gramma)
                struct postinglist *list;
                list = curr->p_list;
                struct pathCont *cont;
                int counter;
                max=-1;//akurh timh
                char *path=NULL;
                while(list!=NULL){
                    //anazhthsh sto posting list.. epistrofh tou path me to max
                    write(fdlog," : ",3);
                    write(fdlog,list->path,strlen(list->path));
                    cont = list->contains;
                    counter = 0;
                    while(cont!=NULL){
                        counter+=cont->times_shown;
                        cont = cont->next;
                    }
                    if(max<counter){
                        max = counter;
                        path = list->path;
                    }
                    else if(max==counter){
                        if(strcmp(path,list->path) > 0){ //alfabhtika mikrotero
                            max = counter;
                            path = list->path;
                        }
                    }

                    list = list->next;
                }
                //printf("file: %s max:%d\n",path,max);
                int res = write(fdr,&max,sizeof(int));
                size_t readsize=strlen(path)+1;
                res = write(fdr,&readsize,sizeof(size_t));
                res = write(fdr,path,readsize);
                return;
            }
        }
        else if(curr->next!=NULL){
            curr = curr->next;
        }
        else{
            //printf("not found '%s' :(\n",q0);
            max = -1;
            int res = write(fdr,&max,sizeof(int));

            return;
        }
    }
    curr = trie;
    //free words
    return;

}

void mincount(struct TrieNode* trie,char *q0,int fdr,int fdlog){
    struct TrieNode* curr;
    int i,min;
    i=0;
    curr = trie;
    while(i<strlen(q0)){
        if(curr->c==q0[i]){
            if(i!=strlen(q0)-1){//an einai sto teleutaio gramma mhn metakinh8ei sto children
                if(curr->children!=NULL){
                    curr = curr->children;
                    i++;
                }
                else{
                    //printf("not found '%s' :(\n",q0);
                    min = -1;
                    int res = write(fdr,&min,sizeof(int));
                    return;
                }
            }
            else{
                if(curr->isfinal==0){
                    //printf("not found '%s' :(\n",q0);
                    min = -1;
                    int res = write(fdr,&min,sizeof(int));

                    return;
                }
                //edw briskoume thn leksh q sto trie(to teleutaio ths gramma)
                struct postinglist *list;
                list = curr->p_list;
                struct pathCont *cont;
                int counter;
                min=-1;//akurh timh
                char *path=NULL;
                while(list!=NULL){
                    //anazhthsh sto posting list.. epistrofh tou path me to min
                    write(fdlog," : ",3);
                    write(fdlog,list->path,strlen(list->path));

                    cont = list->contains;
                    counter = 0;
                    while(cont!=NULL){
                        counter+=cont->times_shown;
                        cont = cont->next;
                    }
                    if(min==-1){
                        min = counter;
                        path = list->path;
                    }
                    if(min>counter){
                        min = counter;
                        path = list->path;
                    }
                    else if(min==counter){
                        if(strcmp(path,list->path) > 0){//mikrotero alfabhtika
                            min = counter;
                            path = list->path;
                        }
                    }

                    list = list->next;
                }
                //printf("file: %s max:%d\n",path,max);
                int res = write(fdr,&min,sizeof(int));
                size_t readsize=strlen(path)+1;
                res = write(fdr,&readsize,sizeof(size_t));
                res = write(fdr,path,readsize);

                return;
            }
        }
        else if(curr->next!=NULL){
            curr = curr->next;
        }
        else{
            //printf("not found '%s' :(\n",q0);
            min = -1;
            int res = write(fdr,&min,sizeof(int));

            return;
        }
    }
    curr = trie;
    //free words
    return;

}

void FreeTrie(struct TrieNode *trie){
    struct TrieNode *curr = trie;
    if(curr->children!=NULL){
        FreeTrie(curr->children);
    }
    if(curr->isfinal==1){
        FreePostList(curr->p_list);
    }
    if(curr->next!=NULL){
        FreeTrie(curr->next);
    }
    free(curr);

}

void FreePostList(struct postinglist* list){
    struct postinglist *curr = list;
    struct postinglist *temp;
    while(curr!=NULL){
        temp = curr;
        curr = curr->next;
        if(temp->path!=NULL)
            free(temp->path);
        FreeContains(temp->contains);
        free(temp);
    }
    return;
}

void FreeContains(struct pathCont* cont){
    struct pathCont *curr,*temp;
    curr = cont;
    while(curr!=NULL){
        temp = curr;
        curr = curr->next;
        free(temp);
    }
}

void FreePath_List(struct Pathslist* list){
    struct Pathslist *curr,*temp;
    curr = list;
    while(curr!=NULL){
        temp = curr;
        curr = curr->next;
        free(temp->path);
        free(temp);
    }

}

void FreeAllPaths(struct AllPaths* list){
    struct AllPaths *curr,*temp;
    curr = list;
    while(curr!=NULL){
        temp = curr;
        curr = curr->next;
        free(temp->path);
        free(temp);
    }
}
