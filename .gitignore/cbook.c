#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <locale.h>
#include <curl/curl.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <wchar.h>
#include <stdarg.h>
#include <json-c/json.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>         
#include <sys/socket.h>        
#include <netinet/in.h>
#include <arpa/inet.h>  
#include <signal.h>  


CURL *curl = NULL;
CURLcode res;
char result[200];
char *cookieF = "cookie\0";
char userAgent[100] = "Mozilla/5.0 (X11; Ubuntu; Linux i686; rv:45.0) Gecko/20100101 Firefox/45.0";
char *userid;
char *usercode;
int lscreenStop;
pthread_t pth;
pthread_t pthUpd;
int midd;

//### ### ### String utils ### ### ###
char *charAt(char *str, unsigned long long int index){
    char *current = (char *) malloc(sizeof(char)*2);
    *current = *(str + index);
    current[1] = '\0';
    
    return current;
}

char *substring(char *str, unsigned long long int min, unsigned long long int max){
    unsigned long long length = strlen(str);
    if(max>length){
        max = length-1;
    }
    int memo = (sizeof(char)*(max-min)+2);
    
    char *character = (char *) malloc(sizeof(char));
    char *cutted = (char *) calloc(1, memo);
    
    for(unsigned long long a = min; a<=max; a++){
        *character = *(str + a);
        character[1] = '\0';
        strcat(cutted, character);
    }
    
    character = (char *) malloc(0);
    return cutted;
}
//##################### STRUCT E COMPAGNI ######################
typedef struct 
{
    char *fib;
    char *name;
    char *surname;
    int loaded;
    int list;
    int update;
}Names;

typedef struct 
{
    char *sender;
    char *receiver;
    char *message;
    char *time;
}Messages;

int positionN = 0;
int positionM = 0;
unsigned long sizeNames;
unsigned long sizeMessages;

Names *n;
Messages *m;

void initStructs(){
    int alpha = 25;
    
    n = (Names *) malloc(alpha);
    m = (Messages *) malloc(alpha);
    
    sizeNames = alpha;
    sizeMessages = alpha;
    midd = 0;
    positionN = 0;
    positionM = 0;
}

int getsizeP(const char *phrase){
    if(phrase)
        return (strlen(phrase)*sizeof(char));
    else 
        return 0;
}

int getsizeI(const char *number){
    return sizeof(number);
}

int findUserPosition(char *tofind){
    int a = 0;
    do{
        if(strcmp(tofind, n[a].fib) == 0){
            return a;
        }
        a++;
    }while(a<positionN);
    return -1;
}

//                NOME COMPLETO| solo il nome       | facebook Id
void growNames(const char *name, const char *surname, const char *fib){
    if(fib != 0 && strcmp(name, "") != 0 && strcmp(surname, "") != 0 && strcmp(name, "Utente Facebook") != 0){
        int sizeN = getsizeP(name)+1;
        int sizeS = getsizeP(surname)+1;
        int sizeF = getsizeP(fib)+1;
        int sizeU = sizeof(int);
        
        if(sizeN<4){
            sizeN = 4;   
        }if(sizeS<4){
            sizeS = 4;
        }if(sizeF<4){
            sizeF = 4;
        }
        
        int totalSize =  sizeN + sizeS + sizeF + sizeU;
        sizeNames += totalSize;
        
        n = (Names *) realloc(n, sizeNames);
        
        n[positionN].name = (char *) calloc(1, sizeN);
        n[positionN].surname = (char *) calloc(1, sizeS);
        n[positionN].fib = (char *) calloc(1, sizeF);
        n[positionN].loaded = 0;
        n[positionN].list = -1;
        n[positionN].update = 0;
        
        char *fibu = (char *) malloc(sizeof(char)*strlen(fib)+1);
        strcpy(fibu, fib);
        
        int isPresent = findUserPosition(fibu);
        
        if(isPresent == -1){
            strcat(n[positionN].fib, fib);
            strcat(n[positionN].name, name);
            strcat(n[positionN].surname, surname);
            positionN++;
        }        
    }
}

void growMessages(const char *sender, const char *receiver, const char *message, const char *time){
    if(message && message[0]){
        int sizeS = getsizeP(sender)+1;
        int sizeR = getsizeP(receiver)+1;
        int sizeM = getsizeP(message)+1;
        int sizeT = getsizeP(time)+1;
        
        if(sizeR<4){
            sizeR = 4;   
        }if(sizeS<4){
            sizeS = 4;
		}if(sizeM<4)
            sizeM = 4;
        }if(sizeT<4){
            sizeT = 4;
        }
        
        int totalSize = sizeS + sizeR + sizeM + sizeT;
        sizeMessages += totalSize;
        
        m = (Messages *) realloc(m, sizeMessages);
        
        m[positionM].message = (char *) calloc(1, sizeM);
        m[positionM].time = (char *) calloc(1, sizeT);
        m[positionM].sender = (char *) calloc(1, sizeS);
        m[positionM].receiver = (char *) calloc(1, sizeR);
        
        strcat(m[positionM].sender, sender);
        strcat(m[positionM].receiver, receiver);
        strcat(m[positionM].message, message);
        strcat(m[positionM].time, time);
        
        positionM++;
    }
}

//#################### PATACCHE CHE SERVONO PER CURL E JSON ##########################
struct curl_fetch_st {
    char *payload;
    size_t size;
};

struct MemoryStruct {
    char *memory;
    size_t size;
};
//############################ JavaScript Parser ################################

int isDigit(char *str){
    int a = 0;
    char *n[11] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
    do{
        if(strcmp(str, n[a]) == 0){
            return 1;
        }
        a++;
    }while(a<10);
    return 0;
}

