/*******************************************************************************************
*
*   raymob - Full HD Portrait Game Architecture with Modular Scene Structure
*
********************************************************************************************/

#include "raymob.h"
#include <raymath.h>

#define MAX(a, b) ((a)>(b)? (a) : (b))
#define MIN(a, b) ((a)<(b)? (a) : (b))

// ----------------------------------------------------------------------------------
// Scene Interface & Shared Game Context
// ----------------------------------------------------------------------------------
typedef struct Scene {
    void (*Init)(void);
    void (*Update)(float deltaTime, Vector2 mousePos);
    void (*Draw)(Vector2 mousePos, bool isPressed);
    void (*Unload)(void);
} Scene;

typedef struct GameContext {
    Font customFont;
    Texture2D ballTexture;
    float canvasWidth;
    float canvasHeight;
    int selectedLevel;
    int score;
    Vector2 playerPos;
    Vector2 coinPos;
    float ballRotation;
} GameContext;

static GameContext gCtx;

// Transition state management
typedef enum TransitionState {
    TRANSITION_OFF = 0,
    TRANSITION_FADE_OUT,
    TRANSITION_FADE_IN
} TransitionState;

static TransitionState transState = TRANSITION_OFF;
static Scene* transTargetScene = NULL;
static float transAlpha = 0.0f;
static float transSpeed = 3.0f;

// Forward declarations of scenes
static Scene StartScene;
static Scene LevelSelectScene;
static Scene GameplayScene;

// Scene Manager Functions
static Scene* currentScene = NULL;

static void SceneManager_SetScene(Scene* newScene)
{
    if (currentScene && currentScene->Unload)
    {
        currentScene->Unload();
    }

    currentScene = newScene;

    if (currentScene && currentScene->Init)
    {
        currentScene->Init();
    }
}

static void SceneManager_Update(float deltaTime, Vector2 mousePos)
{
    if (currentScene && currentScene->Update)
    {
        currentScene->Update(deltaTime, mousePos);
    }
}

static void SceneManager_Draw(Vector2 mousePos, bool isPressed)
{
    if (currentScene && currentScene->Draw)
    {
        currentScene->Draw(mousePos, isPressed);
    }
}

static void ChangeSceneTo(Scene* targetScene)
{
    if (transState == TRANSITION_OFF)
    {
        transTargetScene = targetScene;
        transState = TRANSITION_FADE_OUT;
        transAlpha = 0.0f;
    }
}

// ----------------------------------------------------------------------------------
// UI Drawing Helpers
// ----------------------------------------------------------------------------------
static void DrawTextCentered(Font font, const char* text, float y, float fontSize, float spacing, Color color, float canvasWidth)
{
    Vector2 textSize = MeasureTextEx(font, text, fontSize, spacing);
    Vector2 textPos = { (canvasWidth - textSize.x) / 2.0f, y };

    DrawTextEx(font, text, (Vector2){ textPos.x + 3.0f, textPos.y + 3.0f }, fontSize, spacing, (Color){ 0, 0, 0, 140 });
    DrawTextEx(font, text, textPos, fontSize, spacing, color);
}

static bool DrawButton(Font font, Rectangle rect, const char* text, Color baseColor, Color hoverColor, Color textColor, Vector2 mousePos, bool isPressed)
{
    bool hovered = CheckCollisionPointRec(mousePos, rect);
    Color btnColor = hovered ? hoverColor : baseColor;

    // Drop shadow
    Rectangle shadowRect = { rect.x + 8.0f, rect.y + 10.0f, rect.width, rect.height };
    DrawRectangleRounded(shadowRect, 0.25f, 12, (Color){ 0, 0, 0, 90 });

    // Button body
    DrawRectangleRounded(rect, 0.25f, 12, btnColor);
    DrawRectangleRoundedLinesEx(rect, 0.25f, 12, 5.0f, ColorAlpha(WHITE, 0.8f));

    float fontSize = rect.height * 0.38f;
    if (fontSize < 24.0f) fontSize = 24.0f;
    Vector2 textSize = MeasureTextEx(font, text, fontSize, 3.0f);
    Vector2 textPos = {
        rect.x + (rect.width - textSize.x) / 2.0f,
        rect.y + (rect.height - textSize.y) / 2.0f
    };

    DrawTextEx(font, text, (Vector2){ textPos.x + 3.0f, textPos.y + 3.0f }, fontSize, 3.0f, (Color){ 0, 0, 0, 140 });
    DrawTextEx(font, text, textPos, fontSize, 3.0f, textColor);

    return (hovered && isPressed);
}

