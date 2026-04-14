#include "Uci.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "UciParser.h"
#include "data/TranspositionTable.h"

#define ENGINE_NAME "NullexC"
#define ENGINE_AUTHOR "Alberto Méndez Conrado"
#define ENGINE_VERSION "0.1"
#define ENGINE_GITHUB "https://github.com/albermencon/NullexC"

extern TranspositionTable TT;

void uci_loop(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    
    UciState state;
    uci_state_init(&state);

    char line[4096];
    while (fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\r\n")] = 0;
        
        if (strcmp(line, "uci") == 0) {
            printf("id name %s\n", ENGINE_NAME);
            printf("id author %s\n", ENGINE_AUTHOR);
            printf("id version %s\n", ENGINE_VERSION);
            printf("id github %s\n", ENGINE_GITHUB);
            printf("uciok\n");
        }
        else if (strcmp(line, "isready") == 0) {
            printf("readyok\n");
        }
        else if (strncmp(line, "position", 8) == 0) {
            parse_position(&state, line);
        }
        else if (strncmp(line, "go", 2) == 0) {
            parse_go(&state.pos, line);
        }
        else if (strcmp(line, "ucinewgame") == 0) {
            memset(TT.table, 0, TT.numCluster * sizeof(TTCluster));
            tt_newSearch(&TT); 
        }
        else if (strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) {
            break;
        }
    }
}