char *extract(char *str, int index){
    int length = strlen(str);
    
    char gigi[2] = ":";
    gigi[1] = '\0';
    char space[2] = " ";
    space[1] = '\0';
    char comma[2] = ",";
    comma[1] = '\0';
    char apice[2] = "\"";
    apice[1] = '\0';
    char *value;
    value = (char *)calloc(0,0);
    int size = 1;
    int quantity = 1;
    
    char *c = (char *)malloc(sizeof(char)*2);
    char f = '\0';
    int gg = 0;
    
    for(int a = index; a<length; a++){
        c = charAt(str, a);
        if(strcmp(c, gigi) == 0){
            do{
                c = charAt(str, a);
                if(strcmp(c, apice) == 0){
                    a++;
                    do{
                        if(a>=length){
                            c = (char *) malloc(0);
                            return "";
                        }
                        c = charAt(str, a);
                        if(strcmp(c, apice) != 0){
                            quantity++;
                            size = sizeof(char) * quantity;
                            
                            value = (char *) realloc(value, size);
                            strcat(value, c);
                        }else{
                            c = (char *) malloc(0);
                            return value;
                        } 
                        a++;
                    }while(1);
                }else if(strcmp(c, space) == 0){
                    if(a>=length){
                        c = (char *) malloc(0);
                        return "";
                    }
                }else if(isDigit(c) == 1){
                    do{    
                        c = charAt(str, a);
                        if(a>=length){
                            return "";
                        }if(strcmp(c, comma) == 0){
                            return value;
                        }
                        quantity++;
                        size = sizeof(char) * quantity;
                        
                        value = (char *) realloc(value, size);
                        strcat(value, c);
                        a++;
                    }while(1);
                }
                a++;
            }while(1);
        }
    }
    c = (char *) malloc(0);
    return "";
}

char *find(char *str, char *tofind){
    int length = strlen(str);
    int length1 = strlen(tofind);
    int b = 0;
    char *char1 = (char *) malloc(sizeof(char)*2);
    char *char2 = (char *) malloc(sizeof(char)*2);
    for(int a = 0; a<length; a++){
        char1 = charAt(str, a);
        char2 = charAt(tofind, b);
        if(strcmp(char1, char2) == 0){
            b++;
            if(b == length1){
                char1 = (char *) malloc(0);
                char2 = (char *) malloc(0);
                return extract(str, a);
            }
        }else{
            b = 0;
        }
    }
    return "";
}

void retrieveUserData(char *stri, char *t[3], int mode, char g[2], char gigi[2]){ 
    unsigned long long length = strlen(stri)+1;
    
    int begin[250];
    int end[250]; 
    
    char *current = calloc(2, sizeof(char)*2);
    unsigned long long alpha = 0, beta = 0;
    
    for(alpha = 0; alpha < length; alpha++){
        current = charAt(stri, alpha);
        
        if(strcmp(current, g) == 0){
            begin[beta] = alpha;
        }
        if(strcmp(current, gigi) == 0){
            end[beta] = alpha;
            beta++;
        }
    }
    
    int ji = 0;
    int ypsilon = beta - 1;
    int h7 = 0;
    
    do{
        char *str = substring(stri, begin[ji], end[ji]);
        char *i1 = find(str, t[0]);
        char *i2 = find(str, t[1]);
        char *i3 = find(str, t[2]);
        if(mode == 2){
        growNames(i1, i2, substring(i3, 5, strlen(i3)));
        }else{
        growNames(i1, i2, i3); 
        }
        ji++;
    }while(ji<ypsilon);
}

void findSelector(char *str, char *tofind, int mode, char gigi[2], char gigione[2]){
    unsigned long long int length = strlen(str);
    unsigned long long int length1 = strlen(tofind);
    
    unsigned long long begincut = 0, endcut = 0;
    char *char1 = (char *)malloc(sizeof(char)*2);
    char *char2 = (char *)malloc(sizeof(char)*2);
    
    for(unsigned long long int a = 0, b = 0; a<length; a++){
        char1 = charAt(str, a);
        char2 = charAt(tofind, b);
        if(strcmp(char1, char2) == 0){
            b++;
            if(b == length1){
                begincut = a;
                if(mode == 1){
                    break;
                }
            }
        }else{
            b = 0;
        }
    }
    char2 = (char *)malloc(0);
    int close = 0;
    
    if(mode == 2){
       close = -1;
    }
        for(unsigned long long int a = begincut-1; a<length; a++){
            char1 = charAt(str, a);
            if(strcmp(char1, gigi) == 0){ close++; }
            if(strcmp(char1, gigione) == 0){ 
                close--; 
                if(close == 0){
                    endcut = a; 
                    break;
                }
            }
        }
    
    
    char *stri = (char *) malloc((begincut-endcut)*sizeof(char)+1);
    stri = substring(str, begincut, endcut);  
    str = (char *) malloc(0);
    if(mode == 1){
        char *iii[3] = {"name", "firstName", "id"}; 
        retrieveUserData(stri, iii, mode, gigi, gigione);
    }
    else if(mode == 2){
        char *iii[3] = {"name", "short_name", "id"};
        retrieveUserData(stri, iii, mode, gigi, gigione); 
    }
}

//############################ html parser ######################################
char *loadFile(){
    char *str;
    str = (char *) malloc(sizeof(char));
    
    int size = sizeof(str);
    int c, d = 0;
    
    FILE *file;
    file = fopen(cookieF, "r");
    if (file) {
        while ((c = getc(file)) != EOF){
            size += sizeof(c);
            str = (char *) realloc(str, size);
            str[d] = c;
            d++;
        }
        fclose(file);
    }
    str[d] = '\0';
    return str;
}