// ----------------------------------------------------------------------------------
// SCENE 1: START SCENE
// ----------------------------------------------------------------------------------
static void StartScene_Init(void) {}

static void StartScene_Update(float deltaTime, Vector2 mousePos)
{
    (void)deltaTime;
    (void)mousePos;
}

static void StartScene_Draw(Vector2 mousePos, bool isPressed)
{
    // Background Gradient
    DrawRectangleGradientV(0, 0, (int)gCtx.canvasWidth, (int)gCtx.canvasHeight, (Color){ 18, 24, 44, 255 }, (Color){ 32, 44, 76, 255 });
    DrawCircleGradient((Vector2){ gCtx.canvasWidth / 2.0f, 960.0f }, 560.0f, (Color){ 45, 60, 110, 100 }, (Color){ 0, 0, 0, 0 });

    // Titles
    DrawTextCentered(gCtx.customFont, "RAYMOB", 360.0f, 100.0f, 5.0f, GOLD, gCtx.canvasWidth);
    DrawTextCentered(gCtx.customFont, "REBELLION", 480.0f, 80.0f, 4.0f, ORANGE, gCtx.canvasWidth);
    DrawTextCentered(gCtx.customFont, "Touch screen to Play", 600.0f, 40.0f, 3.0f, LIGHTGRAY, gCtx.canvasWidth);

    // Hero Emblem
    float pulse = sinf((float)GetTime() * 3.5f) * 28.0f;
    DrawCircle((int)gCtx.canvasWidth / 2, 940, 100.0f + pulse, (Color){ 140, 20, 50, 80 });
    DrawCircleLines((int)gCtx.canvasWidth / 2, 940, 96.0f + pulse * 0.5f, GOLD);

    // Ball Texture Menu Animation
    float menuBallRadius = 72.0f + pulse * 0.2f;
    Rectangle srcRec = { 0.0f, 0.0f, (float)gCtx.ballTexture.width, (float)gCtx.ballTexture.height };
    Rectangle dstRec = { gCtx.canvasWidth / 2.0f, 940.0f, menuBallRadius * 2.0f, menuBallRadius * 2.0f };
    Vector2 origin = { menuBallRadius, menuBallRadius };
    DrawTexturePro(gCtx.ballTexture, srcRec, dstRec, origin, (float)GetTime() * 45.0f, WHITE);

    // Start Button
    if (DrawButton(gCtx.customFont, (Rectangle){ gCtx.canvasWidth / 2.0f - 260.0f, 1360.0f, 520.0f, 150.0f }, "START GAME", DARKBLUE, BLUE, WHITE, mousePos, isPressed))
    {
        ChangeSceneTo(&LevelSelectScene);
    }
}

static void StartScene_Unload(void) {}

static Scene StartScene = {
    .Init = StartScene_Init,
    .Update = StartScene_Update,
    .Draw = StartScene_Draw,
    .Unload = StartScene_Unload
};

// ----------------------------------------------------------------------------------
// SCENE 2: LEVEL SELECT SCENE
// ----------------------------------------------------------------------------------
static void LevelSelectScene_Init(void) {}

static void LevelSelectScene_Update(float deltaTime, Vector2 mousePos)
{
    (void)deltaTime;
    (void)mousePos;
}

