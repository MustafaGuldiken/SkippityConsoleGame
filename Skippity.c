#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 
#include <time.h>

#define BOARD_SIZE_MAX 20
#define LETTERS_COUNT 5

//bilgisayara karsi oynanan oyun https://youtu.be/g_-rrPrrCxo
// Oyunculara Karsi oynanan oyun https://youtu.be/CF2wF5UB80I
// c kodunun aciklanmasi https://youtu.be/dESL6hlNMvA
//kısa oyun + oyunun sonlanmasını göremek için videonun son kısmına bakabilirsiniz https://youtu.be/llEvN5mFfMk

typedef struct {
    char** board;
    char** savedLetters; // To store letters for undo
    int playerScores[2][LETTERS_COUNT]; // Separate scoreboards for player 1 and player 2
    int size;
} Game;

void initializeGame(Game* game, int size) {
    int i, j;
    game->size = size;
    game->board = malloc(size * sizeof(char*));
    game->savedLetters = malloc(size * sizeof(char*));
    for (i = 0; i < size; i++) {
        game->board[i] = malloc(size * sizeof(char));
        game->savedLetters[i] = malloc(size * sizeof(char));
        for (j = 0; j < size; j++) {
            if ((i >= size / 2 - 1 && i <= size / 2) && (j >= size / 2 - 1 && j <= size / 2)) {
                game->board[i][j] = '-'; // Leave middle 4 cells blank
            } else {
                int letterIndex = rand() % LETTERS_COUNT; // Random index for the letters
                game->board[i][j] = 'A' + letterIndex; // Assign random letter
            }
            game->savedLetters[i][j] = '-'; // Initialize savedLetters with '-'
        }
    }

    for (i = 0; i < LETTERS_COUNT; i++) {
        game->playerScores[0][i] = 0; // Initialize player 1 scores
        game->playerScores[1][i] = 0; // Initialize player 2 scores
    }
}
bool tryToCompleteSet(Game* game, int currentPlayer, char targetStone);
bool checkAndPerformJump(Game* game, int x, int y, int currentPlayer, char stone);
void collectMostStones(Game* game, int currentPlayer);

void restoreGameFromSave(Game* game, int* gameMode, int* currentPlayer, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Unable to open file for reading.\n");
        return;
    }

    fscanf(file, "%d %d %d", &game->size, gameMode, currentPlayer);  // Oyun boyutunu, oyun modunu ve mevcut oyuncuyu oku
    game->board = malloc(game->size * sizeof(char*));
    game->savedLetters = malloc(game->size * sizeof(char*));
    int i, j;
    for (i = 0; i < game->size; i++) {
        game->board[i] = malloc(game->size * sizeof(char));
        game->savedLetters[i] = malloc(game->size * sizeof(char));
        for (j = 0; j < game->size; j++) {
            fscanf(file, " %c", &game->board[i][j]);
            game->savedLetters[i][j] = '-';  // Kaydedilmiş oyunu geri yüklerken savedLetters'ı boş bırak
        }
    }

    int k, l;
    for (k = 0; k < 2; k++) {
        for (l = 0; l < LETTERS_COUNT; l++) {
            fscanf(file, "%d", &game->playerScores[k][l]);
        }
    }

    fclose(file);
    printf("Game loaded from %s\n", filename);
}


void printBoard(Game* game) {
    int i, j;
    printf("\n  ");
    for (i = 0; i < game->size; i++) {
        printf("%2d", i);
    }
    printf("\n");
    for (i = 0; i < game->size; i++) {
        printf("%2d", i);
        for (j = 0; j < game->size; j++) {
            printf(" %c", game->board[i][j]);
        }
        printf("\n");
    }
}