char *extractcode(char *str, long long int index){
    int length = strlen(str);
    
    char gigi[2] = "=";
    gigi[1] = '\0';
    char space[2] = " ";
    space[1] = '\0';
    char comma[2] = ",";
    comma[1] = '\0';
    char apice[2] = "\"";
    apice[1] = '\0';
    char *value;
    value = (char *)calloc(0,0);
    int size = 1;
    int quantity = 1;
    
    char *c = (char *)malloc(sizeof(char)*2);
    char f = '\0';
    int gg = 0;
    
    for(long long int a = index; a<length; a++){
        c = charAt(str, a);
        if(strcmp(c, gigi) == 0){
            do{
                c = charAt(str, a);
                if(strcmp(c, apice) == 0){
                    a++;
                    do{
                        if(a>=length){
                            c = (char *) malloc(0);
                            return "";
                        }
                        c = charAt(str, a);
                        if(strcmp(c, apice) != 0){
                            quantity++;
                            size = sizeof(char) * quantity;
                            
                            value = (char *) realloc(value, size);
                            strcat(value, c);
                        }else{
                            c = (char *) malloc(0);
                            return value;
                        } 
                        a++;
                    }while(1);
                }else if(strcmp(c, space) == 0){
                    if(a>=length){
                        c = (char *) malloc(0);
                        return "";
                    }
                }
                a++;
            }while(1);
        }
    }
    c = (char *) malloc(0);
    return "";
}

char *findcode(char *str, char *tofind){
    
    int length = strlen(str);
    int length1 = strlen(tofind);
    int b = 0;
    
    char *char1 = (char *)malloc(sizeof(char)*2);
    char *char2 = (char *)malloc(sizeof(char)*2);
    char *code = (char *)malloc(1);
    
    for(int a = 0; a<length; a++){
        char1 = charAt(str, a);
        char2 = charAt(tofind, b);
        if(strcmp(char1, char2) == 0){
            b++;
            if(b == length1){
                char *ypsilon = extractcode(str, a);
                return ypsilon;
            }
        }else{
            b = 0;
        }
    }
    return "";
}

//############################ loading screen ######################################
int lscreenStop;
int exited;

void *loadingScreen(){
    int sleeptime = 100000;
    while(lscreenStop == 1)
    {
        if(lscreenStop == 0)
            break;
        usleep(sleeptime);
        system("tput clear");
        if(lscreenStop == 0)
            break;
        printf("\n\n\n\n\n\n\n\n\n\n\n\n      \\ Loading..\n\n\n\n\n\n\n\n\n");
        usleep(sleeptime);
        system("tput clear");
        if(lscreenStop == 0)
            break;
        printf("\n\n\n\n\n\n\n\n\n\n\n\n      | lOading...\n\n\n\n\n\n\n\n\n");
        usleep(sleeptime);
        system("tput clear");
        if(lscreenStop == 0)
            break;
        printf("\n\n\n\n\n\n\n\n\n\n\n\n      / loAding....\n\n\n\n\n\n\n\n\n");
        usleep(sleeptime);
        system("tput clear");
        if(lscreenStop == 0)
            break;
        printf("\n\n\n\n\n\n\n\n\n\n\n\n      - loaDing.....\n\n\n\n\n\n\n\n\n");
        usleep(sleeptime);
        system("tput clear");
        if(lscreenStop == 0)
            break;
        printf("\n\n\n\n\n\n\n\n\n\n\n\n      \\ loadIng....\n\n\n\n\n\n\n\n\n");
        usleep(sleeptime);
        system("tput clear");
        if(lscreenStop == 0)
            break;
        printf("\n\n\n\n\n\n\n\n\n\n\n\n      | loadiNg...\n\n\n\n\n\n\n\n\n");
        usleep(sleeptime);
        system("tput clear");
        if(lscreenStop == 0)
            break;
        printf("\n\n\n\n\n\n\n\n\n\n\n\n      / loadinG..\n\n\n\n\n\n\n\n\n");
        usleep(sleeptime);
        system("tput clear");
        if(lscreenStop == 0)
            break;
        printf("\n\n\n\n\n\n\n\n\n\n\n\n      - loading.\n\n\n\n\n\n\n\n\n");
    }
    
    system("tput clear");
    exited = 1;
    return NULL;
}

//############################ Curl callback ######################################
static size_t

WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp){
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    
    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }
    
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    
    return realsize;
}

size_t curl_callback (void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;                             
    struct curl_fetch_st *p = (struct curl_fetch_st *) userp;  
    
    p->payload = (char *) realloc(p->payload, p->size + realsize + 1);
    
    
    if (p->payload == NULL) {
        fprintf(stderr, "ERROR: Failed to expand buffer in curl_callback");
        free(p->payload);
        return -1;
    }
    memcpy(&(p->payload[p->size]), contents, realsize);
    p->size += realsize;
    p->payload[p->size] = 0;
    
    return realsize;
}

static size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata){
    return nitems * size;
}

void substr(char phrase[], int low, int high){
    char character = phrase[low];
    int a = low;
    int b = 0;
    do{
        character = phrase[a];
        result[b] = character;
        a++;
        b++;
    }while(a<high);
    
    character = '\0';
    result[b] = character;        
}

void flush(){
    int a = 0;
    char *b = "";
    do{
        result[a] = b[0];
        a++;
    }while(a<sizeof(result));
}

void flushCurl(){
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    curl_global_cleanup();
    curl_easy_setopt(curl, CURLOPT_VERBOSE, NULL);
}