static void LevelSelectScene_Draw(Vector2 mousePos, bool isPressed)
{
    // Background Gradient
    DrawRectangleGradientV(0, 0, (int)gCtx.canvasWidth, (int)gCtx.canvasHeight, (Color){ 24, 32, 38, 255 }, (Color){ 38, 50, 58, 255 });

    // Header
    DrawTextCentered(gCtx.customFont, "SELECT LEVEL", 220.0f, 72.0f, 4.0f, RAYWHITE, gCtx.canvasWidth);
    DrawTextCentered(gCtx.customFont, "Choose difficulty speed", 310.0f, 40.0f, 3.0f, GRAY, gCtx.canvasWidth);

    // Level 1 Button
    if (DrawButton(gCtx.customFont, (Rectangle){ gCtx.canvasWidth / 2.0f - 300.0f, 520.0f, 600.0f, 210.0f }, "LEVEL 1\n(Normal)", DARKGREEN, LIME, WHITE, mousePos, isPressed))
    {
        gCtx.selectedLevel = 1;
        gCtx.score = 0;
        gCtx.playerPos = (Vector2){ gCtx.canvasWidth / 2.0f, gCtx.canvasHeight / 2.0f };
        ChangeSceneTo(&GameplayScene);
    }

    // Level 2 Button
    if (DrawButton(gCtx.customFont, (Rectangle){ gCtx.canvasWidth / 2.0f - 300.0f, 800.0f, 600.0f, 210.0f }, "LEVEL 2\n(Fast)", (Color){ 180, 100, 0, 255 }, ORANGE, WHITE, mousePos, isPressed))
    {
        gCtx.selectedLevel = 2;
        gCtx.score = 0;
        gCtx.playerPos = (Vector2){ gCtx.canvasWidth / 2.0f, gCtx.canvasHeight / 2.0f };
        ChangeSceneTo(&GameplayScene);
    }

    // Level 3 Button
    if (DrawButton(gCtx.customFont, (Rectangle){ gCtx.canvasWidth / 2.0f - 300.0f, 1080.0f, 600.0f, 210.0f }, "LEVEL 3\n(Extreme)", MAROON, RED, WHITE, mousePos, isPressed))
    {
        gCtx.selectedLevel = 3;
        gCtx.score = 0;
        gCtx.playerPos = (Vector2){ gCtx.canvasWidth / 2.0f, gCtx.canvasHeight / 2.0f };
        ChangeSceneTo(&GameplayScene);
    }

    // Back Button
    if (DrawButton(gCtx.customFont, (Rectangle){ 40.0f, 50.0f, 240.0f, 100.0f }, "< BACK", DARKGRAY, GRAY, WHITE, mousePos, isPressed))
    {
        ChangeSceneTo(&StartScene);
    }
}

static void LevelSelectScene_Unload(void) {}

static Scene LevelSelectScene = {
    .Init = LevelSelectScene_Init,
    .Update = LevelSelectScene_Update,
    .Draw = LevelSelectScene_Draw,
    .Unload = LevelSelectScene_Unload
};

// ----------------------------------------------------------------------------------
// SCENE 3: GAMEPLAY SCENE
// ----------------------------------------------------------------------------------
static void GameplayScene_Init(void) {}

static void GameplayScene_Update(float deltaTime, Vector2 mousePos)
{
    float lerpSpeed = 8.0f + (float)gCtx.selectedLevel * 4.0f;
    Vector2 oldPos = gCtx.playerPos;
    gCtx.playerPos = Vector2Lerp(gCtx.playerPos, mousePos, MIN(1.0f, lerpSpeed * deltaTime));

    // Spin ball
    float moveDist = Vector2Distance(oldPos, gCtx.playerPos);
    gCtx.ballRotation += moveDist * 3.0f;

    // Coin collection logic
    if (CheckCollisionCircles(gCtx.playerPos, 56.0f, gCtx.coinPos, 44.0f))
    {
        gCtx.score += 10 * gCtx.selectedLevel;
        gCtx.coinPos.x = (float)GetRandomValue(100, (int)gCtx.canvasWidth - 100);
        gCtx.coinPos.y = (float)GetRandomValue(260, (int)gCtx.canvasHeight - 160);
    }
}

