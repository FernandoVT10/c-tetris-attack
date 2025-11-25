#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"
#include "CCFuncs.h"

#define BLOCKS_SPRITESHEET_FILE "./assets/blocks.png"

// how many columns per row should contain a panel
#define PANEL_NUM_OF_COLS 6
#define PANEL_NUM_OF_ROWS 12

// the time that needs to be waited to make the blocks fall
#define PANEL_BLOCK_FALLING_TIME 0.05

#define MIN(a, b) ((a) > (b) ? (b) : (a))
#define MAX(a, b) ((a) < (b) ? (b) : (a))

typedef enum {
    PANEL_BLOCK_NONE = 0,
    PANEL_BLOCK_YELLOW,
    PANEL_BLOCK_RED,
    PANEL_BLOCK_PURPLE,
    PANEL_BLOCK_GREEN,
    PANEL_BLOCK_BLUE,
    PANEL_BLOCK_DARK_BLUE,
} PanelBlockType;

typedef struct {
    PanelBlockType type;
    float currentY; // used to fall smoothly
    bool inCombo;
    bool addedToCombo; // used to avoid creating new combos
} PanelBlock;

typedef struct {
    PanelBlock items[PANEL_NUM_OF_COLS];
} PanelRow;

typedef struct {
    PanelBlock **items; // All the blocks positions that conform a combo
    size_t count;
    size_t capacity;
    float time;
} Combo;

typedef struct {
    Vector2 pos;
    Vector2 size;
    PanelBlock blocks[PANEL_NUM_OF_ROWS][PANEL_NUM_OF_COLS];

    struct {
        int x;
        int y;
    } cursor;

    float fallingTime; // used to count when the block should fall

    struct {
        Combo *items;
        size_t count;
        size_t capacity;
    } combos;
} Panel;

Texture2D blocks;

PanelBlock *get_block(Panel *panel, int row, int col) {
    return &panel->blocks[row][col];
}

void gravity(Panel *panel) {
    panel->fallingTime += GetFrameTime();

    if(panel->fallingTime < PANEL_BLOCK_FALLING_TIME) return;

    panel->fallingTime = 0;

    // iterate from panel bottom to top
    // we skip the bottom row since gravity will not affect it
    for(int row = PANEL_NUM_OF_ROWS - 2; row >= 0; row--) {
        for(int col = 0; col < PANEL_NUM_OF_COLS; col++) {
            PanelBlock *block = get_block(panel, row, col);
            if(block->type == PANEL_BLOCK_NONE || block->inCombo) continue;

            PanelBlock *botBlock = get_block(panel, row + 1, col);
            if(botBlock->type != PANEL_BLOCK_NONE) continue;

            PanelBlock temp = *block;
            *block = *botBlock;
            *botBlock = temp;
        }
    }
}

bool is_block_comboable(Panel *panel, int row, int col, PanelBlockType t) {
    if(row >= PANEL_NUM_OF_ROWS || row < 0 || col >= PANEL_NUM_OF_COLS || col < 0)
        return false;

    PanelBlock b = panel->blocks[row][col];
    return !b.addedToCombo && b.type == t;
}

void panel_update(Panel *panel) {
    gravity(panel);

    for(int row = 0; row < PANEL_NUM_OF_ROWS; row++) {
        for(int col = 0; col < PANEL_NUM_OF_COLS; col++) {
            PanelBlock *curBlock = get_block(panel, row, col);
            if(curBlock->type == PANEL_BLOCK_NONE || curBlock->addedToCombo) continue;

            int xCount = 1;
            while(is_block_comboable(panel, row, col + xCount, curBlock->type)) xCount++;

            int yCount = 1;
            while(is_block_comboable(panel, row + yCount, col, curBlock->type)) yCount++;

            if(!curBlock->inCombo && (xCount >= 3 || yCount >= 3)) {
                curBlock->inCombo = true;
            }

            if(xCount >= 3) {
                while(xCount > 1) {
                    PanelBlock *b = get_block(panel, row, col + xCount - 1);
                    b->inCombo = curBlock->inCombo;
                    xCount--;
                }
            }

            if(yCount >= 3) {
                while(yCount > 1) {
                    PanelBlock *b = get_block(panel, row + yCount - 1, col);
                    b->inCombo = curBlock->inCombo;
                    yCount--;
                }
            }
        }
    }

    Combo combo = {0};

    for(int row = 0; row < PANEL_NUM_OF_ROWS; row++) {
        for(int col = 0; col < PANEL_NUM_OF_COLS; col++) {
            PanelBlock *block = get_block(panel, row, col);
            if(!block->inCombo || block->addedToCombo) continue;
            block->addedToCombo = true;
            da_append(&combo, block);
        }
    }

    if(combo.count > 0) {
        da_append(&panel->combos, combo);
    }
}