char *getuserid(){
    char *str = loadFile();
    char *tofind = "c_user";
    int length = strlen(str);
    int length1 = strlen(tofind);
    int b = 0;
    
    char *char1 = (char *)malloc(sizeof(char)*2);
    char *char2 = (char *)malloc(sizeof(char)*2);
    
    int lowtown = 0, hightown = 0; 
    char space[2] = "\n";
    space[1] = '\0';
    char gr[2] = "#";
    gr[1] = '\0';
    
    for(int a = 0; a<length; a++){
        char1 = charAt(str, a);
        char2 = charAt(tofind, b);        
        if(strcmp(char1, char2) == 0){
            b++;
            if(b == length1){
                lowtown = a;
                do{
                    if(strcmp(charAt(str, a), gr) == 0){
                        char *cutted = substring(str, lowtown+2, a-2);
                        return cutted;
                        break;
                    }
                    a++;
                }while(a<length);
            }
        }else{
            b = 0;
        }
    }
    return "";
}

unsigned char getNthBit(unsigned char c, unsigned char n){
    unsigned char tmp=1<<n;
    return (c & tmp)>>n;
}

void strip(char *s) {
    char *p2 = s;
    while(*s != '\0') {
        if(*s != '\t' && *s != '\n') {
            *p2++ = *s++;
        } else {
            ++s;
        }
    }
    *p2 = '\0';
}

const char *getShit(char *a, struct json_object *messageObj){
    struct json_object *messagePlain = json_object_object_get(messageObj, a);
    return json_object_get_string(messagePlain);
}   

const char *getShit2(int i, struct json_object *messageObj){
    struct json_object *messagePlain = json_object_array_get_idx(messageObj, i);
    return json_object_get_string(messagePlain);
}   

void loadMessages(char *target){
    flushCurl();
    
    int size = 0;
    char *g = "messages[user_ids][";
    //vai beppe
    char *uf = "messages[user_ids][][offset]=0&messages[user_ids][][timestamp]&messages[user_ids][][limit]=20&client=web_messenger&__user=&__a=1&__dyn=aKhoFeyfyGmaqQ449UoGya4A5ER6yUmyVbGAF9oyfirWo8popyui9zob4q4F8Hxubxu7UaqwHzQ4UJi2eq4EnFeeKcVrDG4XzFE8oiGta5Eaeum4UpKq4G-FFUkgmVV8Gicx2jxm1iyECium8yUgx66EK3Ou49LZooQ4bBx-EuHAyAdGezu49oG&__af=o&__req=4s&__be=-1&__pc=PHASED%3ADEFAULT&__rev=2667401&fb_dtsg=AQE9_3hC9d7T%3AAQF7fkzv-KoP&ttstamp=2658169579551104675710055845865817055102107122118457511180";
    
    size += strlen(uf);
    size += strlen(userid);
    size += strlen(target)*3;
    size += strlen(usercode);
    size = (sizeof(char)*size) +1;
    char *hd_src = (char *) calloc (1, size);    
    
    strcat(hd_src, g);
    strcat(hd_src, target);
    strcat(hd_src, "][offset]=0&messages[user_ids][");
    strcat(hd_src, target);
    strcat(hd_src, "][limit]=15&messages[user_ids][");
    strcat(hd_src, target);
    strcat(hd_src, "][timestamp]&client=web_messenger&__user=");
    strcat(hd_src, userid);
    strcat(hd_src, "&__a=1&__dyn=&__af=o&__req=5&__be=-1&__pc=PHASED%3ADEFAULT&fb_dtsg=");
    strcat(hd_src, usercode);
    strcat(hd_src, "&ttstamp=&__rev=&__srp_t=");
    
    struct MemoryStruct chunk;
    
    chunk.memory = malloc(1);  
    chunk.size = 0;    
    
    char *url = "https://www.facebook.com/ajax/mercury/thread_info.php?dpr=1";
    
    struct curl_slist *headers = NULL;                      
    
    curl_slist_append(headers, "POST /ajax/mercury/thread_info.php?dpr=1 HTTP/1.1");  
    curl_slist_append(headers, "Host: www.facebook.com");
    curl_slist_append(headers, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");               
    curl_slist_append(headers, "Accept-Language: it-IT,it;q=0.8,en-US;q=0.5,en;q=0.3");
    curl_slist_append(headers, "Accept-Encoding: gzip, deflate, br");
    curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    curl_slist_append(headers, "X-MSGR-Region: LLA");
    curl_slist_append(headers, "Connection: keep-alive");  
    
    curl_easy_setopt(curl, CURLOPT_REFERER, "http://www.facebook.com/");  
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);   
    
    curl_easy_setopt(curl, CURLOPT_UNRESTRICTED_AUTH, 1L);  
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);   
    curl_easy_setopt(curl, CURLOPT_HEADER, 0);             
    curl_easy_setopt(curl, CURLOPT_URL, url);              
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);     
    curl_easy_setopt(curl, CURLOPT_PIPEWAIT, 1L);          
    
    curl_easy_setopt(curl, CURLOPT_COOKIE, NULL);          
    curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookieF);    
    curl_easy_setopt (curl, CURLOPT_COOKIEFILE, cookieF);  
    curl_easy_setopt(curl, CURLOPT_COOKIESESSION, 0);      
    
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); 
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, hd_src);
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    
    lscreenStop = 1;
    pthread_create(&pth, NULL, loadingScreen, NULL);
    res = curl_easy_perform(curl);
    lscreenStop = 0;
    pthread_join(pth,NULL);
    curl_slist_free_all(headers);
    //#####################################################################################################################                
    
    int gigi = 9;
    int alpha = strlen(chunk.memory)-gigi, beta = 0;
    char str[alpha];
    FILE *f = fopen("file.txt", "w");
	fprintf(f, "%s", str);
    do{
        str[beta] = chunk.memory[beta+gigi];
        beta++;
    }while(beta<alpha+gigi);
    if(strlen(str)>0){
		
        struct json_object *jobj;
        
        jobj = json_tokener_parse((char *)&str);
        
        struct json_object *payload;
        
        struct json_object *messageArray, *messageObj;
        
        json_object_object_get_ex(jobj, "payload", (void *)&payload); 
        
        if(payload == NULL){
            printf("Errore, non carica il json..");
        }else{
            messageArray = json_object_object_get (payload, "actions");
            if(messageArray){
            int delta = 0;
            int omega = json_object_array_length(messageArray);
            
            for (int i = 0; i < omega; i++) {
                messageObj = json_object_array_get_idx(messageArray, i);
                
                const char *message = getShit("body", messageObj);
				const char *time = getShit("timestamp", messageObj);
                const char *senderr = getShit("author", messageObj);
                const char *receiver = getShit("other_user_fbid", messageObj);
            
                char sender[strlen(senderr)+1];
                for(int ah = 0; ah<strlen(sender)+1; ah++)                    
                sender[ah] = senderr[ah+5];
                sender[strlen(sender)] = '\0';
                
                if(strcmp(userid, sender) == 0){
                    growMessages(userid, target, message, time);
                }else{
                    growMessages(target, userid, message, time);
                }                
            }
            
            free(payload);
            free(messageArray);
            free(messageObj);
            free(jobj);
            }else{printf("No messages..\n");}
        }
    }else{
        printf("No messages to see.. sh*t");
    }
    
    if(res != CURLE_OK){
        fprintf(stderr, "Richiesta non riuscita: %s\n",
                curl_easy_strerror(res));     
    }         
    curl_global_cleanup();
}