void moveStone(Game* game, int fromX, int fromY, int toX, int toY, int currentPlayer) {
    // Hareket ge erli mi diye kontrol et (yaln zca d z hareket, arada ta  var ve hedef bo  olmal )
    if ((abs(fromX - toX) == 2 && fromY == toY) || (abs(fromY - toY) == 2 && fromX == toX)) {
        int middleX = (fromX + toX) / 2;
        int middleY = (fromY + toY) / 2;
        char jumpedStone = game->board[middleX][middleY];
        char destination = game->board[toX][toY];
        
        if (jumpedStone != '-' && destination == '-') {
            // Hareketin ge ici olarak kaydedilmesi
            game->savedLetters[fromX][fromY] = game->board[fromX][fromY];  // Ta  n eski konumundaki harf
            game->savedLetters[middleX][middleY] = jumpedStone;            // Atlad   m z ta 
            game->savedLetters[toX][toY] = '-';                            // Hedef konumdaki bo  alan

            // Ta  n yeni konuma ta  nmas  ve eski yerlerinin temizlenmesi
            game->board[toX][toY] = game->board[fromX][fromY];
            game->board[fromX][fromY] = '-';
            game->board[middleX][middleY] = '-';

            // Skor g ncellemesi
            int letterIndex = jumpedStone - 'A';
            game->playerScores[currentPlayer][letterIndex]++;

            printf("Player %d jumped over a %c from (%d, %d) to (%d, %d).\n", currentPlayer + 1, jumpedStone, fromX, fromY, toX, toY);
        } else {
            printf("Invalid move: No stone to jump over or destination is not empty.\n");
        }
    } else {
        printf("Invalid move: Moves must be straight and jump exactly one stone.\n");
    }
}



void undoMove(Game* game, int fromX, int fromY, int toX, int toY, int currentPlayer) {
    // Hedef konumundaki ta  n eski haline d nd r lmesi
    game->board[toX][toY] = game->savedLetters[toX][toY];
    
    // Atlad   m z ta  n eski haline d nd r lmesi
    int middleX = (fromX + toX) / 2;
    int middleY = (fromY + toY) / 2;
    char middleStone = game->savedLetters[middleX][middleY];
    game->board[middleX][middleY] = middleStone;
    
    // Ta  n ba lang   konumunun eski haline d nd r lmesi
    game->board[fromX][fromY] = game->savedLetters[fromX][fromY];

    // Skorun geri al nmas 
    if (middleStone != '-') {
        int letterIndex = middleStone - 'A';
        game->playerScores[currentPlayer][letterIndex]--;
    }

    printf("Undo move: Stone returned from (%d, %d) to (%d, %d) and the jumped stone restored at (%d, %d).\n", toX, toY, fromX, fromY, middleX, middleY);
}



int askForUndo(Game* game, int currentPlayer, int fromX, int fromY, int toX, int toY) {
    char choice;
    printf("Player %d, do you want to undo your move? (y/n): ", currentPlayer + 1);
    scanf(" %c", &choice);

    if (choice == 'y' || choice == 'Y') {
        undoMove(game, fromX, fromY, toX, toY, currentPlayer);  // Pass currentPlayer to undoMove
        displayRestoredBoard(game);  // Undo requested, restore saved letters
        return 1;  // Undo requested
    }
    return 0;  // Undo not requested
}



void displayRestoredBoard(Game* game) {
    int i, j;
    printf("\nRestored Board:\n");
    printf("  ");
    for (i = 0; i < game->size; i++) {
        printf("%2d", i);
    }
    printf("\n");
    for (i = 0; i < game->size; i++) {
        printf("%2d", i);
        for (j = 0; j < game->size; j++) {
            printf(" %c", game->board[i][j]);
        }
        printf("\n");
    }
}

void displayScoreboard(Game* game, int currentPlayer) {
    int i;
    printf("\nCurrent Scores:\n");
    for (i = 0; i < LETTERS_COUNT; i++) {
        printf("Player 1 - %c: %d\tPlayer 2 - %c: %d\n", 'A' + i, game->playerScores[0][i], 'A' + i, game->playerScores[1][i]);
    }
}

