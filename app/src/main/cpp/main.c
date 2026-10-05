/*******************************************************************************************
*
*   raymob - FullHD (1080x1920) Portrait Game Architecture with Custom Font & Ball Texture
*
********************************************************************************************/

#include "raymob.h"
#include <raymath.h>

#define MAX(a, b) ((a)>(b)? (a) : (b))
#define MIN(a, b) ((a)<(b)? (a) : (b))

// Game screens
typedef enum GameScreen {
    SCREEN_START = 0,
    SCREEN_LEVEL_SELECT,
    SCREEN_GAMEPLAY
} GameScreen;

// Transition states
typedef enum TransitionState {
    TRANSITION_OFF = 0,
    TRANSITION_FADE_OUT,
    TRANSITION_FADE_IN
} TransitionState;

static TransitionState transState = TRANSITION_OFF;
static GameScreen transTargetScreen = SCREEN_START;
static float transAlpha = 0.0f;
static float transSpeed = 3.0f;

// Trigger a smooth screen transition
static void ChangeScreenTo(GameScreen targetScreen)
{
    if (transState == TRANSITION_OFF)
    {
        transTargetScreen = targetScreen;
        transState = TRANSITION_FADE_OUT;
        transAlpha = 0.0f;
    }
}

// Helper function to draw centered text with custom font & drop shadow
static void DrawTextCentered(Font font, const char* text, float y, float fontSize, float spacing, Color color, float canvasWidth)
{
    Vector2 textSize = MeasureTextEx(font, text, fontSize, spacing);
    Vector2 textPos = { (canvasWidth - textSize.x) / 2.0f, y };

    // Drop shadow for crisp readability
    DrawTextEx(font, text, (Vector2){ textPos.x + 3.0f, textPos.y + 3.0f }, fontSize, spacing, (Color){ 0, 0, 0, 140 });
    DrawTextEx(font, text, textPos, fontSize, spacing, color);
}

// Helper function to draw an interactive button with custom font
static bool DrawButton(Font font, Rectangle rect, const char* text, Color baseColor, Color hoverColor, Color textColor, Vector2 mousePos, bool isPressed)
{
    bool hovered = CheckCollisionPointRec(mousePos, rect);
    Color btnColor = hovered ? hoverColor : baseColor;

    // Soft drop shadow
    Rectangle shadowRect = { rect.x + 8.0f, rect.y + 10.0f, rect.width, rect.height };
    DrawRectangleRounded(shadowRect, 0.25f, 12, (Color){ 0, 0, 0, 90 });

    // Main button body
    DrawRectangleRounded(rect, 0.25f, 12, btnColor);
    DrawRectangleRoundedLinesEx(rect, 0.25f, 12, 5.0f, ColorAlpha(WHITE, 0.8f));

    // Measure text using custom font
    float fontSize = rect.height * 0.38f;
    if (fontSize < 24.0f) fontSize = 24.0f;
    Vector2 textSize = MeasureTextEx(font, text, fontSize, 3.0f);
    Vector2 textPos = {
        rect.x + (rect.width - textSize.x) / 2.0f,
        rect.y + (rect.height - textSize.y) / 2.0f
    };

    // Text shadow for crisp legibility
    DrawTextEx(font, text, (Vector2){ textPos.x + 3.0f, textPos.y + 3.0f }, fontSize, 3.0f, (Color){ 0, 0, 0, 140 });
    DrawTextEx(font, text, textPos, fontSize, 3.0f, textColor);

    return (hovered && isPressed);
}