int cookieAvailability(){
    FILE *fptr = NULL;
    if ( !( fptr = fopen( cookieF, "r" ))){
        printf( "Cookie not found\n" );
        return 0;    
    }
    else {
        fseek(fptr, 0, SEEK_END);
        unsigned long len = (unsigned long)ftell(fptr);
        if (len > 0) { 
            fclose( fptr );
            return 1;
        }else{
            fclose( fptr );
            return 0;
        }
    }
    return 0;
}

int noInternet;

void getRecentContacts(){
    struct MemoryStruct chunk;
    
    chunk.memory = malloc(1); 
    chunk.size = 0; 
    
    flushCurl();
    char *url = "https://www.facebook.com/messages/";
    
    char post[120] = "refsrc=https://m.facebook.com/&refid=8&_rdr=";
    
    curl_easy_setopt(curl, CURLOPT_REFERER, "http://www.facebook.com/");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);   
    
    curl_easy_setopt(curl, CURLOPT_UNRESTRICTED_AUTH, 1L);  
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);    
    
    curl_easy_setopt(curl, CURLOPT_HEADER, NULL);             
    
    curl_easy_setopt(curl, CURLOPT_URL, url);               
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);     
    curl_easy_setopt(curl, CURLOPT_PIPEWAIT, 1L);          
    
    curl_easy_setopt(curl, CURLOPT_COOKIE, NULL);          
    curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookieF);    
    curl_easy_setopt (curl, CURLOPT_COOKIEFILE, cookieF);  
    curl_easy_setopt(curl, CURLOPT_COOKIESESSION, 0);      
    
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent); 
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); 
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    
    lscreenStop = 1;
    pthread_create(&pth, NULL, loadingScreen, NULL);
    res = curl_easy_perform(curl);
    lscreenStop = 0;
    pthread_join(pth, NULL);
    
    curl_easy_cleanup(curl);
    
    if(res != CURLE_OK){
        printf("Fallito accesso ad internet. Cercare una soluzione online??\n");            
        noInternet = 1;
    }
    
    if(noInternet == 0){
        int gigih = 0;
        int alphah = strlen(chunk.memory)-gigih, betah = 0;
        char strh[alphah+1];
        
        do{
            strh[betah] = chunk.memory[betah+gigih];
            betah++;
        }while(betah<alphah+gigih);
        strh[betah] = '\0';
        findSelector((char *)&strh, "participants:", 2, "{", "}");
    }
    curl_global_cleanup();
}