static void GameplayScene_Draw(Vector2 mousePos, bool isPressed)
{
    // Dark Blue Space Background
    DrawRectangleGradientV(0, 0, (int)gCtx.canvasWidth, (int)gCtx.canvasHeight, (Color){ 10, 14, 24, 255 }, (Color){ 20, 26, 42, 255 });

    // Top UI Header Bar
    DrawRectangle(0, 0, (int)gCtx.canvasWidth, 160, (Color){ 28, 34, 52, 255 });
    DrawLineEx((Vector2){ 0, 160 }, (Vector2){ gCtx.canvasWidth, 160 }, 4.0f, (Color){ 60, 70, 90, 255 });

    // Menu / Back Button
    if (DrawButton(gCtx.customFont, (Rectangle){ 40.0f, 30.0f, 220.0f, 100.0f }, "MENU", DARKGRAY, RED, WHITE, mousePos, isPressed))
    {
        ChangeSceneTo(&LevelSelectScene);
    }

    // Level and Score Info
    DrawTextEx(gCtx.customFont, TextFormat("LVL %i", gCtx.selectedLevel), (Vector2){ 310.0f, 55.0f }, 48.0f, 4.0f, YELLOW);
    DrawTextEx(gCtx.customFont, TextFormat("SCORE: %i", gCtx.score), (Vector2){ gCtx.canvasWidth - 360.0f, 55.0f }, 48.0f, 4.0f, GREEN);

    // Guidance text
    DrawTextCentered(gCtx.customFont, "Touch & Drag to control player", 210.0f, 36.0f, 3.0f, (Color){ 100, 115, 140, 255 }, gCtx.canvasWidth);

    // Collectible Coin with Outer Pulse Ring
    float coinPulse = sinf((float)GetTime() * 6.0f) * 8.0f;
    DrawCircleV(gCtx.coinPos, 52.0f + coinPulse, (Color){ 255, 215, 0, 70 });
    DrawCircleV(gCtx.coinPos, 40.0f, GOLD);
    DrawCircleLinesV(gCtx.coinPos, 46.0f, YELLOW);
    DrawTextEx(gCtx.customFont, "$", (Vector2){ gCtx.coinPos.x - 12.0f, gCtx.coinPos.y - 24.0f }, 48.0f, 4.0f, BLACK);

    // Player Avatar
    float playerRadius = 56.0f;
    DrawCircleV(gCtx.playerPos, playerRadius + 16.0f, (Color){ 100, 200, 255, 60 });
    DrawCircleLinesV(gCtx.playerPos, playerRadius + 4.0f, ColorAlpha(WHITE, 0.8f));

    Rectangle srcRec = { 0.0f, 0.0f, (float)gCtx.ballTexture.width, (float)gCtx.ballTexture.height };
    Rectangle dstRec = { gCtx.playerPos.x, gCtx.playerPos.y, playerRadius * 2.0f, playerRadius * 2.0f };
    Vector2 origin = { playerRadius, playerRadius };
    DrawTexturePro(gCtx.ballTexture, srcRec, dstRec, origin, gCtx.ballRotation, WHITE);
}

static void GameplayScene_Unload(void) {}

static Scene GameplayScene = {
    .Init = GameplayScene_Init,
    .Update = GameplayScene_Update,
    .Draw = GameplayScene_Draw,
    .Unload = GameplayScene_Unload
};

