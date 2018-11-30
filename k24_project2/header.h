extern int workersnum;
extern pid_t *procids;

struct Pathslist{ //gia wc
    char *path;
    int byte_num;
    int word_num;
    int line_num;
    struct Pathslist *next;
};

struct AllPaths{ //o job executor exei auth th domh gia na blepei poios worker exei poio path
    char *path;
    int workerid;
    struct AllPaths* next;
};

struct postinglist{
    char *path;
    struct pathCont *contains;
    struct postinglist *next;
};

struct pathCont{
    int text_id;
    int times_shown;
    struct pathCont *next;
};

struct TrieNode{
    char c;
    struct TrieNode* children;
    struct TrieNode* next;
    int isfinal;
    struct postinglist* p_list;
};


struct TrieNode* createNode(char);
void Worker(int,int,FILE *);
void insert(char *,struct TrieNode*,int ,char *);
void search(struct TrieNode*,char *,int,int);
char *findText(char *,int);
int countwords(char*);
void maxcount(struct TrieNode*,char *,int,int);
void mincount(struct TrieNode*,char *,int,int);
void FreeTrie(struct TrieNode*);
void FreePostList(struct postinglist*);
void FreeContains(struct pathCont*);
void FreePath_List(struct Pathslist*);
void FreeAllPaths(struct AllPaths*);
void childkill(int,siginfo_t*,void*);