void login(){
    struct MemoryStruct chunk;
    
    chunk.memory = malloc(1); 
    chunk.size = 0;   
    
    char email[50] = "";     
    char password[50] = "";  
    
    int cookieIsAvailable = cookieAvailability(cookieF);
    flushCurl();
    
    if(curl) {
        
        int exit = 0;
        
        if(cookieIsAvailable == 0){         //SE DIOSTRACANE NON C'è IL COOKIE
            printf("Insert email fucker: ");
            fgets(email, 50, stdin);
            
            printf("Insert password kunt: ");
            fgets(password, 50, stdin);
            
            email[strlen(email)] = '\0';
            password[strlen(password)] = '\0';
            email[strlen(email)-1] = '0';
            password[strlen(password)-1] = '0';
            email[strlen(email)-1] = '\0';
            password[strlen(password)-1] = '\0';
            
            char *url = "https://mobile.facebook.com/login.php"; 
            
            char post[512] = "lsd=AVrpu0xm&charset_test=%E2%82%AC%2C%C2%B4%2C%E2%82%AC%2C%C2%B4%2C%E6%B0%B4%2C%D0%94%2C%D0%84&version\
            =1&ajax=0&width=0&pxr=0&gps=0&dimensions=0&m_ts=1476268003&li=4w_-V5QCzPUEjKiw7pxX-1E0&email=";
            strcat(post, email);
            strcat(post, "&pass=");
            strcat(post, password);
            strcat(post, "&login=Log+In");                
            
            curl_easy_setopt(curl, CURLOPT_HEADER, NULL);
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0); 
            curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent); 
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); 
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
            
            curl_easy_setopt(curl, CURLOPT_COOKIE, NULL); 
            curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookieF); 
            curl_easy_setopt (curl, CURLOPT_COOKIEFILE, cookieF); 
            
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
            
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post);
            
            lscreenStop = 1;
            pthread_create(&pth, NULL, loadingScreen, NULL);
            res = curl_easy_perform(curl);
            lscreenStop = 0;
            pthread_join(pth,NULL);
            
            curl_easy_cleanup(curl);
            
            if(res != CURLE_OK){
                printf("Fallito accesso ad internet. Cercare una soluzione online??\n");   
                noInternet = 1;
            }
            
        } if (noInternet == 0) {                          
            flushCurl();
            char *url = "https://www.facebook.com/home.php";
                
            char post[120] = "refsrc=https://m.facebook.com/&refid=8&_rdr=";
                
            curl_easy_setopt(curl, CURLOPT_REFERER, "http://www.google.com/");
            curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);   
                
            curl_easy_setopt(curl, CURLOPT_UNRESTRICTED_AUTH, 1L);  
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);    
                
            curl_easy_setopt(curl, CURLOPT_HEADER, NULL);             
                
            curl_easy_setopt(curl, CURLOPT_URL, url);               
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);     
            curl_easy_setopt(curl, CURLOPT_PIPEWAIT, 1L);          
            
            curl_easy_setopt(curl, CURLOPT_COOKIE, NULL);          
            curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookieF);    
            curl_easy_setopt (curl, CURLOPT_COOKIEFILE, cookieF);  
            curl_easy_setopt(curl, CURLOPT_COOKIESESSION, 0);      
            
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent); 
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); 
            curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
            
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
            
            lscreenStop = 1;
            pthread_create(&pth, NULL, loadingScreen, NULL);
            res = curl_easy_perform(curl);
            lscreenStop = 0;
            pthread_join(pth,NULL);
            
            curl_easy_cleanup(curl);
            
            if(res != CURLE_OK){
                printf("Fallito accesso ad internet. Cercare una soluzione online??\n");            
                noInternet = 1;
            }
            
            if(noInternet == 0){
                int gigi = 0;
                int alpha = strlen(chunk.memory)-gigi, beta = 0;
                char str[alpha+1];
                
                do{
                    str[beta] = chunk.memory[beta+gigi];
                    beta++;
                }while(beta<alpha+gigi);
                str[beta] = '\0';
                
                char *p0 = getuserid();
                userid = (char *) malloc(sizeof(char)*strlen(p0)+1);
                strcpy(userid, p0);
                usercode = findcode((char *)&str, "fb_dtsg");
                
                if(strlen(usercode)>5){
                    getRecentContacts();
                    findSelector((char *)&str, "shortProfiles:", 1, "{", "}");
                }else{ printf("No connection..\n"); noInternet = 1; }
            }
        }
    }       
    curl_global_cleanup();
}

void printMessages(int other){
    int a = 0;
    do{
        if(n[other].fib && m[a].sender){
            if(strcmp(n[other].fib, m[a].sender) == 0 ){
                printf("# %s #\n# %s #\n%s\n####################################################################################################################\n", m[a].time, n[other].surname, m[a].message);
            }else if(strcmp(n[other].fib, m[a].receiver) == 0){
                printf("                         # %s #\n                         # Io #\n                         %s\n####################################################################################################################\n", m[a].time, m[a].message);
            }
        }
        a++;
    }while(a<positionM);
    printf("\n->");
}

void sendmsgf(char *target, char *messaggio){
    flushCurl();
    
    int size = 0;
    char *g = "fb_dtsg=";
    char *uf = "fb_dtsg=&charset_test=%E2%82%AC%2C%C2%B4%2C%E2%82%AC%2C%C2%B4%2C%E6%B0%B4%2C%D0%94%2C%D0%84&ids%5B%5D=&text_ids%5B%5D=&body=&Send=Invia";
    
    size += strlen(uf);
    size += strlen(usercode);
    size += strlen(target)*3;
    size += strlen(messaggio);
    size = (sizeof(char)*size) +1;
    char *hd_src = (char *) calloc (1, size); 
    
    strcat(hd_src, g);
    strcat(hd_src, usercode);
    strcat(hd_src, "&charset_test=%E2%82%AC%2C%C2%B4%2C%E2%82%AC%2C%C2%B4%2C%E6%B0%B4%2C%D0%94%2C%D0%84&ids%5B");
    strcat(hd_src, target);
    strcat(hd_src, "%5D=");
    strcat(hd_src, target);
    strcat(hd_src, "&text_ids%5B");
    strcat(hd_src, target);
    strcat(hd_src, "&body=");
    strcat(hd_src, messaggio);
    strcat(hd_src, "&Send=Invia");
    
    struct MemoryStruct chunk;
    
    chunk.memory = malloc(1);  
    chunk.size = 0;    
    
    char *url = "https://m.facebook.com/messages/send/?icm=1&refid=12";
    
    struct curl_slist *headers = NULL;                      
    
    curl_slist_append(headers, "POST /messages/send/?icm=1&refid=12 HTTP/1.1");  
    curl_slist_append(headers, "Host: m.facebook.com");
    curl_slist_append(headers, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");               
    curl_slist_append(headers, "Accept-Language: it-IT,it;q=0.8,en-US;q=0.5,en;q=0.3");
    curl_slist_append(headers, "Accept-Encoding: gzip, deflate, br");
    curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    curl_slist_append(headers, "Connection: keep-alive");  
    curl_slist_append(headers, "Upgrade-Insecure-Requests: 1");
    
    curl_easy_setopt(curl, CURLOPT_REFERER, "http://www.facebook.com/");  
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);   
    
    curl_easy_setopt(curl, CURLOPT_UNRESTRICTED_AUTH, 1L);  
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);   
    curl_easy_setopt(curl, CURLOPT_HEADER, 0);             
    curl_easy_setopt(curl, CURLOPT_URL, url);              
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);     
    curl_easy_setopt(curl, CURLOPT_PIPEWAIT, 1L);          
    
    curl_easy_setopt(curl, CURLOPT_COOKIE, NULL);          
    curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookieF);    
    curl_easy_setopt (curl, CURLOPT_COOKIEFILE, cookieF);  
    curl_easy_setopt(curl, CURLOPT_COOKIESESSION, 0);      
    
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); 
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, hd_src);
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    
    lscreenStop = 1;
    pthread_create(&pth, NULL, loadingScreen, NULL);
    res = curl_easy_perform(curl);
    lscreenStop = 0;
    pthread_join(pth,NULL);
    curl_slist_free_all(headers);
    
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    
    int gigi = 9;
    int alpha = strlen(chunk.memory)-gigi, beta = 0;
    char str[alpha];
    
    do{
        str[beta] = chunk.memory[beta+gigi];
        beta++;
    }while(beta<alpha+gigi);
    
    if(strlen(str)>0){
    }else{
        printf("No messages to see..\n");
    }
    
    chunk.memory = malloc(0);  
}