int main(void)
{
    // Enable 4X Multisampling Anti-Aliasing (MSAA) & VSync for smooth rendering
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);

    // Full HD Portrait Window Resolution (1080x1920)
    const int screenWidth = 1080;
    const int screenHeight = 1920;

    InitWindow(screenWidth, screenHeight, "raymob - Full HD Game");
    SetWindowMinSize(360, 640);
    SetTargetFPS(60);

    // Full HD Virtual Portrait Canvas (1080x1920)
    const float gameScreenWidth = 1080.0f;
    const float gameScreenHeight = 1920.0f;

    RenderTexture2D target = LoadRenderTexture((int)gameScreenWidth, (int)gameScreenHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_TRILINEAR);

    // Load High-Res Custom Font from assets folder
    Font customFont = LoadFontEx("RebellionSquad-ZpprZ.ttf", 96, NULL, 0);
    SetTextureFilter(customFont.texture, TEXTURE_FILTER_BILINEAR);

    // Load Ball Texture from assets folder
    Texture2D ballTexture = LoadTexture("ball.png");
    SetTextureFilter(ballTexture, TEXTURE_FILTER_BILINEAR);

    GameScreen currentScreen = SCREEN_START;
    int selectedLevel = 1;

    // Gameplay variables
    Vector2 playerPos = { gameScreenWidth / 2.0f, gameScreenHeight / 2.0f };
    Vector2 coinPos = { 540.0f, 760.0f };
    int score = 0;
    float ballRotation = 0.0f;

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();

        // ------------------------------------------------------------------
        // Input & Resolution Scaling
        // ------------------------------------------------------------------
        float scale = MIN((float)GetScreenWidth() / gameScreenWidth, (float)GetScreenHeight() / gameScreenHeight);

        Vector2 mouse = GetMousePosition();
        Vector2 virtualMouse = { 0 };
        virtualMouse.x = (mouse.x - ((float)GetScreenWidth() - (gameScreenWidth * scale)) * 0.5f) / scale;
        virtualMouse.y = (mouse.y - ((float)GetScreenHeight() - (gameScreenHeight * scale)) * 0.5f) / scale;
        virtualMouse = Vector2Clamp(virtualMouse, (Vector2){ 0.0f, 0.0f }, (Vector2){ gameScreenWidth, gameScreenHeight });

        // Enable button clicks only when not actively transitioning
        bool isPressed = (transState == TRANSITION_OFF) && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || GetGestureDetected() == GESTURE_TAP);

        // ------------------------------------------------------------------
        // Transition Manager Update
        // ------------------------------------------------------------------
        if (transState == TRANSITION_FADE_OUT)
        {
            transAlpha += transSpeed * deltaTime;
            if (transAlpha >= 1.0f)
            {
                transAlpha = 1.0f;
                currentScreen = transTargetScreen;
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

        // ------------------------------------------------------------------
        // Smooth Gameplay Physics & Interpolation
        // ------------------------------------------------------------------
        if (currentScreen == SCREEN_GAMEPLAY && transState == TRANSITION_OFF)
        {
            // Smooth exponential lerp toward target cursor/touch position
            float lerpSpeed = 8.0f + (float)selectedLevel * 4.0f;
            Vector2 oldPos = playerPos;
            playerPos = Vector2Lerp(playerPos, virtualMouse, MIN(1.0f, lerpSpeed * deltaTime));

            // Spin ball proportional to movement velocity
            float moveDist = Vector2Distance(oldPos, playerPos);
            ballRotation += moveDist * 3.0f;

            // Coin collection logic
            if (CheckCollisionCircles(playerPos, 56.0f, coinPos, 44.0f))
            {
                score += 10 * selectedLevel;
                coinPos.x = (float)GetRandomValue(100, (int)gameScreenWidth - 100);
                coinPos.y = (float)GetRandomValue(260, (int)gameScreenHeight - 160);
            }
        }

        // ------------------------------------------------------------------
        // Draw to Virtual Portrait Target (Full HD 1080x1920)
        // ------------------------------------------------------------------
        BeginTextureMode(target);

        switch (currentScreen)
        {
            // --------------------------------------------------------------
            // SCREEN 1: START SCREEN (Full HD)
            // --------------------------------------------------------------
            case SCREEN_START:
            {
                // Background Gradient
                DrawRectangleGradientV(0, 0, (int)gameScreenWidth, (int)gameScreenHeight, (Color){ 18, 24, 44, 255 }, (Color){ 32, 44, 76, 255 });
                DrawCircleGradient((Vector2){ gameScreenWidth / 2.0f, 960.0f }, 560.0f, (Color){ 45, 60, 110, 100 }, (Color){ 0, 0, 0, 0 });

                // Title with Custom RebellionSquad Font
                DrawTextCentered(customFont, "RAYMOB", 360.0f, 100.0f, 5.0f, GOLD, gameScreenWidth);
                DrawTextCentered(customFont, "REBELLION", 480.0f, 80.0f, 4.0f, ORANGE, gameScreenWidth);
                DrawTextCentered(customFont, "Touch screen to Play", 600.0f, 40.0f, 3.0f, LIGHTGRAY, gameScreenWidth);

                // Hero Emblem with Ball Texture & Glowing Rings
                float pulse = sinf((float)GetTime() * 3.5f) * 28.0f;
                DrawCircle((int)gameScreenWidth / 2, 940, 100.0f + pulse, (Color){ 140, 20, 50, 80 });
                DrawCircleLines((int)gameScreenWidth / 2, 940, 96.0f + pulse * 0.5f, GOLD);

                // Render Ball Texture in Menu
                float menuBallRadius = 72.0f + pulse * 0.2f;
                Rectangle srcRec = { 0.0f, 0.0f, (float)ballTexture.width, (float)ballTexture.height };
                Rectangle dstRec = { gameScreenWidth / 2.0f, 940.0f, menuBallRadius * 2.0f, menuBallRadius * 2.0f };
                Vector2 origin = { menuBallRadius, menuBallRadius };
                DrawTexturePro(ballTexture, srcRec, dstRec, origin, (float)GetTime() * 45.0f, WHITE);

                // Start Button
                if (DrawButton(customFont, (Rectangle){ gameScreenWidth / 2.0f - 260.0f, 1360.0f, 520.0f, 150.0f }, "START GAME", DARKBLUE, BLUE, WHITE, virtualMouse, isPressed))
                {
                    ChangeScreenTo(SCREEN_LEVEL_SELECT);
                }
                break;
            }

            // --------------------------------------------------------------
            // SCREEN 2: LEVEL SELECTOR (Full HD)
            // --------------------------------------------------------------
            case SCREEN_LEVEL_SELECT:
            {
                // Background Gradient
                DrawRectangleGradientV(0, 0, (int)gameScreenWidth, (int)gameScreenHeight, (Color){ 24, 32, 38, 255 }, (Color){ 38, 50, 58, 255 });

                // Header
                DrawTextCentered(customFont, "SELECT LEVEL", 220.0f, 72.0f, 4.0f, RAYWHITE, gameScreenWidth);
                DrawTextCentered(customFont, "Choose difficulty speed", 310.0f, 40.0f, 3.0f, GRAY, gameScreenWidth);

                // Level 1 Button
                if (DrawButton(customFont, (Rectangle){ gameScreenWidth / 2.0f - 300.0f, 520.0f, 600.0f, 210.0f }, "LEVEL 1\n(Normal)", DARKGREEN, LIME, WHITE, virtualMouse, isPressed))
                {
                    selectedLevel = 1;
                    score = 0;
                    playerPos = (Vector2){ gameScreenWidth / 2.0f, gameScreenHeight / 2.0f };
                    ChangeScreenTo(SCREEN_GAMEPLAY);
                }

                // Level 2 Button
                if (DrawButton(customFont, (Rectangle){ gameScreenWidth / 2.0f - 300.0f, 800.0f, 600.0f, 210.0f }, "LEVEL 2\n(Fast)", (Color){ 180, 100, 0, 255 }, ORANGE, WHITE, virtualMouse, isPressed))
                {
                    selectedLevel = 2;
                    score = 0;
                    playerPos = (Vector2){ gameScreenWidth / 2.0f, gameScreenHeight / 2.0f };
                    ChangeScreenTo(SCREEN_GAMEPLAY);
                }

                // Level 3 Button
                if (DrawButton(customFont, (Rectangle){ gameScreenWidth / 2.0f - 300.0f, 1080.0f, 600.0f, 210.0f }, "LEVEL 3\n(Extreme)", MAROON, RED, WHITE, virtualMouse, isPressed))
                {
                    selectedLevel = 3;
                    score = 0;
                    playerPos = (Vector2){ gameScreenWidth / 2.0f, gameScreenHeight / 2.0f };
                    ChangeScreenTo(SCREEN_GAMEPLAY);
                }

                // Back Button
                if (DrawButton(customFont, (Rectangle){ 40.0f, 50.0f, 240.0f, 100.0f }, "< BACK", DARKGRAY, GRAY, WHITE, virtualMouse, isPressed))
                {
                    ChangeScreenTo(SCREEN_START);
                }
                break;
            }

            // --------------------------------------------------------------
            // SCREEN 3: GAMEPLAY SCREEN (Full HD)
            // --------------------------------------------------------------
            case SCREEN_GAMEPLAY:
            {
                // Dark Blue Space Background
                DrawRectangleGradientV(0, 0, (int)gameScreenWidth, (int)gameScreenHeight, (Color){ 10, 14, 24, 255 }, (Color){ 20, 26, 42, 255 });

                // Top UI Header Bar
                DrawRectangle(0, 0, (int)gameScreenWidth, 160, (Color){ 28, 34, 52, 255 });
                DrawLineEx((Vector2){ 0, 160 }, (Vector2){ gameScreenWidth, 160 }, 4.0f, (Color){ 60, 70, 90, 255 });

                // Menu / Back Button
                if (DrawButton(customFont, (Rectangle){ 40.0f, 30.0f, 220.0f, 100.0f }, "MENU", DARKGRAY, RED, WHITE, virtualMouse, isPressed))
                {
                    ChangeScreenTo(SCREEN_LEVEL_SELECT);
                }

                // Level and Score Info
                DrawTextEx(customFont, TextFormat("LVL %i", selectedLevel), (Vector2){ 310.0f, 55.0f }, 48.0f, 4.0f, YELLOW);
                DrawTextEx(customFont, TextFormat("SCORE: %i", score), (Vector2){ gameScreenWidth - 360.0f, 55.0f }, 48.0f, 4.0f, GREEN);

                // Guidance text
                DrawTextCentered(customFont, "Touch & Drag to control player", 210.0f, 36.0f, 3.0f, (Color){ 100, 115, 140, 255 }, gameScreenWidth);

                // Collectible Coin with Outer Pulse Ring
                float coinPulse = sinf((float)GetTime() * 6.0f) * 8.0f;
                DrawCircleV(coinPos, 52.0f + coinPulse, (Color){ 255, 215, 0, 70 });
                DrawCircleV(coinPos, 40.0f, GOLD);
                DrawCircleLinesV(coinPos, 46.0f, YELLOW);
                DrawTextEx(customFont, "$", (Vector2){ coinPos.x - 12.0f, coinPos.y - 24.0f }, 48.0f, 4.0f, BLACK);

                // Player Avatar rendered with ball.png Texture & Glow
                float playerRadius = 56.0f;
                DrawCircleV(playerPos, playerRadius + 16.0f, (Color){ 100, 200, 255, 60 });
                DrawCircleLinesV(playerPos, playerRadius + 4.0f, ColorAlpha(WHITE, 0.8f));

                Rectangle srcRec = { 0.0f, 0.0f, (float)ballTexture.width, (float)ballTexture.height };
                Rectangle dstRec = { playerPos.x, playerPos.y, playerRadius * 2.0f, playerRadius * 2.0f };
                Vector2 origin = { playerRadius, playerRadius };
                DrawTexturePro(ballTexture, srcRec, dstRec, origin, ballRotation, WHITE);

                break;
            }
        }

        // Overlay transition fade overlay if transition is active
        if (transState != TRANSITION_OFF)
        {
            DrawRectangle(0, 0, (int)gameScreenWidth, (int)gameScreenHeight, ColorAlpha(BLACK, transAlpha));
        }

        EndTextureMode();

        // ------------------------------------------------------------------
        // Draw Scaled Portrait Target to Screen
        // ------------------------------------------------------------------
        BeginDrawing();
        ClearBackground(BLACK);

        DrawTexturePro(target.texture,
                       (Rectangle){ 0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height },
                       (Rectangle){ ((float)GetScreenWidth() - (gameScreenWidth * scale)) * 0.5f,
                                    ((float)GetScreenHeight() - (gameScreenHeight * scale)) * 0.5f,
                                    gameScreenWidth * scale,
                                    gameScreenHeight * scale },
                       (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);

        EndDrawing();
    }

    // Cleanup
    UnloadTexture(ballTexture);
    UnloadFont(customFont);
    UnloadRenderTexture(target);
    CloseWindow();

    return 0;
}