void makeMultipleMoves(Game* game, int currentPlayer) {
    int numMoves;
    printf("Enter the number of moves: ");
    scanf("%d", &numMoves);
    int move;
    for (move = 0; move < numMoves; move++) {
        int fromX, fromY, toX, toY;
        printf("Enter coordinates of stone %d to move (fromX fromY): ", move + 1);
        scanf("%d %d", &fromX, &fromY);
        printf("Enter coordinates to place stone %d (toX toY): ", move + 1);
        scanf("%d %d", &toX, &toY);

        if (game->board[fromX][fromY] == '-' || game->board[toX][toY] != '-') {
            printf("Invalid move!\n");
            move--; // Stay on the same move index
        } else {
            moveStone(game, fromX, fromY, toX, toY, currentPlayer);
            if (askForUndo(game, currentPlayer, fromX, fromY, toX, toY)) {
                move--; // Undo requested, stay on the same move index
            }
        }
    }
}

int canMove(Game* game, int currentPlayer) {
    int i, j;
    for (i = 0; i < game->size; i++) {
        for (j = 0; j < game->size; j++) {
            if (game->board[i][j] != '-') {
                // Check if there is an available move for the current player
                if ((i >= 2 && game->board[i - 1][j] != '-' && game->board[i - 2][j] == '-') ||
                    (i <= game->size - 3 && game->board[i + 1][j] != '-' && game->board[i + 2][j] == '-')) {
                    return 1; // Can move
                }
                if ((j >= 2 && game->board[i][j - 1] != '-' && game->board[i][j - 2] == '-') ||
                    (j <= game->size - 3 && game->board[i][j + 1] != '-' && game->board[i][j + 2] == '-')) {
                    return 1; // Can move
                }
            }
        }
    }
    return 0; // No available move
}
void saveGameToTextFile(Game* game, int gameMode, int currentPlayer, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        printf("Unable to open file for writing.\n");
        return;
    }

    // Oyun boyutunu, oyun modunu ve mevcut oyuncuyu kaydet
    fprintf(file, "%d %d %d\n", game->size, gameMode, currentPlayer);
    int i, j;
    for (i = 0; i < game->size; i++) {
        for (j = 0; j < game->size; j++) {
            fprintf(file, "%c ", game->board[i][j]);
        }
        fprintf(file, "\n");
    }

    // Oyuncu skorlarını kaydet
    int k, l;
    for (k = 0; k < 2; k++) {
        for (l = 0; l < LETTERS_COUNT; l++) {
            fprintf(file, "%d ", game->playerScores[k][l]);
        }
        fprintf(file, "\n");
    }

    fclose(file);
    printf("Game saved to %s\n", filename);
}

void loadGameFromTextFile(Game* game, int* gameMode, int* currentPlayer, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Unable to open file for reading.\n");
        return;
    }

    int size;
    fscanf(file, "%d %d %d", &size, gameMode, currentPlayer);

    char** boardState = malloc(size * sizeof(char*));
    int playerScores[2][LETTERS_COUNT];
	int i,j;
    for ( i = 0; i < size; i++) {
        boardState[i] = malloc(size * sizeof(char));
        for ( j = 0; j < size; j++) {
            fscanf(file, " %c", &boardState[i][j]);
        }
    }
	int k,l;
    for ( k = 0; k < 2; k++) {
        for ( l = 0; l < LETTERS_COUNT; l++) {
            fscanf(file, "%d", &playerScores[k][l]);
        }
    }

    fclose(file);

    restoreGameFromSave(game, size, boardState, playerScores);

    // Clean up temporary board state memory
    int n;
    for (n = 0; n < size; n++) {
        free(boardState[n]);
    }
    free(boardState);

    printf("Game loaded from %s\n", filename);
}