int currentContact;

void printContacts(void){
    currentContact = -1;
    size_t len = 254;
    char *color = "\x1B[34m";
    char *normal = "\x1B[0m";
    do{
        int a = 0;
        do{
            if(n[a].update == 0){
            printf("%s%d) %s\n", normal, a, n[a].name);
            }else{
                printf("%s%d) %s%s\n", color, a, n[a].name, normal);
            }
            a++;
        }while(a<positionN);
        
        printf("->");
        char *pp = (char *) calloc(4, sizeof(char)*3+1);
        getline(&pp, &len, stdin);
        int p = atoi(pp);
        system("tput clear");
        
        if(p >= 0 && p < positionN && n[p].loaded == 0){
            loadMessages(n[p].fib);
            printMessages(p);
            currentContact = p;
            n[p].update = 0;
        }else if(n[p].loaded == 1){
            printMessages(p);
            currentContact = p;
            n[p].update = 0;
        }
        do{
            char *mm = (char *)calloc(255, sizeof(char)*255+1);
            char *esc = "000";
            getline(&mm, &len, stdin);
            char *mmi = substring(mm, 0, strlen(mm)-2);
            free(mm);
            
            if(strcmp(mmi, esc) == 0){
                currentContact = -1;
                break;
            }else{
                time_t t = time(NULL);
                struct tm *tmp = gmtime(&t);
                int hourl = tmp->tm_hour + 1;
                int minl = tmp->tm_min;
                
                char str[3];
                sprintf(str, "%d", hourl);
                
                char *tt = (char *)calloc(6, sizeof(char)*6+1);
                strcat(tt, str);
                sprintf(str, "%d", minl);
                strcat(tt, ":");
                strcat(tt, str);
                
                sendmsgf(n[p].fib, mmi);
                growMessages(userid, n[p].fib, mmi, tt);
                system("tput clear");
                printMessages(p);
            }          
        }while(1);
        currentContact = -1;
        break;
    }while(1);
}

void menu(){
    size_t len = 0;
    do{
        system("tput clear");
        printf("######## Facebook  ######## \n\n");
        printf("1) Selezione utente\n2) Quit the g4me\n3) Slogga\n->");
        
        char *pp = (char *) calloc(4, sizeof(char)*3+1);
        getline(&pp, &len, stdin);
        int p = atoi(pp);
        
        if(p == 1){
            printContacts();
        }else if (p == 2){
            break;
        }else if (p == 3){
            remove(cookieF);
            login();
        }
    }while(1);
}

char *seq;