// ----------------------------------------------------------------------------------
// Main Entry Point
// ----------------------------------------------------------------------------------
int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);

    const int screenWidth = 1080;
    const int screenHeight = 1920;

    InitWindow(screenWidth, screenHeight, "raymob - Modular Scene Game");
    SetWindowMinSize(360, 640);
    SetTargetFPS(60);

    gCtx.canvasWidth = 1080.0f;
    gCtx.canvasHeight = 1920.0f;

    RenderTexture2D target = LoadRenderTexture((int)gCtx.canvasWidth, (int)gCtx.canvasHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_TRILINEAR);

    // Load Resources
    gCtx.customFont = LoadFontEx("RebellionSquad-ZpprZ.ttf", 96, NULL, 0);
    SetTextureFilter(gCtx.customFont.texture, TEXTURE_FILTER_BILINEAR);

    gCtx.ballTexture = LoadTexture("ball.png");
    SetTextureFilter(gCtx.ballTexture, TEXTURE_FILTER_BILINEAR);

    gCtx.selectedLevel = 1;
    gCtx.playerPos = (Vector2){ gCtx.canvasWidth / 2.0f, gCtx.canvasHeight / 2.0f };
    gCtx.coinPos = (Vector2){ 540.0f, 760.0f };
    gCtx.score = 0;
    gCtx.ballRotation = 0.0f;

    // Start with StartScene
    SceneManager_SetScene(&StartScene);

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();

        // Scaling & Input Calculation
        float scale = MIN((float)GetScreenWidth() / gCtx.canvasWidth, (float)GetScreenHeight() / gCtx.canvasHeight);

        Vector2 mouse = GetMousePosition();
        Vector2 virtualMouse = { 0 };
        virtualMouse.x = (mouse.x - ((float)GetScreenWidth() - (gCtx.canvasWidth * scale)) * 0.5f) / scale;
        virtualMouse.y = (mouse.y - ((float)GetScreenHeight() - (gCtx.canvasHeight * scale)) * 0.5f) / scale;
        virtualMouse = Vector2Clamp(virtualMouse, (Vector2){ 0.0f, 0.0f }, (Vector2){ gCtx.canvasWidth, gCtx.canvasHeight });

        bool isPressed = (transState == TRANSITION_OFF) && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || GetGestureDetected() == GESTURE_TAP);

        // Transition Manager Update
        if (transState == TRANSITION_FADE_OUT)
        {
            transAlpha += transSpeed * deltaTime;
            if (transAlpha >= 1.0f)
            {
                transAlpha = 1.0f;
                SceneManager_SetScene(transTargetScene);
                transState = TRANSITION_FADE_IN;
            }
        }
        else if (transState == TRANSITION_FADE_IN)
        {
            transAlpha -= transSpeed * deltaTime;
            if (transAlpha <= 0.0f)
            {
                transAlpha = 0.0f;
                transState = TRANSITION_OFF;
            }
        }

        // Scene Logic Update
        if (transState == TRANSITION_OFF)
        {
            SceneManager_Update(deltaTime, virtualMouse);
        }

        // Draw Scene to Render Target
        BeginTextureMode(target);

        SceneManager_Draw(virtualMouse, isPressed);

        if (transState != TRANSITION_OFF)
        {
            DrawRectangle(0, 0, (int)gCtx.canvasWidth, (int)gCtx.canvasHeight, ColorAlpha(BLACK, transAlpha));
        }

        EndTextureMode();

        // Render Canvas Target to Display
        BeginDrawing();
        ClearBackground(BLACK);

        DrawTexturePro(target.texture,
                       (Rectangle){ 0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height },
                       (Rectangle){ ((float)GetScreenWidth() - (gCtx.canvasWidth * scale)) * 0.5f,
                                    ((float)GetScreenHeight() - (gCtx.canvasHeight * scale)) * 0.5f,
                                    gCtx.canvasWidth * scale,
                                    gCtx.canvasHeight * scale },
                       (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);

        EndDrawing();
    }

    // Cleanup
    if (currentScene && currentScene->Unload)
    {
        currentScene->Unload();
    }
    UnloadTexture(gCtx.ballTexture);
    UnloadFont(gCtx.customFont);
    UnloadRenderTexture(target);
    CloseWindow();

    return 0;
}