void determineWinner(Game* game) {
    int totalStones[2] = {0, 0};
    int minSetScore[2] = {INT_MAX, INT_MAX};

    // Her oyuncu i in set skoru ve toplam ta  say s n  hesapla
    int player, stoneType;
    for ( player = 0; player < 2; player++) {
        for ( stoneType = 0; stoneType < LETTERS_COUNT; stoneType++) {
            totalStones[player] += game->playerScores[player][stoneType];
            if (game->playerScores[player][stoneType] < minSetScore[player]) {
                minSetScore[player] = game->playerScores[player][stoneType];
            }
        }
    }

    // Kazanan  belirle
    printf("\nPlayer 1's Set Score: %d, Total Stones: %d\n", minSetScore[0], totalStones[0]);
    printf("Player 2's Set Score: %d, Total Stones: %d\n", minSetScore[1], totalStones[1]);

    if (minSetScore[0] > minSetScore[1]) {
        printf("Player 1 wins by set score!\n");
    } else if (minSetScore[1] > minSetScore[0]) {
        printf("Player 2 wins by set score!\n");
    } else { // Set skorlar  e it, toplam ta  say s na bak
        if (totalStones[0] > totalStones[1]) {
            printf("Player 1 wins by total stones collected!\n");
        } else if (totalStones[1] > totalStones[0]) {
            printf("Player 2 wins by total stones collected!\n");
        } else {
            printf("The game ends in a draw!\n");
        }
    }
}
void computerMove(Game* game, int currentPlayer) {
    int counts[LETTERS_COUNT];
    int minCount = game->playerScores[currentPlayer][0];  //  lk ta  t r n n say s n  ba lang   de eri olarak al
    int totalStones = 0;

    // Her ta  t r n n say s n  hesapla ve en d   k say y  bul
    int i;
    for ( i = 0; i < LETTERS_COUNT; i++) {
        counts[i] = game->playerScores[currentPlayer][i];
        totalStones += counts[i];
        if (counts[i] < minCount) {
            minCount = counts[i];
        }
    }

    // En az say da olan ta  t rlerini belirle
    int j;
    int minTypes[LETTERS_COUNT], minTypesCount = 0;
    for ( j = 0; j < LETTERS_COUNT; j++) {
        if (counts[j] == minCount) {
            minTypes[minTypesCount++] = j;
        }
    }

    // E er bir tane en az say da ta  varsa onu tamamlamaya  al  
    if (minTypesCount == 1) {
        char targetStone = 'A' + minTypes[0];
        if (!tryToCompleteSet(game, currentPlayer, targetStone)) {
            // Tamamlama ba ar s z olursa en  ok ta   almaya  al  
            collectMostStones(game, currentPlayer);
        }
    } else {
        // E er birden fazla en az say da ta  t r  varsa en  ok ta  toplamaya y nelik oyna
        collectMostStones(game, currentPlayer);
    }
}
bool tryToCompleteSet(Game* game, int currentPlayer, char targetStone) {
    bool moveMade = false;
    // Hedef ta  n etraf ndaki ta lar  ve bo  alanlar  kontrol et
    int i,j;
    for ( i = 0; i < game->size; i++) {
        for ( j = 0; j < game->size; j++) {
            if (game->board[i][j] == targetStone) {
                // Hedef ta  bulundu,  imdi etraf ndaki potansiyel hareketleri kontrol et
                if (checkAndPerformJump(game, i, j, currentPlayer, targetStone)) {
                    moveMade = true;
                    break;
                }
            }
        }
        if (moveMade) break;
    }
    return moveMade;
}

