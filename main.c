#include <math.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include "threads.h"

const int tileSize = 20;

const int cellWidth = 1;
const int cellHeight = 1;

static int gridWidth = 500;
static int gridHeight = 500;

typedef struct GetResult {
	bool success;
	bool* ptr;
} GetResult;

bool inBounds(int x, int y) {
	return x >= 0 && x < gridWidth && y >= 0 && y < gridHeight;
}

GetResult get(bool* grid, int x, int y) {
	if (!inBounds(x, y)) {
		GetResult v = {false, NULL};
		return v;
	}

	GetResult v = {true, &grid[x * gridWidth + y]};
	return v;
}

bool getUnwrap(bool* grid, int x, int y) {
	GetResult res = get(grid, x, y);
	if (!res.success) {
		return false;
	}
	return *res.ptr;
}

void drawGrid(bool* old, bool* new, Color col) {
	for (int x = 0; x < gridWidth; x++) {
		for (int y = 0; y < gridHeight; y++) {
			bool res = getUnwrap(new, x, y);

			bool oldRes = getUnwrap(old, x, y);

			// Only draw if cell has changed
			if (res == oldRes) {
				continue;
			}
			
			if (res)
				DrawRectangle(x*cellWidth, y*cellHeight, cellWidth, cellHeight, col);
			else
				DrawRectangle(x*cellWidth, y*cellHeight, cellWidth, cellHeight, BLACK);
		}
	}
}

typedef struct WorkerData {
	bool* old;
	bool* new;

	int minX;
	int maxX;
	int minY;
	int maxY;
} WorkerData;

void* worker(void* arg) {
	WorkerData* data = (WorkerData*)arg;

	for (int x = data->minX; x < data->maxX; x++) {
		for (int y = data->minY; y < data->maxY; y++) {
			int neighbors = 0;

			bool oldState = getUnwrap(data->old, x, y);

			neighbors += getUnwrap(data->old, x - 1, y - 1);
			neighbors += getUnwrap(data->old, x, y - 1);
			neighbors += getUnwrap(data->old, x + 1, y - 1);

			neighbors += getUnwrap(data->old, x - 1, y);
			// Current cell
			neighbors += getUnwrap(data->old, x + 1, y);

			neighbors += getUnwrap(data->old, x - 1, y + 1);
			neighbors += getUnwrap(data->old, x, y + 1);
			neighbors += getUnwrap(data->old, x + 1, y + 1);
			
			GetResult res = get(data->new, x, y);
			if (!res.success) {
				printf("Failed to get (%d,%d)\n", x, y);
				exit(1);
			}
			*res.ptr = oldState;

			if (oldState) {
				if (neighbors < 2) {
					GetResult res = get(data->new, x, y);
					*res.ptr = false;
				} else if (neighbors == 2 || neighbors == 3) {
					GetResult res = get(data->new, x, y);
					*res.ptr = true;
				} else if (neighbors > 3) {
					GetResult res = get(data->new, x, y);
					*res.ptr = false;
				}
			} else {
				if (neighbors == 3) {
					GetResult res = get(data->new, x, y);
					*res.ptr = true;
				}
			}
		}
	}
	
	return NULL;
}

void gameStep(bool* old, bool* new) {
	int threadCount = 12;

	thread_t* threadHandles = malloc(threadCount*sizeof(thread_t));

	WorkerData* workerDataHandles = malloc(threadCount*sizeof(WorkerData));

	double div = gridWidth/(double)threadCount;
	for (int i = 0; i < threadCount; i++) {
		WorkerData d = {
			old,
			new,
			(int)ceil(i*div),
			(int)ceil((i+1)*div),
			0,
			gridHeight
		};
		workerDataHandles[i] = d;
		thread_create(&threadHandles[i], worker, (void*)&workerDataHandles[i]);
	}
	

	for (int i = 0; i < threadCount; i++) {
		thread_join(threadHandles[i]);
	}

	free(threadHandles);
	free(workerDataHandles);
}


bool* currentGrid(bool* grid1, bool* grid2, bool flipped) {
	return flipped ? grid2 : grid1;
}

int main() {
	bool* grid1 = (bool*)malloc(gridWidth*gridHeight*sizeof(bool));
	bool* grid2 = (bool*)malloc(gridWidth*gridHeight*sizeof(bool));

	bool flipped = false;
	bool paused = false;

	const int windowWidth = cellWidth*gridWidth;
	const int windowHeight = cellWidth*gridHeight;

	InitWindow(windowWidth, windowHeight, "Game of Life");
	SetTargetFPS(99999999);

	RenderTexture2D tex = LoadRenderTexture(windowWidth, windowHeight);
	Rectangle buffer_rec = {0.0, 0.0, windowWidth, -windowHeight};
	Vector2 buffer_vec = {0.0, 0.0};

	
	while (!WindowShouldClose()) {
		if (!paused) {
			if (flipped)
				gameStep(grid2, grid1);
			else
				gameStep(grid1, grid2);

			flipped = !flipped;
		}

		if (IsKeyPressed(KEY_R)) {
			for (int x = 0; x < gridWidth; x++) {
				for (int y = 0; y < gridHeight; y++) {
					GetResult res = get(currentGrid(grid1, grid2, flipped), x, y);
					if (!res.success) {
						return 1;
					}
					*res.ptr = (rand()/(double)RAND_MAX) > (double)0.5;
				}
			}
		} 
		
		if (IsKeyPressed(KEY_P)) {
			paused = !paused;
		}

		if (IsKeyDown(KEY_T)) {
			int cell_x = GetMouseX()/cellWidth;
			int cell_y = GetMouseY()/cellHeight;
			if (inBounds(cell_x, cell_y)) {
				bool* ptr = get(currentGrid(grid1, grid2, flipped), cell_x, cell_y).ptr;
				*ptr = true;
			}
		}
		
		if (IsKeyDown(KEY_Y)) {
			int cell_x = GetMouseX()/cellWidth;
			int cell_y = GetMouseY()/cellHeight;
			if (inBounds(cell_x, cell_y)) {
				bool* ptr = get(currentGrid(grid1, grid2, flipped), cell_x, cell_y).ptr;
				*ptr = false;
			}
		}

		BeginTextureMode(tex);
		
		drawGrid(
			flipped ? grid1 : grid2,
			flipped ? grid2 : grid1,
			paused ? DARKGRAY : WHITE
		);

		EndTextureMode();

		
		BeginDrawing();

			ClearBackground(BLACK);
			DrawTextureRec(tex.texture, buffer_rec, buffer_vec, WHITE);
			DrawFPS(10, 10);

		EndDrawing();
	}


	UnloadRenderTexture(tex);

	free(grid1);
	free(grid2);
}
