#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "hashmap.h"

#define MCR_VERSION 1.3

struct macro
{
    char key[32];
    char macros[512];
};

struct hashmap *macroses;
char path[128];

void reset_macro(struct macro *macros)
{
    memset(macros, 0x00, sizeof(struct macro));
}

int macro_compare(const void* a, const void* b, void* udata)
{
    const struct macro *ma = a;
    const struct macro *mb = b;
    return strcmp(ma->key, mb->key);
}

uint64_t macro_hash(const void* item, uint64_t seed0, uint64_t seed1)
{
    const struct macro *macrObj = item;
    return hashmap_sip(macrObj->key, strlen(macrObj->key), seed0, seed1);
}

bool macro_iter_listcmd(const void* item, void* udata){
    const struct macro* macro = item;
    printf("%s - \"%s\"\n", macro->key, macro->macros);
    return true;
}

bool macro_iter(const void* item, void* udata){
    const struct macro* macro = item;
    printf("Key: \"%s\", macros: \"%s\"\n", macro->key, macro->macros);
    return true;
}

int init_hashmap(void)
{
    macroses = hashmap_new(sizeof(struct macro), 5, 0, 0, macro_hash, macro_compare, NULL, NULL);
}

char* convert_to_rawdata(void){
    int32_t size = 0;
    size_t iter = 0;
    void* item;
    while (hashmap_iter(macroses, &iter, &item)){
        const struct macro* macros = item;
        size += strlen(macros->key) + strlen(macros->macros) + 4;
    }
    int32_t offset = 0;
    char* data = (char*)calloc(size, sizeof(char));
    if (!data){
        fprintf(stderr, "Allocation failure!\n");
        exit(1);
    }
    iter = 0;
    item = NULL;
    while (hashmap_iter(macroses, &iter, &item)){
        const struct macro* macros = item;
        int32_t keylen = strlen(macros->key);
        int32_t macroslen = strlen(macros->macros);
        memcpy(data + offset, macros->key, keylen);
        offset += keylen;
        strcpy(data + offset, "=\"");
        offset += 2;
        memcpy(data + offset, macros->macros, macroslen);
        offset += macroslen;
        strcpy(data + offset, "\"\n");
        offset += 2;
    }
    return data;
}

int parse_macroses(void)
{
    init_hashmap();
    FILE* config = fopen(path, "r");
    if (!config){
        char command[256];
        strcpy(command, "touch ");
        strcat(command, path);
        system(command);
        config = fopen(path, "r");
    }
    fseek(config, 0L, SEEK_END);
    uint64_t filesize = ftell(config);
    if (filesize == 0) return 1;
    rewind(config);
    char contents[filesize + 1];
    int bytesRead = fread(contents, sizeof(char), filesize, config);
    contents[filesize] = '\0';
    fclose(config);
    if (bytesRead == 0){
        fprintf(stderr, "Cannot read file contents!\n");
        return -1;
    }
    uint64_t offset = 0;
    int key_registering = 1;
    int key_offset = 0;
    int macros_registering = 0;
    int macros_offset = 0;
    struct macro macros = {0};
    for (uint64_t i = 0; i < filesize; i++)
    {
        if (strlen(macros.macros) > 0 && key_registering == 1){
            hashmap_set(macroses, &macros);
            reset_macro(&macros);
        }
        if (contents[i] == '\n') continue;
        if (contents[i] == '=' && key_registering == 1){
            key_registering = 0;
            key_offset = 0;
            continue;
        }
        else if (contents[i] == '"' && macros_registering == 0){
            macros_registering = 1;
            macros_offset = 0;
            continue;
        }
        else if (contents[i] == '"' && macros_registering == 1){
            macros_registering = 0;
            macros_offset = 0;
            key_registering = 1;
            key_offset = 0;
            continue;
        }
        if (key_registering == 1)
            macros.key[key_offset++] = contents[i];
        else if (macros_registering == 1)
            macros.macros[macros_offset++] = contents[i];
        else
        {
            fprintf(stderr, "Error while parsing config. Please reset macroses or fix error yourself.\n");
            exit(-1);
        }
    }
}

void write_macroses(void){
    char* data = convert_to_rawdata();
    FILE* config = fopen(path, "w");
    if (!config) {
        fprintf(stderr, "Cannot open config file!");
        exit(1);
    }
    int retcode = fwrite(data, sizeof(char), strlen(data), config);
    if (retcode == 0 && hashmap_count(macroses) != 0) {
        fprintf(stderr, "Config writing error!\n");
        exit(1);
    }
}

void display_help(void){
    printf("Simple macros manager.\nUsage:\n");
    printf("\tmcr --install - install program\n");
    printf("\tmcr --list - display a list of available macroses\n");
    printf("\tmcr --addmacro <macros> \"<value>\" - add macros\n");
    printf("\tmcr --remove <macros> - remove macros\n");
    printf("\tmcr --update - fetch latest sources and update\n");
    printf("\tmcr --version - display current MCR version\n");
}