void draw_block(PanelBlock *block, int x, int y, int width, int height) {
    if(block->inCombo) return;

    // TODO: this should be improved in the future
    Rectangle rec = {
        .x = 0,
        .y = 0,
        .width = 100,
        .height = 100,
    };

    switch(block->type) {
        case PANEL_BLOCK_YELLOW:    rec.x = 0;   break;
        case PANEL_BLOCK_RED:       rec.x = 100; break;
        case PANEL_BLOCK_PURPLE:    rec.x = 200; break;
        case PANEL_BLOCK_GREEN:     rec.x = 300; break;
        case PANEL_BLOCK_BLUE:      rec.x = 400; break;
        case PANEL_BLOCK_DARK_BLUE: rec.x = 500; break;
        default: return;
    }

    Rectangle dest = {
        .x = x,
        .y = y,
        .width = width,
        .height = height,
    };
    DrawTexturePro(blocks, rec, dest, (Vector2){0, 0}, 0, WHITE);

    if(block->inCombo) {
        DrawRectangle(x, y, width, height, (Color){255, 255, 255, 120});
    }
}

void panel_draw(Panel *panel) {
    int blockWidth = panel->size.x / PANEL_NUM_OF_COLS;
    int blockHeight = panel->size.y / PANEL_NUM_OF_ROWS;

    float dt = GetFrameTime();

    for(int row = 0; row < PANEL_NUM_OF_ROWS; row++) {
        for(int col = 0; col < PANEL_NUM_OF_COLS; col++) {
            PanelBlock *block = get_block(panel, row, col);

            if(block->currentY < row) {
                // the falling animation time should be the same as the actual falling
                block->currentY += 1.0 / PANEL_BLOCK_FALLING_TIME * dt;
            } else if(block->currentY > row) {
                block->currentY = row;
            }

            int posX = panel->pos.x + blockWidth * col;
            int posY = panel->pos.y + blockHeight * block->currentY;

            draw_block(block, posX, posY, blockWidth, blockHeight);
        }
    }

    Rectangle cursorRec = {
        .x = panel->cursor.x * blockWidth,
        .y = panel->cursor.y * blockHeight,
        .width = blockWidth * 2,
        .height = blockHeight,
    };
    DrawRectangleLinesEx(cursorRec, 3, WHITE);
}

void panel_cursor_swap(Panel *panel) {
    int cursorX = panel->cursor.x;
    int cursorY = panel->cursor.y;

    PanelBlock *left = get_block(panel, cursorY, cursorX);
    if(left->inCombo) return;
    PanelBlock *right = get_block(panel, cursorY, cursorX + 1);
    if(right->inCombo) return;

    PanelBlock temp = *left;

    *left = *right;
    *right = temp;
}

void player_controller(Panel *panel) {
    if(IsKeyPressed(KEY_RIGHT)) {
        // here we substract by 2 because the cursor's length is 2 blocks
        panel->cursor.x = MIN(panel->cursor.x + 1, PANEL_NUM_OF_COLS - 2);
    } else if(IsKeyPressed(KEY_LEFT)) {
        panel->cursor.x = MAX(panel->cursor.x - 1, 0);
    }

    if(IsKeyPressed(KEY_DOWN)) {
        panel->cursor.y = MIN(panel->cursor.y + 1, PANEL_NUM_OF_ROWS - 1);
    } else if(IsKeyPressed(KEY_UP)) {
        panel->cursor.y = MAX(panel->cursor.y - 1, 0);
    }

    if(IsKeyPressed(KEY_X)) {
        panel_cursor_swap(panel);
    }

    // DEBUG
    if(IsKeyPressed(KEY_S)) {
        panel->blocks[0][0].type = PANEL_BLOCK_DARK_BLUE;
        panel->blocks[0][0].currentY = 0;
        panel->blocks[1][0].type = PANEL_BLOCK_GREEN;
        panel->blocks[1][0].currentY = 1;
    }
}

int main(void) {
    InitWindow(1280, 720, "C Tetris Attack");
    SetTargetFPS(60);

    Panel panel = {
        .pos = {0, 0},
        .size = {370, 720}
    };

    srand(time(NULL));

    blocks = LoadTexture(BLOCKS_SPRITESHEET_FILE);

    for(int i = 11; i > 5; i--) {
        for(int j = 0; j < PANEL_NUM_OF_COLS; j++) {
            panel.blocks[i][j].type = rand() % 6 + 1;
            panel.blocks[i][j].currentY = i;
        }
    }

    while(!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        panel_update(&panel);
        panel_draw(&panel);
        player_controller(&panel);

        float dt = GetFrameTime();

        for(size_t i = 0; i < panel.combos.count; i++) {
            Combo *combo = &panel.combos.items[i];
            combo->time += dt;

            if(combo->time >= 1) {
                for(size_t i = 0; i < combo->count; i++) {
                    PanelBlock *block = combo->items[i];
                    *block = (PanelBlock){0};
                }

                da_free(combo);
                da_remove_unordered(&panel.combos, i);

                if(i > 0) i--;
            }
        }

        // for(size_t i = 0; i < panel.combos.count; i++) {
        //     Combo combo = panel.combos.items[i];
        //
        //     for(size_t j = 0; j < combo.count; j++) {
        //         PanelBlock *block = combo.items[j];
        //         int posX = panel->pos.x + 60 * col;
        //         int posY = panel->pos.y + 60 * block->currentY;
        //         draw_block(block, posX, posY, 60, 60);
        //     }
        // }

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
