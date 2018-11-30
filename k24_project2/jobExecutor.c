#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include "header.h"

pid_t *procids;//pinakas me process ids
int workersnum;//posoi workers uparxoun

void childkill(int signo,siginfo_t *siginfo,void *context){
    int i,worker;
    for(i=0;i<workersnum;i++){
        if(siginfo->si_pid == procids[i]){
            worker = i;
        }
    }
    procids[worker]=-1;//an ginei kill kapoios worker allazoume to process id tou se -1

}

int main(int argc , char *argv[] ){
    int w,i,wexists=0,dexists=0;
    char* filename;
    FILE *file;
    //struct mapentry* array;
    for(i=0;i<argc;i++){
        if(strcmp(argv[i],"-d")==0){
            filename=argv[i+1];
            dexists=1;
            continue;
        }
        if(strcmp(argv[i],"-w")==0){
            sscanf(argv[i+1],"%d",&w);
            if(w<=0){
                printf("w must be a positive number!\n");
                return -1;
            }
            wexists=1;
        }
    }
    if(dexists==0){
        printf("Give Argument -d!\n");
        return -1;
    }
    if(wexists==0){
        printf("Give Argument -w!\n");
        return -1;
    }

    file = fopen(filename,"r");
    if(!file){
        printf("Error reading file..\n");
        return -2;
    }

    int status,fd,fdr;
    int fds[w];
    int fdrs[w];
    char name[6];
    char *arr=NULL;
    pid_t childpid;
    procids = malloc(sizeof(pid_t)*w);
    workersnum=w;
    //******// forks
    //fd -> JobExec->worker
    //fdr -> worker -> JobExec
    for(i=0;i<w;i++){
        //ftiaxnoume fifos (2 gia ka8e worker)
        sprintf(name,"./f%d",i);
        mkfifo(name,0666);
        sprintf(name,"./rf%d",i);
        mkfifo(name,0666);

        if( ( childpid = fork() ) == 0){
            free(procids);
            sprintf(name,"./f%d",i);
            fd = open(name,O_RDONLY);
            if(fd == -1){
                perror("Error Opening File");
                exit(-3);
            }
            sprintf(name,"./rf%d",i);

            if((fdr = open(name,O_WRONLY))==-1){
                perror("Error Opening r file");
                exit(-3);
            }
            break;
        }
        procids[i] = childpid;
        sprintf(name,"./f%d",i);
        if((fd = open(name, O_WRONLY)) == -1){
            perror("Error parent opening");
        }
        sprintf(name,"./rf%d",i);
        if((fdr = open(name,O_RDONLY)) == -1){
            perror("Error parent r opening");
        }
        fds[i] = fd;
        fdrs[i] = fdr;
    }
    if(childpid == 0){ // worker
        Worker(fd,fdr,file);
    }
    else{ // jobExec
        i=0;
        int j=0;
        int res;
        DIR *pdir=NULL;
        char *rpath=NULL;
        struct dirent *pdirent=NULL;
        size_t length=0;
        size_t readsize=0;
        struct AllPaths* allpaths=NULL;
        struct AllPaths* temp_all=NULL;

        static struct sigaction act;

        struct stat st ={0};
        if(stat("log",&st)==-1)
            mkdir("log",0700);
        act.sa_sigaction = &childkill;
        act.sa_flags = SA_SIGINFO;
        sigfillset(&(act.sa_mask));

        if(sigaction(SIGCHLD,&act, NULL) < 0){
            perror("sigaction");
        }

        while(getline(&arr,&length,file) >= 0){
            arr[strlen(arr)-1] = '\0';//gia na mhn paroume to '\n'

            pdir = opendir(arr);
            if(pdir == NULL){
                printf("Cannot Open Directory %s\n",arr);
                continue;
            }
            while((pdirent = readdir(pdir)) != NULL){
                if(!strcmp(pdirent->d_name,".") || !strcmp(pdirent->d_name,".."))
                        continue;
                readsize = strlen(arr)+strlen(pdirent->d_name)+2;
                rpath = malloc(readsize);
                bzero(rpath,sizeof(rpath));
                strncpy(rpath,arr,strlen(arr));
                rpath[strlen(arr)] = '/';
                rpath[strlen(arr)+1] = '\0';
                strcat(rpath,pdirent->d_name);
                rpath[strlen(rpath)]='\0';

                //AllPaths update
                if(allpaths==NULL){
                    allpaths = malloc(sizeof(struct AllPaths));
                    allpaths->path = malloc(strlen(rpath)+1);
                    strcpy(allpaths->path,rpath);
                    allpaths->workerid = i;
                    allpaths->next=NULL;
                    temp_all = allpaths;
                }
                else{
                    temp_all->next = malloc(sizeof(struct AllPaths));
                    temp_all->next->path = malloc(strlen(rpath)+1);
                    strcpy(temp_all->next->path,rpath);
                    temp_all->next->workerid = i;
                    temp_all->next->next=NULL;
                    temp_all = temp_all->next;
                }

                res = write(fds[i],&readsize,sizeof(size_t));
                if(res == -1)
                    printf("write error\n\n");


                res = write(fds[i], rpath, readsize);
                if(res ==-1)
                    printf("write error2\n\n");
                i++;
                if(i==w)
                    i=0;
                free(rpath);
            }
            closedir(pdir);
        }
        free(arr);
        readsize=0;
        for(i=0;i<w;i++){
            res = write(fds[i],&readsize,sizeof(size_t));
            if(res==-1)
                perror("FAILED TO WRITE 0");
        }
        int check=0;
        int r;
        for(i=0;i<w;i++){
            r = read(fdrs[i], &check, sizeof(int));
            if(r==-1)
                perror("Read -1");
            else if(r==0)
                perror("EOF?");
            if(check!=-1){
                perror("Not Ok");
                exit(-2);
            }
            check=0;
        }

	while(1){
            char fun[512];
            char *q0;
            for(i=0;i<w;i++){
                if(procids[i]==-1){
                    temp_all = allpaths;
                    printf("Replacing Dead Worker...\n");

                    close(fds[i]);
                    close(fdrs[i]);

                    if((childpid = fork()) == 0){
                        sprintf(name,"./f%d",i);
                        fd = open(name,O_RDONLY);
                        if(fd == -1){
                            perror("Error Opening File");
                            exit(-3);
                        }
                        sprintf(name,"./rf%d",i);

                        if((fdr = open(name,O_WRONLY))==-1){
                            perror("Error Opening r file");
                            exit(-3);
                        }
                        FreeAllPaths(allpaths);
                        free(procids);
                        Worker(fd,fdr,file);
                        return 1;
                    }
                    else{

                        procids[i] = childpid;
                        sprintf(name,"./f%d",i);
                        if((fd = open(name, O_WRONLY)) == -1){
                            perror("Error parent opening");
                        }
                        sprintf(name,"./rf%d",i);
                        if((fdr = open(name,O_RDONLY)) == -1){
                            perror("Error parent r opening");
                        }
                        fds[i] = fd;
                        fdrs[i] = fdr;

                        while(temp_all!=NULL){
                            if(temp_all->workerid == i){
                                //give the new worker the paths;
                                readsize = strlen(temp_all->path)+1;
                                write(fds[i],&readsize,sizeof(size_t));
                                write(fds[i],temp_all->path,readsize);
                            }
                            temp_all = temp_all->next;
                        }
                        readsize = 0;
                        write(fds[i],&readsize,sizeof(size_t));
                        read(fdrs[i],&check,sizeof(int));
                        if(check!=-1){
                            perror("New Worker");
                        }
                    }
                }
            }
            printf("Do Something (/search q1 q2 -d X.. /wc /maxcount word /mincount word.. /exit)\n/");
            if(fgets(fun,sizeof(fun), stdin)==NULL){
                perror("fgets");
                if(errno == EINTR)
                    continue;
                continue;
            }
            q0 = strtok(fun," \n");
            if(!strcmp(q0,"search")){
                q0 = strtok(NULL,"\n");
                if(q0==NULL)
                    continue;
                for(i=0;i<w;i++){
                    if((res = write(fds[i],"SEARCH",7)) == -1)
                        perror("Search Write");
                    readsize = strlen(q0)+1;
                    res = write(fds[i],&readsize,sizeof(size_t));

                    res = write(fds[i],q0,readsize);
                }
                int wfailed=0;
                char *path=NULL;
                char *text;
                int sent;
                int success;
                for(i=0;i<w;i++){
                    r = read(fdrs[i],&success,sizeof(int));
                    if(success==-2){
                        wfailed++;
                        continue;
                    }
                    ////////

                    while(1){
                        r = read(fdrs[i],&readsize,sizeof(size_t));
                        if(readsize==0){
                            break;
                        }
                        path = malloc(readsize);
                        r = read(fdrs[i],path,readsize);
                        while(1){
                            r = read(fdrs[i],&sent,sizeof(int));
                            if(sent==-1)
                                break;
                            printf("Path: %s Line: %d Text: %s\n",path,sent,text = findText(path,sent));
                            if(text!=NULL)
                                free(text);
                        }
                        free(path);
                    }
                }
                printf("%d out of %d Workers Gave Answer.\n",w-wfailed,w);
                if(w==wfailed){
                    printf("Workers Couldn't Answer in Time or No words from the query existed\n");
                }
            }
            else if(!strcmp(q0,"exit")){
                sigdelset(&(act.sa_mask) , SIGCHLD);//RESTORE TO DEFAULT
                act.sa_sigaction = NULL;
                sigaction(SIGCHLD,&act,NULL);
                for(i=0;i<w;i++){
                    res = write(fds[i] , "EXIT",7);
                }
                //free(procids);
                break;
            }
            else if(!strcmp(q0,"wc")){
                int bytes=0,words=0,lines=0;
                int sumbytes=0,sumwords=0,sumlines=0;
                for(i=0;i<w;i++){
                    res = write(fds[i],"WC",7);
                }
                for(i=0;i<w;i++){
                    r = read(fdrs[i],&bytes,sizeof(int));
                    r = read(fdrs[i],&words,sizeof(int));
                    r = read(fdrs[i],&lines,sizeof(int));
                    sumbytes = sumbytes + bytes;
                    sumwords = sumwords + words;
                    sumlines = sumlines + lines;
                }
                printf("Sum of Bytes: %d\nSum of Words: %d\nSum of lines: %d\n",sumbytes,sumwords,sumlines);

            }
            else if(!strcmp(q0,"maxcount")){
                q0 = strtok(NULL," \n");
                if(q0==NULL){
                    printf("Give Word to maxcount!\n");
                    continue;
                }
                readsize = strlen(q0)+1;
                for(i=0;i<w;i++){
                    res = write(fds[i],"MAXCNT",7);
                    res = write(fds[i],&readsize,sizeof(size_t));
                    res = write(fds[i],q0,readsize);
                }
                int max;
                int maxarr[w];
                char *paths[w];
                char maxpath[64];
                bzero(maxpath,sizeof(maxpath));
                for(i=0;i<w;i++){
                    r = read(fdrs[i],&max,sizeof(int));
                    if(max==-1){
                        maxarr[i]=max;
                        paths[i]=NULL;
                        continue;
                    }
                    r = read(fdrs[i],&readsize,sizeof(size_t));
                    r = read(fdrs[i],maxpath,readsize);
                    maxarr[i] = max;
                    paths[i]=malloc(strlen(maxpath)+1);
                    strcpy(paths[i],maxpath);
                    bzero(maxpath,sizeof(maxpath));

                }
                max = -1;
                for(i=0;i<w;i++){
                    if(maxarr[i]!=-1 && max==-1){
                        max = maxarr[i];
                        strcpy(maxpath,paths[i]);
                    }
                    if(maxarr[i]>max){
                        bzero(maxpath,sizeof(maxpath));
                        max = maxarr[i];
                        strcpy(maxpath,paths[i]);
                    }
                }
                if(max==-1){
                    printf("Word does not exists\n");
                }
                else
                    printf("File: %s max: %d\n",maxpath,max);
                for(i=0;i<w;i++){
                    free(paths[i]);
                }
            }
            else if(!strcmp(q0,"mincount")){
                q0 = strtok(NULL," \n");
                if(q0==NULL){
                    printf("Give word to mincount!\n");
                    continue;
                }
                readsize = strlen(q0)+1;
                for(i=0;i<w;i++){
                    res = write(fds[i],"MINCNT",7);
                    res = write(fds[i],&readsize,sizeof(size_t));
                    res = write(fds[i],q0,readsize);
                }
                int min;
                int minarr[w];
                char *paths[w];
                char minpath[64];
                bzero(minpath,sizeof(minpath));
                for(i=0;i<w;i++){
                    r = read(fdrs[i],&min,sizeof(int));
                    if(min==-1){
                        minarr[i]=min;
                        paths[i]=NULL;
                        continue;
                    }
                    r = read(fdrs[i],&readsize,sizeof(size_t));
                    r = read(fdrs[i],minpath,readsize);
                    minarr[i] = min;
                    paths[i]=malloc(strlen(minpath)+1);
                    strcpy(paths[i],minpath);
                    bzero(minpath,sizeof(minpath));

                }
                min = -1;
                for(i=0;i<w;i++){
                    if(minarr[i]!=-1 && min==-1){
                        min = minarr[i];
                        strcpy(minpath,paths[i]);
                    }
                    if(minarr[i]!=-1 && minarr[i]<min){
                        bzero(minpath,sizeof(minpath));
                        min = minarr[i];
                        strcpy(minpath,paths[i]);
                    }
                }
                if(min==-1){
                    printf("Word does not exists\n");
                }
                else
                    printf("File: %s min: %d\n",minpath,min);
                for(i=0;i<w;i++){
                    free(paths[i]);
                }


            }

        }
        FreeAllPaths(allpaths);
        free(procids);

        for(i=0;i<w;i++){
            close(fds[i]);
            close(fdrs[i]);
        }
        for(i=0;i<w;i++){
            sprintf(name,"./f%d",i);
            unlink(name);
            sprintf(name,"./rf%d",i);
            unlink(name);
        }

        while(wait(&status)>0){
            if(WIFEXITED(status)){
                printf("child terminated with status: %d\n",WEXITSTATUS(status));
            }
        }
        fclose(file);
        exit(1);
    }

}