int main(int argc, char *argv[])
{
    if (argc == 1){
        display_help();
        exit(0);
    }
    char username[64];
    getlogin_r(username, 64);
    strcpy(path, "/home/");
    strcat(path, username);
    strcat(path, "/.config/mcr.conf");
    if (argc == 2 && strcmp(argv[1], "--install") == 0)
    {
        char current_path[1024];
        if (getcwd(current_path, sizeof(current_path)) == NULL)
        {
            fprintf(stderr, "Cannot get current directory, please install program manually or run with sudo.\n");
            return 1;
        }
        strcat(current_path, "/");
        strcat(current_path, argv[0]);
        char command[1536];
        strcpy(command, "cp ");
        strcat(command, current_path);
        strcat(command, " /bin/mcr > /dev/null");
        int retcode = system(command);
        switch (retcode)
        {
            case -1:
                fprintf(stderr, "Installation failure!\n");
                return 1;
            case 0:
                fprintf(stdout, "Installation successful!\n");
                return 0;
            default:
                fprintf(stdout, "Please check installation success.\n");
                return 0;
        }
    } //Program installation
    else if (strcmp(argv[1], "--addmacro") == 0) {
        parse_macroses();
        if (argc < 4)
            return -1;
        char *key = argv[2];
        char *macro = argv[3];
        struct macro temp = {0};
        if (strlen(key) > 32) {
            fprintf(stderr, "Too big macros key!\n");
            return -1;
        }
        if (strlen(macro) > 512) {
            fprintf(stderr, "Too big macros!\n");
            return -1;
        }
        strcpy(temp.key, key);
        strcpy(temp.macros, macro);
        hashmap_set(macroses, &temp);
        write_macroses();
        exit(0);
    } //Adding macros
    else if (strcmp(argv[1], "--remove") == 0){
        if (argc != 3) return 1;
        parse_macroses();
        struct macro temp;
        strcpy(temp.key, argv[2]);
        struct macro* macros = hashmap_get(macroses, &temp);
        if (!macros){
            fprintf(stderr, "Cannot find macros, please check list of available macroses.\n");
            exit(1);
        }
        hashmap_delete(macroses, &temp);
        write_macroses();
        exit(0);
    } //Removing macros
    else if (strcmp(argv[1], "--list") == 0){
        if (argc != 2) return 1;
        parse_macroses();
        printf(hashmap_count(macroses) > 0 ? "Available macroses:\n" : "There is no registered macroses!\n");
        hashmap_scan(macroses, macro_iter_listcmd, NULL);
        exit(0);
    }
    else if (strcmp(argv[1], "--help") == 0){
        if (argc != 2) return 1;
        display_help();
        exit(0);
    }
    else if (strcmp(argv[1], "--update") == 0){
        if (argc != 2) return 1;
        printf("[1/3] Downloading latest MCR sources...\n");
        int retcode = system("git clone https://github.com/HyperWinX/MCR.git mcr_update > /dev/null");
        if (retcode != 0){
            fprintf(stderr, "Downloading failed!\n");
            return 1;
        }
        printf("[2/3] Compiling MCR for your platform...\n");
        retcode = system("gcc -O2 -march=native mcr_update/hashmap.c mcr_update/main.c -o mcr_update/mcr > /dev/null");
        if (retcode != 0){
            fprintf(stderr, "Compiling failed!\n");
            system("rm -rf MCR");
            return 1;
        }
        printf("[3/3] Trying to replace current MCR installation...\n");
        retcode = system("rm -f /bin/mcr && mv mcr_update/mcr /bin/mcr");
        if (retcode != 0){
            fprintf(stderr, "Replacing current MCR installation failed!\n");
            system("rm -rf mcr_update");
            return 1;
        }
        printf("MCR update successful!\n");
        system("rm -rf mcr_update");
        exit(0);
    }
    else if (strcmp(argv[1], "--version") == 0){
        printf("Current MCR version: %.1f\n", MCR_VERSION);
        exit(0);
    }
    parse_macroses();
    struct macro tmp;
    strcpy(tmp.key, argv[1]);
    struct macro* macros = hashmap_get(macroses, &tmp);
    if (!macros){
        fprintf(stderr, "Invalid macros! Please initialize first.\n");
        exit(1);
    }
    int32_t length = 0;
    char need_schars[argc - 2];
    if (argc >= 3){
        for (int i = 2; i < argc; i++){
            length += strlen(argv[i]) + 1;
            char* res = strchr(argv[i], ' ');
            if (res == NULL) continue;
            need_schars[i - 2] = 1;
        }
        length--;
    }
    length += strlen(macros->macros) + 1;
    for (int i = 0; i < argc - 2; i++)
        if (need_schars[i] == 1) length += 2;
    char* finalcommand = (char*)calloc(length, sizeof(char));
    if (!finalcommand){
        fprintf(stderr, "Allocation failure!\n");
        return 1;
    }
    strcpy(finalcommand, macros->macros);
    finalcommand[strlen(macros->macros)] = ' ';
    int32_t offset = strlen(macros->macros) + 1;
    for (int i = 2; i < argc; i++){
        if (need_schars[i - 2] == 1)
            finalcommand[offset++] = '"';
        strcat(finalcommand, argv[i]);
        offset += strlen(argv[i]);
        if (need_schars[i - 2] == 1)
            finalcommand[offset++] = '"';
        finalcommand[offset++] = ' ';
    }
    finalcommand[offset - 1] = '\0';
    char buff[length];
    memcpy(&buff, finalcommand, length + 2);
    free(finalcommand);
    hashmap_free(macroses);
    return system(buff);
}