bool checkAndPerformJump(Game* game, int x, int y, int currentPlayer, char stone) {
    int dx[4] = {-2, 2, 0, 0};
    int dy[4] = {0, 0, -2, 2};
    int mx[4] = {-1, 1, 0, 0};
    int my[4] = {0, 0, -1, 1};
	int dir;
    for ( dir = 0; dir < 4; dir++) {
        int nx = x + dx[dir], ny = y + dy[dir];
        int midX = x + mx[dir], midY = y + my[dir];
        if (nx >= 0 && nx < game->size && ny >= 0 && ny < game->size && game->board[nx][ny] == '-' && game->board[midX][midY] != '-' && game->board[midX][midY] != stone) {
            // Atlamadan  nce  zerinden atlanan ta  n t r n  sakla
            char jumpedStone = game->board[midX][midY];
            
            // Hareketi yap ve ta  n  zerinden atlama i lemini ger ekle tir
            game->board[nx][ny] = stone;
            game->board[x][y] = '-';
            game->board[midX][midY] = '-';  // Atlad   m z ta   kald r

            // Skoru g ncelle:  zerinden atlad   m z ta  n indeksini kullan
            int jumpedStoneIndex = jumpedStone - 'A';
            game->playerScores[currentPlayer][jumpedStoneIndex]++;
            
            printf("Computer jumps from (%d,%d) over (%d,%d) to (%d,%d), jumping over %c\n", x, y, midX, midY, nx, ny, jumpedStone);
            return true;
        }
    }
    return false;
}




void collectMostStones(Game* game, int currentPlayer) {
    // Tahtadaki her ta  i in en iyi hareketi bulmaya  al  
    int i,j;
    for ( i = 0; i < game->size; i++) {
        for ( j = 0; j < game->size; j++) {
            if (game->board[i][j] != '-') {
                char stone = game->board[i][j];
                if (checkAndPerformJump(game, i, j, currentPlayer, stone)) {
                    return;  // Ba ar l  bir hareket yap ld , d ng den   k
                }
            }
        }
    }
}

int main() {
    srand(time(NULL));
    Game game;
    int size, loadChoice, saveChoice, gameMode, currentPlayer;
    char filename[100];

    printf("Do you want to load a game? (1 for yes, 0 for no): ");
    scanf("%d", &loadChoice);

    if (loadChoice) {
        printf("Enter filename to load: ");
        scanf("%s", filename);
        restoreGameFromSave(&game, &gameMode, &currentPlayer, filename);
    } else {
        printf("Select game mode (1 for single player vs computer, 2 for two players): ");
        scanf("%d", &gameMode);
        printf("Enter the size of the game board (up to %d): ", BOARD_SIZE_MAX);
        scanf("%d", &size);
        if (size > BOARD_SIZE_MAX || size <= 0) {
            printf("Invalid size! Exiting...\n");
            return 1;
        }
        initializeGame(&game, size);
        currentPlayer = 0;  // Yeni oyun için başlangıç oyuncusu
    }

    while (1) {
        printBoard(&game);
        printf("Do you want to save the game? (1 for yes, 0 for no): ");
        scanf("%d", &saveChoice);
        if (saveChoice) {
            printf("Enter filename to save: ");
            scanf("%s", filename);
            saveGameToTextFile(&game, gameMode, currentPlayer, filename);
            printf("Game saved. Do you want to continue playing? (1 for yes, 0 for no): ");
            scanf("%d", &saveChoice);
            if (!saveChoice) break; // Eğer kullanıcı devam etmek istemezse oyun döngüsünden çık
        }

        if (!canMove(&game, currentPlayer)) {
            printf("No more moves available.\n");
            break;  // No more moves available, end the game
        }
        if (gameMode == 1 && currentPlayer == 1) {  // Check if it's computer's turn
            computerMove(&game, currentPlayer);  // Computer makes a move
        } else {
            makeMultipleMoves(&game, currentPlayer);  // Human player makes moves
        }
        
        displayScoreboard(&game, currentPlayer);  // Display scores after each move
        currentPlayer = 1 - currentPlayer;  // Switch player
    }

    displayScoreboard(&game, currentPlayer); // Final scoreboard display
    determineWinner(&game);  // Kazananı belirle ve ilan et
    return 0;
}