void *getUpdates(){
    do{
    
    CURL *curl = NULL;
//     curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_VERBOSE, NULL);
    
    CURLM *multi_handle = NULL;
    int siu = 0;
    int still_running; 
    int repeats = 0;
    
    struct MemoryStruct chunk;
    
    chunk.memory = malloc(1);  
    chunk.size = 0;    
    
    char *p1 = "channel=p_";
    
    char *ps2 = "https://6-edge-chat.facebook.com/pull?";
    char *p2 = "channel=p_&seq=&partition=-2&clientid=45e9af40&cb=iz1q&idle=3&qp=y&cap=8&pws=fresh&isq=31265&msgs_recv=1&uid=&viewer_uid=&request_batch=0&msgr_region=LLA&state=active";
    
    int sizeOne = sizeof(char)*(strlen(p2)+strlen(seq)+strlen(userid)*3)+1;
    
    char *hd_src = (char *)calloc(1, sizeOne);
    
    strcat(hd_src, p1);
    strcat(hd_src, userid);
    strcat(hd_src, "&seq=");
    strcat(hd_src, seq);
    strcat(hd_src, "&partition=-2&clientid=");
    strcat(hd_src, "45e9af40");
    strcat(hd_src, "&cb=iz1q&idle=3&qp=y&cap=8&pws=fresh&isq=31265&msgs_recv=1&uid=");
    strcat(hd_src, userid);
    strcat(hd_src, "&viewer_uid=");
    strcat(hd_src, userid); 
    strcat(hd_src, "&request_batch=1&msgr_region=LLA&state=active");
    
    int sizeThree = (strlen(ps2)+strlen(hd_src))*sizeof(char)+1;
    char *url = (char *)calloc(1, sizeThree);
    strcat(url, ps2);
    strcat(url, hd_src);
    
    char *p4 = "GET /pull?";
    char *p6 = " HTTP/1.1";
    int sizeTwo = ((strlen(p4)+strlen(p6))*sizeof(char))+sizeOne;
    char *p5 = (char *)calloc(1, sizeTwo);
    strcat(p5, p4);
    strcat(p5, hd_src);
    strcat(p5, p6);    
        
    struct curl_slist *headers = NULL;              
    
    curl_slist_append(headers, p5);  
    curl_slist_append(headers, "Host: www.facebook.com");
    curl_slist_append(headers, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");               
    curl_slist_append(headers, "Accept-Language: it-IT,it;q=0.8,en-US;q=0.5,en;q=0.3");
    curl_slist_append(headers, "Accept-Encoding: gzip, deflate, br");
    curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    curl_slist_append(headers, "X-MSGR-Region: LLA");
    curl_slist_append(headers, "Connection: keep-alive");  
    
    curl_easy_setopt(curl, CURLOPT_REFERER, "http://www.facebook.com/");  
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);   
    
    curl_easy_setopt(curl, CURLOPT_UNRESTRICTED_AUTH, 1L);  
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);   
    curl_easy_setopt(curl, CURLOPT_HEADER, 0);             
    curl_easy_setopt(curl, CURLOPT_URL, url);
    
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);     
    curl_easy_setopt(curl, CURLOPT_PIPEWAIT, 1L);          
    
    curl_easy_setopt(curl, CURLOPT_COOKIE, NULL);          
    curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookieF);    
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, cookieF);  
    curl_easy_setopt(curl, CURLOPT_COOKIESESSION, 0);      
    
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); 
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
 
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    multi_handle = curl_multi_init();
    
    curl_multi_add_handle(multi_handle, curl);
    
    curl_multi_perform(multi_handle, &still_running);
    
    int timej = 1000;
    int rep = 0;
    
    do {
        CURLMcode mc; 
        int numfds, repeats = 0;
        
        mc = curl_multi_perform(multi_handle, &still_running); 
        
        mc = curl_multi_wait(multi_handle, NULL, 0, timej, &numfds);
        
        if(mc != CURLM_OK) {
            fprintf(stderr, "curl_multi_wait() failed, code %d.\n", mc);
            break;
        }
        
        if(!numfds) {
            repeats++; 
            if(repeats > 1) {
                usleep(timej*1000); 
            }
        }else
            repeats = 0;
        
        int gigi = 9;
        int alpha = strlen(chunk.memory)-gigi, beta = 0;
        char str[alpha];
        
        do{
            str[beta] = chunk.memory[beta+gigi];
            beta++;
        }while(beta<alpha+gigi);
        str[beta] = '\0';
        
        rep++;
        
        if(strlen(str)>24 && strlen(str)<40){
            seq = find(str, "seq");
        }else if(strlen(str)>39){
                    seq = find(substring((char *)&str, 90, strlen(str)), "seq");
                    
        }
        
        if(strlen(str)>115){
        //JSONJSONJSONJSONJSONJSONJSONJSONJSONJSONJSONJSONJSONJSONJSONJSONJSONJSONJSONJSON
            struct json_object *jobj, *jk;
            jobj = json_tokener_parse((char *)&str);
            
            if(jobj != NULL){
                struct json_object *batches, *meta, *ms, *lanciat, *arr, *lancia;
                json_object_object_get_ex(jobj, "batches", (void *)&batches); 
                if(batches != NULL){
                    arr = json_object_array_get_idx(batches, 1);
                    if(arr != NULL){
                        ms = json_object_object_get (arr, "ms");
                        if(ms != NULL){
                            int delta = 0;
                            int omega = json_object_array_length(ms);
                            for (int i = 0; i < omega; i++) {
                                lanciat = json_object_array_get_idx(ms, i);
                                lancia = json_object_object_get(lanciat, "delta");
                                if(lancia != NULL){
                                    meta = json_object_object_get(lancia, "messageMetadata");
                                    if(meta != NULL){
                                        const char *sender = json_object_get_string(json_object_object_get(meta, "actorFbId"));
                                        const char *body = json_object_get_string(json_object_object_get(lancia, "body"));
                                        
                                        time_t t = time(NULL);
                                        struct tm *tmp = gmtime(&t);
                                        int hourl = tmp->tm_hour + 1;
                                        int minl = tmp->tm_min;
                                        
                                        char str[3];
                                        sprintf(str, "%d", hourl);
                                        
                                        char *tt = (char *)calloc(6, sizeof(char)*6+1);
                                        strcat(tt, str);
                                        sprintf(str, "%d", minl);
                                        strcat(tt, ":");
                                        strcat(tt, str);
                                        if(body){
                                        growMessages(sender, userid, body, tt);
                                        }
                                        char *sendern;
                                        strcpy(sendern, sender);
                                        
                                        int h7 = findUserPosition(sendern);
                                        if(currentContact == h7){
                                            printMessages(h7);
                                        }else{
                                            if(sendern != userid)
                                            n[h7].update = 1;
                                        }
                                    }    
                                }
                            }
                        }                
                    }
                }
            }
            //JSONJSONJSONJSONJSONJSONJSONJSONJSONJSONJSONJSONJSONJSONJSONJSONJSONJSONJSONJSON
        }
    } while(still_running);
    
    curl_multi_remove_handle(multi_handle, curl);
    curl_easy_cleanup(curl);
    curl_multi_cleanup(multi_handle);
    curl_global_cleanup();
    
    }while(1);
}

int main(void)
{       
    initStructs();
    noInternet = 0;
    
    login(); 
    
    if(noInternet == 1){
        return 0;
    }
    
    seq = "0";
    currentContact = -1;
    
    pthread_create(&pthUpd, NULL, getUpdates, NULL);
    
    menu();
    
    curl_global_cleanup(); 
    
    return 0;
}
