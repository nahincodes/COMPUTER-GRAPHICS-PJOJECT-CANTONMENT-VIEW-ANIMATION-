#include <cmath>
#include <GL/glut.h>
#include <math.h>

#include <cmath>   // you already have this, keep it

#if defined(_WIN32) || defined(WIN32) || defined(__WIN32__) || defined(__MINGW32__) || defined(__MINGW64__)
  #include <windows.h>
  #include <mmsystem.h>
  // MSVC only uses this pragma; MinGW uses -lwinmm (already added above)
  #ifdef _MSC_VER
    #pragma comment(lib, "winmm.lib")
  #endif
#endif



#if defined(_WIN32) || defined(WIN32) || defined(__WIN32__) || defined(__MINGW32__) || defined(__MINGW64__)
#include <string>
#include <cstdio>

static std::wstring getExeDirW() {
    wchar_t buf[MAX_PATH];
    DWORD n = GetModuleFileNameW(NULL, buf, MAX_PATH);
    while (n > 0 && buf[n-1] != L'\\' && buf[n-1] != L'/') --n;
    return std::wstring(buf, buf + n); // trailing slash kept
}

// Looping tank SFX: starts when moving==true, stops when moving==false
static void ensureTankSfxPlaying(bool moving) {
    static bool isPlaying = false;
    static bool pathBuilt = false;
    static std::wstring wavPath;  // e.g., ...\bin\Debug\tank_move.wav

    if (!pathBuilt) {
        wavPath = getExeDirW() + L"tank_move.wav";
        std::fwprintf(stderr, L"[SFX] Looking for: %ls\n", wavPath.c_str());
        pathBuilt = true;
    }

    if (moving) {
        if (!isPlaying) {
            BOOL ok = PlaySoundW(wavPath.c_str(), NULL,
                                 SND_ASYNC | SND_LOOP | SND_FILENAME | SND_NODEFAULT);
            if (!ok) {
                std::fwprintf(stderr, L"[SFX] PlaySoundW failed. File missing or not a WAV/PCM?\n");
            } else {
                std::fprintf(stderr, "[SFX] Tank loop started.\n");
                isPlaying = true;
            }
        }
    } else {
        if (isPlaying) {
            PlaySoundW(NULL, 0, 0);  // stop
            std::fprintf(stderr, "[SFX] Tank loop stopped.\n");
            isPlaying = false;
        }
    }
}
#else
// Non-Windows: do nothing
static void ensureTankSfxPlaying(bool) {}
#endif



// ---------- Constants ----------
static const int WIDTH = 1920;
static const int HEIGHT = 1080;

static const int GROUND_TOP = 561;   // ground height
static const int ROAD_TOP   = 270;   // main road top edge
static const int ROAD_BOTTOM= 54;    // main road bottom edge

// ---------- Helpers ----------
void drawCircle(float cx, float cy, float r, int segments) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; ++i) {
        float theta = 2.0f * 3.1415926f * float(i) / float(segments);
        float x = r * cosf(theta);
        float y = r * sinf(theta);
        glVertex2f(cx + x, cy + y);
    }
    glEnd();
}
// ===== Helpers (add near your other utility functions) =====================

/*static inline*/ void disk(float cx, float cy, float r, int seg=32) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= seg; ++i) {
        float t = 2.0f*3.1415926f*i/seg;
        glVertex2f(cx + r*cosf(t), cy + r*sinf(t));
    }
    glEnd();
}

/*static inline*/ void ring(float cx, float cy, float rInner, float rOuter, int seg=48) {
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= seg; ++i) {
        float t = 2.0f*3.1415926f*i/seg;
        float ct = cosf(t), st = sinf(t);
        glVertex2f(cx + rOuter*ct, cy + rOuter*st);
        glVertex2f(cx + rInner*ct, cy + rInner*st);
    }
    glEnd();
}

/*static inline*/ void wheelDetailed(float cx, float cy,
                                 float rTire=42.f, float rRim=33.f, float rHub=14.f,
                                 int bolts=8) {
    // Tire
    glColor3f(0.09f, 0.10f, 0.10f);
    ring(cx, cy, rRim+2.f, rTire, 48);

    // Rim
    glColor3f(0.38f, 0.49f, 0.37f);
    ring(cx, cy, rHub+3.f, rRim, 48);

    // Hub
    glColor3f(0.25f, 0.32f, 0.22f);
    disk(cx, cy, rHub, 32);

    // Bolt caps
    glColor3f(0.10f, 0.14f, 0.10f);
    for (int i = 0; i < bolts; ++i) {
        float t = 2.0f*3.1415926f*i/bolts;
        disk(cx + (rHub+6.f)*cosf(t), cy + (rHub+6.f)*sinf(t), 2.6f, 10);
    }
}

// Rounded rectangle band for tracks (rect + semicircular ends)
/*static inline */void roundedBand(float x0, float y0, float x1, float y1, float radius) {
    // Middle rectangle
    glBegin(GL_QUADS);
        glVertex2f(x0, y0); glVertex2f(x1, y0);
        glVertex2f(x1, y1); glVertex2f(x0, y1);
    glEnd();
    // Left cap
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(x0, (y0+y1)*0.5f);
        for (int i=0;i<=32;++i){
            float t = 3.1415926f/2 + 3.1415926f*i/32;
            glVertex2f(x0 + radius*cosf(t), (y0+y1)*0.5f + radius*sinf(t));
        }
    glEnd();
    // Right cap
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(x1, (y0+y1)*0.5f);
        for (int i=0;i<=32;++i){
            float t = -3.1415926f/2 + 3.1415926f*i/32;
            glVertex2f(x1 + radius*cosf(t), (y0+y1)*0.5f + radius*sinf(t));
        }
    glEnd();
}








// ===== Night mode & UI state ===============================================
// Global toggle (button or 'N' key)
static bool gNightMode = false;

// Simple button geometry (top-left of window in your bottom-left coord system)
static const int BTN_W = 180;
static const int BTN_H = 40;
static const int BTN_X = 20;
static const int BTN_Y = HEIGHT - 60; // 20px margin from top edge

/*static inline*/ bool inRect(int x, int y, int rx, int ry, int rw, int rh) {

    return x >= rx && x <= rx+rw && y >= ry && y <= ry+rh;
}





//helper for playground
/*static inline*/ void circleOutline2f(float cx, float cy, float r, int seg=64) {
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < seg; ++i) {
        float t = 2.0f * 3.1415926f * (float)i / (float)seg;
        glVertex2f(cx + r * cosf(t), cy + r * sinf(t)); // <- only glVertex2f
    }
    glEnd();
}

static inline void arcOutline2f(float cx, float cy, float r,
                                float a0, float a1, int seg=32) {
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= seg; ++i) {
        float t = a0 + (a1 - a0) * (float)i / (float)seg;
        glVertex2f(cx + r * cosf(t), cy + r * sinf(t)); // <- only glVertex2f
    }
    glEnd();
}



// ===== The tank (place on your road) =======================================
// x,y = bottom-left of track band; scale = overall size; angleDeg = rotation
void drawTankDetailed(float x, float y, float scale=1.0f, float angleDeg=0.0f) {
    glPushMatrix();

    // --- Self-contained animation state (no globals) -----------------------
    static int  t0ms    = -1;
    if (t0ms < 0) t0ms = glutGet(GLUT_ELAPSED_TIME);
    float t = (glutGet(GLUT_ELAPSED_TIME) - t0ms) * 0.001f; // seconds

    const float speedPxPerSec = 140.0f;
    const float startLeft     = -900.0f;
    const float rightReset    = WIDTH + 120.0f;
    const float span          = rightReset - startLeft;
    float worldX = startLeft + fmodf(t * speedPxPerSec, span);

    static bool  initBase = false;
    static float baseX = 0.0f;
    if (!initBase) { baseX = x; initBase = true; }

    const float treadStep   = 28.f;
    const float treadSpeed  = 80.f;
    float padOffset = fmodf(t * treadSpeed, treadStep);

#if defined(_WIN32) || defined(WIN32) || defined(__WIN32__) || defined(__MINGW32__) || defined(__MINGW64__)
    bool moving = std::fabs(speedPxPerSec) > 1e-3f;
    ensureTankSfxPlaying(moving);
#endif

    // ------------------  transforms ---------------------------
    glTranslatef(x, y, 0);
    glTranslatef(worldX - baseX, 0, 0);
    glScalef(scale, scale, 1);
    glRotatef(angleDeg, 0, 0, 1);

    // --- Proportions (all in local tank space) ---
    const float trackH      = 120.f;
    const float trackPadH   = 12.f;
    const float trackRad    = 58.f;
    const float wheelY      = 60.f + 30.f; // 90
    const int   wheelCount  = 6;
    const float wheelSpace  = 108.f;
    const float wheelStartX = 120.f;
    const float bandLeft    = 60.f;
    const float bandRight   = 780.f;       // 120 + (6-1)*108 + 120
    const float bandBottom  = 20.f;
    const float bandTop     = 140.f;       // 20 + 120

    // ===================== BACK LAYER: TRACKS ===============================
    glColor3f(0.12f, 0.13f, 0.13f);  // dark track band
    roundedBand(bandLeft, bandBottom, bandRight, bandTop, trackRad);

    // Track pads (animated bottom + subtle top echo)
   glColor3f(0.18f, 0.19f, 0.19f);
   //glColor3f(1.0f,0.0f,0.0f);
    for (float px = bandLeft + 8.f - padOffset; px < bandRight-8.f; px += treadStep) {
        // bottom run
        glBegin(GL_QUADS);
            glVertex2f(px,          bandBottom + 4);               // px, 24
            glVertex2f(px + 18.f,   bandBottom + 4);               // px+18, 24
            glVertex2f(px + 18.f,   bandBottom + 4 + trackPadH);   // px+18, 36
            glVertex2f(px,          bandBottom + 4 + trackPadH);   // px,    36
        glEnd();
        // top run (thin echo helps sell motion)
        glBegin(GL_QUADS);
            glVertex2f(px,          bandTop - 6 - trackPadH*0.6f); // px,    127.8
            glVertex2f(px + 18.f,   bandTop - 6 - trackPadH*0.6f); // px+18, 127.8
            glVertex2f(px + 18.f,   bandTop - 6);                  // px+18, 134
            glVertex2f(px,          bandTop - 6);                  // px,    134
        glEnd();
    }

    // ===================== WHEELS (on top of tracks) ========================
    for (int i=0; i<wheelCount; ++i) {
        float cx = wheelStartX + i*wheelSpace;
        float cy = bandBottom + wheelY;
        wheelDetailed(cx, cy, 56.f, 43.f, 18.f, 8);
    }

// Drive sprocket(shamne)
glColor3f(0.12f, 0.16f, 0.12f); ring(720.0f, 114.0f, 38.0f, 56.0f, 32);
glColor3f(0.28f, 0.38f, 0.26f); ring(720.0f, 114.0f, 20.0f, 35.0f, 32);

// Idler(rcircle)
glColor3f(0.12f, 0.16f, 0.12f);
ring(120.0f, 112.0f, 36.0f, 54.0f, 32);
glColor3f(0.28f, 0.38f, 0.26f);
ring(120.0f, 112.0f, 18.0f, 33.0f, 32);

    // ===================== SIDE SKIRTS (armor) ==============================
    glColor3f(0.35f, 0.55f, 0.35f);
   //glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_POLYGON); // long skirt with front/back chamfers
        glVertex2f(50.0f, 132.0f);
        glVertex2f(790.0f, 132.0f);
        glVertex2f(840.0f, 102.0f);
        glVertex2f(740.0f, 82.0f);
        glVertex2f(70.0f, 84.0f);
        glVertex2f(20.0f, 110.0f);
    glEnd();

    // Panel cuts on skirt
glLineWidth(2);
glColor3f(0.22f, 0.33f, 0.20f);
//glColor3f(1.0f, 0.0f, 0.0f);
glBegin(GL_LINES);
glVertex2f(300.0f, 132.0f);
glVertex2f(300.0f, 84.0f);
 glEnd();
glBegin(GL_LINES);
 glVertex2f(540.0f, 132.0f);
 glVertex2f(540.0f, 84.0f);
  glEnd();


    // ===================== HULL =================================================
    // Lower hull / fenders
    glColor3f(0.32f, 0.52f, 0.32f);
     //glColor3f(1.0f,0.0f,0.0f);
    glBegin(GL_QUADS);
        glVertex2f(20.0f, 90.0f);
        glVertex2f(830.0f, 90.0f);
        glVertex2f(870.0f, 150.0f);
        glVertex2f(60.0f, 150.0f);
    glEnd();

    // Upper glacis (front slope)(trivuj)
    glColor3f(0.39f, 0.60f, 0.38f);
    //glColor3f(1.0f,0.0f,0.0f);
    glBegin(GL_TRIANGLES);
        glVertex2f(80.0f, 150.0f);
        glVertex2f(300.0f, 235.0f);
        glVertex2f(460.0f, 150.0f);
    glEnd();

    // Hull deck (top slab)
   glColor3f(0.37f, 0.58f, 0.37f);
   // glColor3f(1.0f,0.0f,0.0f);
    glBegin(GL_QUADS);
        glVertex2f(240.0f, 210.0f);
        glVertex2f(720.0f, 210.0f);
        glVertex2f(865.0f, 150.0f);
        glVertex2f(180.0f, 150.0f);
    glEnd();

    // ===================== TURRET =================================================

    // Turret body (angular polygon)
    // glColor3f(1.0f,0.0f,0.0f);
    glColor3f(0.33f, 0.54f, 0.33f);
    glBegin(GL_POLYGON);
        glVertex2f(400.0f, 240.0f);
        glVertex2f(670.0f, 240.0f);
        glVertex2f(740.0f, 205.0f);
        glVertex2f(650.0f, 170.0f);
        glVertex2f(420.0f, 170.0f);
        glVertex2f(320.0f, 195.0f);
    glEnd();
//glass
        glLineWidth(7);
      glColor3ub(157, 204, 237);
        glBegin(GL_LINES);
        glVertex2f(740.0f, 205.0f);
         glVertex2f(670.0f, 240.0f);
        glEnd();
    // Turret ring
    glColor3f(0.25f, 0.40f, 0.25f);
    ring(405.0f, 180.0f, 20.0f, 34.0f, 36);

    // Optics & small boxes
    glColor3f(0.25f, 0.40f, 0.25f);
    glBegin(GL_QUADS); // left sight box
        glVertex2f(380.0f, 208.0f);
        glVertex2f(420.0f, 208.0f);
        glVertex2f(420.0f, 228.0f);
        glVertex2f(380.0f, 228.0f);
    glEnd();


    glColor3f(0.15f, 0.20f, 0.15f); // glass slit
    glBegin(GL_QUADS);
        glVertex2f(386.0f, 218.0f);
        glVertex2f(414.0f, 218.0f);
        glVertex2f(414.0f, 223.0f);
        glVertex2f(386.0f, 223.0f);
    glEnd();

  // Set color for smoke launchers
glColor3f(0.30f, 0.45f, 0.30f);

// --- First Bank of Smoke Launchers (Upper Row) ---

// Launcher 1
glBegin(GL_QUADS);
    glVertex2f(580.0f, 200.0f);
    glVertex2f(596.0f, 200.0f);
    glVertex2f(594.0f, 182.0f);
    glVertex2f(578.0f, 182.0f);
glEnd();
disk(587.0f, 194.0f, 5.0f, 18);

// Launcher 2
glBegin(GL_QUADS);
    glVertex2f(608.0f, 200.0f);
    glVertex2f(624.0f, 200.0f);
    glVertex2f(622.0f, 182.0f);
    glVertex2f(606.0f, 182.0f);
glEnd();
disk(615.0f, 194.0f, 5.0f, 18);

// Launcher 3
glBegin(GL_QUADS);
    glVertex2f(636.0f, 200.0f);
    glVertex2f(652.0f, 200.0f);
    glVertex2f(650.0f, 182.0f);
    glVertex2f(634.0f, 182.0f);
glEnd();
disk(643.0f, 194.0f, 5.0f, 18);

// --- Second Bank of Smoke Launchers (Lower Row) ---

// Launcher 1
glBegin(GL_QUADS);
    glVertex2f(580.0f, 172.0f);
    glVertex2f(596.0f, 172.0f);
    glVertex2f(594.0f, 154.0f);
    glVertex2f(578.0f, 154.0f);
glEnd();
disk(587.0f, 166.0f, 5.0f, 18);

// Launcher 2
glBegin(GL_QUADS);
    glVertex2f(608.0f, 172.0f);
    glVertex2f(624.0f, 172.0f);
    glVertex2f(622.0f, 154.0f);
    glVertex2f(606.0f, 154.0f);
glEnd();
disk(615.0f, 166.0f, 5.0f, 18);

// Launcher 3
glBegin(GL_QUADS);
    glVertex2f(636.0f, 172.0f);
    glVertex2f(652.0f, 172.0f);
    glVertex2f(650.0f, 154.0f);
    glVertex2f(634.0f, 154.0f);
glEnd();
disk(643.0f, 166.0f, 5.0f, 18);

    // Cupola
    glColor3f(0.36f, 0.56f, 0.36f);
    disk(470.0f, 236.0f, 20.0f, 24);
    glColor3f(0.22f, 0.33f, 0.22f);
    ring(470.0f, 236.0f, 12.0f, 20.0f, 24);

    // MG mount & barrel
    glColor3f(0.18f, 0.22f, 0.18f);
     //glColor3f(1.0f,0.0f,0.0f);
    glBegin(GL_QUADS); // mount
        glVertex2f(462.0f, 242.0f);
        glVertex2f(498.0f, 242.0f);
        glVertex2f(498.0f, 250.0f);
        glVertex2f(462.0f, 250.0f);
    glEnd();
    glBegin(GL_QUADS); // barrel
        glVertex2f(498.0f, 246.0f);
        glVertex2f(580.0f, 246.0f);
        glVertex2f(580.0f, 248.0f);
        glVertex2f(498.0f, 248.0f);
    glEnd();

    // Antennas
    glLineWidth(2);
    glBegin(GL_LINES);
        glVertex2f(520.0f, 220.0f);
        glVertex2f(520.0f, 330.0f);
    glEnd();
    glBegin(GL_LINES);
        glVertex2f(630.0f, 210.0f);
        glVertex2f(640.0f, 330.0f);
    glEnd();

// ===================== GUN ====================================================
// rear darker sleeve
    glColor3f(0.22f, 0.35f, 0.22f); // Dark green sleeve
glBegin(GL_QUADS);
    glVertex2f(660.0f, 185.0f);
    glVertex2f(670.0f, 185.0f);
    glVertex2f(670.0f, 205.0f);
    glVertex2f(660.0f, 205.0f);
glEnd();
//long barrel
   glColor3f(0.32f, 0.50f, 0.32f); // Barrel green
glBegin(GL_QUADS);
    glVertex2f(670.0f, 185.0f);   // near
    glVertex2f(1010.0f, 187.0f);  // far bottom (taper)
    glVertex2f(1010.0f, 203.0f);  // far top
    glVertex2f(670.0f, 205.0f);   // near top
glEnd();

    // fume extractor
    glColor3f(0.28f, 0.44f, 0.28f); // Mid green
glBegin(GL_QUADS);
    glVertex2f(810.0f, 182.0f);
    glVertex2f(870.0f, 182.0f);
    glVertex2f(870.0f, 208.0f);
    glVertex2f(810.0f, 208.0f);
glEnd();

glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // muzzle tip
   // Base muzzle color (dark metal)
glColor3f(0.10f, 0.12f, 0.12f);
glBegin(GL_QUADS);
    glVertex2f(1010.0f, 185.0f);
    glVertex2f(1030.0f, 185.0f);
    glVertex2f(1030.0f, 205.0f);
    glVertex2f(1010.0f, 205.0f);
glEnd();
//sperks
glColor4f(1.0f, 0.8f, 0.2f, 0.6f);
glPointSize(3.0f);
glBegin(GL_POINTS);
    glVertex2f(1042.0f, 192.0f);
    glVertex2f(1046.0f, 198.0f);
    glVertex2f(1038.0f, 202.0f);
glEnd();
// Firing flash overlay (bright orange-yellow)
glColor3f(1.0f, 0.6f, 0.0f); // Fiery orange
glBegin(GL_TRIANGLE_FAN);
    glVertex2f(1030.0f, 195.0f); // center
    glVertex2f(1045.0f, 190.0f);
    glVertex2f(1048.0f, 195.0f);
    glVertex2f(1045.0f, 200.0f);
    glVertex2f(1040.0f, 202.0f);
    glVertex2f(1035.0f, 200.0f);
    glVertex2f(1032.0f, 195.0f);
    glVertex2f(1035.0f, 190.0f);

glEnd();

    // ===================== LINE ACCENTS ==========================================
    glLineWidth(2);
    glColor3f(0.20f, 0.30f, 0.20f);

    // a few seams on turret
    glBegin(GL_LINES);
        glVertex2f(420.0f, 200.0f); glVertex2f(670.0f, 200.0f);
        glVertex2f(440.0f, 220.0f); glVertex2f(580.0f, 220.0f);
    glEnd();

    glPopMatrix();

    // Keep the animation running even if you didn't set an idle/timer.
    glutPostRedisplay();
}


















void drawSkyGradient() {
    // Colors you can tweak:
    // Day: darker blue at zenith -> pale near horizon
    const GLfloat dayTop[3]    = {0.25f, 0.55f, 0.95f};
    const GLfloat dayBottom[3] = {0.72f, 0.88f, 1.00f};

    // Night: almost-navy at zenith -> slightly lighter near horizon
    const GLfloat nightTop[3]    = {0.02f, 0.05f, 0.09f};
    const GLfloat nightBottom[3] = {0.07f, 0.11f, 0.20f};

    const GLfloat* top =  gNightMode ? nightTop    : dayTop;
    const GLfloat* bot =  gNightMode ? nightBottom : dayBottom;

    glBegin(GL_QUADS);
        // TOP EDGE (darker)
        glColor3fv(top); glVertex2i(0,      HEIGHT);
        glColor3fv(top); glVertex2i(WIDTH,  HEIGHT);

        // BOTTOM EDGE at ground line (lighter)
        glColor3fv(bot); glVertex2i(WIDTH,  GROUND_TOP);
        glColor3fv(bot); glVertex2i(0,      GROUND_TOP);
    glEnd();
}











/*
// ---------- Ground ----------
void drawGround() {
    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_QUADS);
        glVertex2i(0, 0);
        glVertex2i(WIDTH, 0);
        glVertex2i(WIDTH, GROUND_TOP);
        glVertex2i(0, GROUND_TOP);
    glEnd();
}
*/










void drawGround() {
    const GLfloat dayTop[3]    = {0.62f, 0.84f, 0.42f};
    const GLfloat dayMid[3]    = {0.32f, 0.65f, 0.24f};
    const GLfloat dayBot[3]    = {0.10f, 0.50f, 0.14f};

    const GLfloat nightTop[3]  = {0.08f, 0.16f, 0.10f};
    const GLfloat nightMid[3]  = {0.05f, 0.11f, 0.07f};
    const GLfloat nightBot[3]  = {0.03f, 0.08f, 0.05f};

    const GLfloat* top =  gNightMode ? nightTop : dayTop;
    const GLfloat* mid =  gNightMode ? nightMid : dayMid;
    const GLfloat* bot =  gNightMode ? nightBot : dayBot;

    // Place the mid band around ~45% of ground height
    const int yMid = static_cast<int>(GROUND_TOP * 0.45f);

    // Bottom -> mid
    glBegin(GL_QUADS);
        glColor3fv(bot); glVertex2i(0,     0);
        glColor3fv(bot); glVertex2i(WIDTH, 0);
        glColor3fv(mid); glVertex2i(WIDTH, yMid);
        glColor3fv(mid); glVertex2i(0,     yMid);
    glEnd();

    // Mid -> top
    glBegin(GL_QUADS);
        glColor3fv(mid); glVertex2i(0,     yMid);
        glColor3fv(mid); glVertex2i(WIDTH, yMid);
        glColor3fv(top); glVertex2i(WIDTH, GROUND_TOP);
        glColor3fv(top); glVertex2i(0,     GROUND_TOP);
    glEnd();
}










// ---------- Playground (football field) ----------
// Fits entirely between ROAD_TOP and GROUND_TOP; uses only glVertex2f.
// Basic primitives: quads (grass), lines (markings), line-loops (circles/arcs).
void drawPlayground() {
    // --- Layout within allowed vertical band ---
    const float marginY = 10.0f;
    const float bottom  = ROAD_TOP   + marginY;      // >= 270
    const float top     = GROUND_TOP - marginY;      // <= 561

    // Keep left side clear of building (building starts at x=1420)
    const float marginX = 40.0f;
    const float left    = 120.0f + marginX;          // a bit into scene
    const float right   = 1380.0f;                   // safely left of building

    const float w = right - left;
    const float h = top   - bottom;
    const float cx = 0.5f * (left + right);
    const float cy = 0.5f * (bottom + top);

    // --- Grass base (solid quad) ---
    // (You can darken at night if you like; keeping it neutral here)
    glColor3f(0.12f, 0.55f, 0.20f);
    glBegin(GL_QUADS);
        glVertex2f(left,  bottom);
        glVertex2f(right, bottom);
        glVertex2f(right, top);
        glVertex2f(left,  top);
    glEnd();

    // --- Field boundary line ---
    glLineWidth(3.0f);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(left,  bottom);
        glVertex2f(right, bottom);
        glVertex2f(right, top);
        glVertex2f(left,  top);
    glEnd();

    // --- Halfway line ---
    glBegin(GL_LINES);
        glVertex2f(cx, bottom);
        glVertex2f(cx, top);
    glEnd();

    // --- Center circle + spot ---
    const float rCenter = h * 0.18f;
    circleOutline2f(cx, cy, rCenter, 64);

    // tiny filled center spot using triangles (still basic primitive)
    {
        const float rDot = 4.0f;
        glBegin(GL_TRIANGLES);
            // Approximate filled dot as a 12-triangle fan (no fan primitive needed)
            const int seg = 12;
            for (int i = 0; i < seg; ++i) {
                float t0 = 2.0f * 3.1415926f *  i      / seg;
                float t1 = 2.0f * 3.1415926f * (i + 1) / seg;
                glVertex2f(cx, cy);
                glVertex2f(cx + rDot * cosf(t0), cy + rDot * sinf(t0));
                glVertex2f(cx + rDot * cosf(t1), cy + rDot * sinf(t1));
            }
        glEnd();
    }

    // --- Penalty boxes (rectangles), goal areas, and spots ---
    const float boxDepth   = w * 0.16f; // how far box extends into field
    const float boxHalfH   = h * 0.30f; // half height of penalty box
    const float goalDepth  = w * 0.06f; // small goal area depth
    const float goalHalfH  = h * 0.14f;

    // Left penalty box
    glBegin(GL_LINE_LOOP);
        glVertex2f(left,            cy - boxHalfH);
        glVertex2f(left + boxDepth, cy - boxHalfH);
        glVertex2f(left + boxDepth, cy + boxHalfH);
        glVertex2f(left,            cy + boxHalfH);
    glEnd();

    // Right penalty box
    glBegin(GL_LINE_LOOP);
        glVertex2f(right - boxDepth, cy - boxHalfH);
        glVertex2f(right,            cy - boxHalfH);
        glVertex2f(right,            cy + boxHalfH);
        glVertex2f(right - boxDepth, cy + boxHalfH);
    glEnd();

    // Left goal area
    glBegin(GL_LINE_LOOP);
        glVertex2f(left,             cy - goalHalfH);
        glVertex2f(left + goalDepth, cy - goalHalfH);
        glVertex2f(left + goalDepth, cy + goalHalfH);
        glVertex2f(left,             cy + goalHalfH);
    glEnd();

    // Right goal area
    glBegin(GL_LINE_LOOP);
        glVertex2f(right - goalDepth, cy - goalHalfH);
        glVertex2f(right,              cy - goalHalfH);
        glVertex2f(right,              cy + goalHalfH);
        glVertex2f(right - goalDepth,  cy + goalHalfH);
    glEnd();

    // Penalty spots
    {
        const float rDot = 3.0f;
        const float spotOffset = boxDepth * 0.55f; // distance from goal line
        // left
        for (int i = 0; i < 12; ++i) {
            float t0 = 2.0f * 3.1415926f *  i      / 12.0f;
            float t1 = 2.0f * 3.1415926f * (i + 1) / 12.0f;
            glBegin(GL_TRIANGLES);
                glVertex2f(left + spotOffset, cy);
                glVertex2f(left + spotOffset + rDot * cosf(t0), cy + rDot * sinf(t0));
                glVertex2f(left + spotOffset + rDot * cosf(t1), cy + rDot * sinf(t1));
            glEnd();
        }
        // right
        for (int i = 0; i < 12; ++i) {
            float t0 = 2.0f * 3.1415926f *  i      / 12.0f;
            float t1 = 2.0f * 3.1415926f * (i + 1) / 12.0f;
            glBegin(GL_TRIANGLES);
                glVertex2f(right - spotOffset, cy);
                glVertex2f(right - spotOffset + rDot * cosf(t0), cy + rDot * sinf(t0));
                glVertex2f(right - spotOffset + rDot * cosf(t1), cy + rDot * sinf(t1));
            glEnd();
        }
    }

    // --- Penalty arcs (outside the box edge) ---
    {
        const float rArc = rCenter * 0.60f;
        // Left arc: centered at left penalty spot, only the part outside the box
        arcOutline2f(left + boxDepth * 0.55f, cy, rArc, -0.6f * 3.1415926f, 0.6f * 3.1415926f, 36);
        // Right arc (mirror)
        arcOutline2f(right - boxDepth * 0.55f, cy, rArc, 0.4f * 3.1415926f, 1.6f * 3.1415926f, 36);
    }

    // --- Corner arcs (inside field corners) ---
    {
        const float rc = 16.0f;
        // bottom-left corner (90° arc)
        arcOutline2f(left,  bottom, rc, 0.0f, 0.5f * 3.1415926f, 12);
        // top-left
        arcOutline2f(left,  top,    rc, -0.5f * 3.1415926f, 0.0f, 12);
        // top-right
        arcOutline2f(right, top,    rc, 3.1415926f, 1.5f * 3.1415926f, 12);
        // bottom-right
        arcOutline2f(right, bottom, rc, 0.5f * 3.1415926f, 3.1415926f, 12);
    }

    // --- (Optional) simple goals drawn as thin quads on the outer edge ---
    glColor3f(1.0f, 1.0f, 1.0f);
    const float goalW = 6.0f; // post thickness
    const float goalH = h * 0.18f;
    // left goal (outside the field line, hugging it)
    glBegin(GL_QUADS);
        glVertex2f(left - goalW, cy - goalH*0.5f);
        glVertex2f(left,         cy - goalH*0.5f);
        glVertex2f(left,         cy + goalH*0.5f);
        glVertex2f(left - goalW, cy + goalH*0.5f);
    glEnd();
    // right goal
    glBegin(GL_QUADS);
        glVertex2f(right,        cy - goalH*0.5f);
        glVertex2f(right + goalW,cy - goalH*0.5f);
        glVertex2f(right + goalW,cy + goalH*0.5f);
        glVertex2f(right,        cy + goalH*0.5f);
    glEnd();

    // Reset line width if you change it elsewhere later
    glLineWidth(1.0f);
}














// ---------- Trees ----------
void drawTrees() {
    for (int i = 100; i <= 1800; i += 300) {
        if (i == 1600) continue; // skip rightmost for building

        // trunk
        glColor3f(0.545f, 0.271f, 0.075f);
        glBegin(GL_QUADS);
            glVertex2i(i, GROUND_TOP);
            glVertex2i(i + 20, GROUND_TOP);
            glVertex2i(i + 20, 620);
            glVertex2i(i, 620);
        glEnd();

        // leaves
        glColor3f(0.0f, 0.5f, 0.0f);
        glBegin(GL_TRIANGLES);
            glVertex2i(i - 30, 620);
            glVertex2i(i + 50, 620);
            glVertex2i(i + 10, 680);
        glEnd();
        glBegin(GL_TRIANGLES);
            glVertex2i(i - 20, 650);
            glVertex2i(i + 40, 650);
            glVertex2i(i + 10, 710);
        glEnd();
    }
}

// ---------- Benches ----------
void drawBenches() {
    for (int i = 250; i <= 1750; i += 300) {
        if (i == 1450 || i == 1750) continue;

        glColor3f(0.545f, 0.271f, 0.075f);
        // seat
        glBegin(GL_QUADS);
            glVertex2i(i, 580);
            glVertex2i(i + 60, 580);
            glVertex2i(i + 60, 590);
            glVertex2i(i, 590);
        glEnd();
        // legs
        glBegin(GL_QUADS);
            glVertex2i(i + 5, 560);
            glVertex2i(i + 15, 560);
            glVertex2i(i + 15, 580);
            glVertex2i(i + 5, 580);
        glEnd();
        glBegin(GL_QUADS);
            glVertex2i(i + 45, 560);
            glVertex2i(i + 55, 560);
            glVertex2i(i + 55, 580);
            glVertex2i(i + 45, 580);
        glEnd();
    }
}

// ---------- Building (right side) ----------
void drawBuilding() {
    // main body
    glColor3f(0.80f, 0.82f, 0.85f);
    glBegin(GL_QUADS);
        glVertex2i(1420, GROUND_TOP);
        glVertex2i(1840, GROUND_TOP);
        glVertex2i(1840, 961);
        glVertex2i(1420, 961);
    glEnd();

    // floor separators
    glLineWidth(2);
    glColor3f(0.65f, 0.67f, 0.70f);
    glBegin(GL_LINES);
        glVertex2i(1420, 641); glVertex2i(1840, 641);
        glVertex2i(1420, 721); glVertex2i(1840, 721);
        glVertex2i(1420, 801); glVertex2i(1840, 801);
        glVertex2i(1420, 881); glVertex2i(1840, 881);
    glEnd();

    // windows 3x5
    glColor3f(0.35f, 0.55f, 0.75f);
    for (int row = 0; row < 5; ++row) {
        int wy = 586 + row * 80;
        glBegin(GL_QUADS);
         glVertex2i(1460, wy);
          glVertex2i(1500, wy);
           glVertex2i(1500, wy + 40);
           glVertex2i(1460, wy + 40);
         glEnd();
        glBegin(GL_QUADS);
         glVertex2i(1600, wy);
          glVertex2i(1640, wy);
           glVertex2i(1640, wy + 40);
            glVertex2i(1600, wy + 40);
         glEnd();
        glBegin(GL_QUADS);
         glVertex2i(1740, wy);
         glVertex2i(1780, wy);
          glVertex2i(1780, wy + 40);
           glVertex2i(1740, wy + 40);
        glEnd();
    }

    // door
    glColor3f(0.30f, 0.25f, 0.20f);
    glBegin(GL_QUADS);
        glVertex2i(1585, GROUND_TOP);
        glVertex2i(1675, GROUND_TOP);
        glVertex2i(1675, 621);
        glVertex2i(1585, 621);
    glEnd();

    // parapet
    glColor3f(0.70f, 0.72f, 0.75f);
    glBegin(GL_QUADS);
        glVertex2i(1420, 961);
        glVertex2i(1840, 961);
        glVertex2i(1840, 975);
        glVertex2i(1420, 975);
    glEnd();

    // sign board
    glColor3f(0.10f, 0.12f, 0.18f);
    glBegin(GL_QUADS);
        glVertex2i(1470, 980);
        glVertex2i(1790, 980);
        glVertex2i(1790, 1015);
        glVertex2i(1470, 1015);
    glEnd();

    // sign text
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2i(1480, 993);
    const char* signTxt = "            OFFICERS MESS";
    for (const char* p = signTxt; *p; ++p)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
}

// ---------- Connector road (building to main road) ----------
void drawConnectorRoad() {
    // road body
    glColor3f(0.22f, 0.22f, 0.22f);
    glBegin(GL_QUADS);
        glVertex2i(1570, GROUND_TOP);
        glVertex2i(1690, GROUND_TOP);
        glVertex2i(1690, ROAD_TOP);
        glVertex2i(1570, ROAD_TOP);
    glEnd();

    // side borders
    glLineWidth(4);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINES);
    glVertex2i(1570, GROUND_TOP);
     glVertex2i(1570, ROAD_TOP);
      glEnd();
    glBegin(GL_LINES);
    glVertex2i(1690, GROUND_TOP);
    glVertex2i(1690, ROAD_TOP);
     glEnd();

    // middle dashed line
    int midX = (1570 + 1690) / 2; // center X = 1630
    glLineWidth(3);
    glColor3f(1.0f, 1.0f, 1.0f);

    glBegin(GL_LINES);
        for (int y = ROAD_TOP; y < GROUND_TOP; y += 40) {
            glVertex2i(midX, y);
            glVertex2i(midX, y + 20); // dash length = 20, gap = 20
        }
    glEnd();
}

// ---------- Main road ----------
void drawMainRoad() {
    // asphalt
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
        glVertex2i(0, ROAD_TOP);
        glVertex2i(WIDTH, ROAD_TOP);
        glVertex2i(WIDTH, ROAD_BOTTOM);
        glVertex2i(0, ROAD_BOTTOM);
    glEnd();

    // dashed center line
    glLineWidth(4);
    glColor3f(1.0f, 1.0f, 1.0f);
    int yLine = (ROAD_TOP + ROAD_BOTTOM) / 2;
    glBegin(GL_LINES);
        for (int x = 0; x < WIDTH; x += 100) {
            glVertex2i(x, yLine);
            glVertex2i(x + 50, yLine);
        }
    glEnd();

    // borders
    glLineWidth(6);
    glBegin(GL_LINES); glVertex2i(0, ROAD_TOP);
    glVertex2i(WIDTH, ROAD_TOP);    glEnd();
    glBegin(GL_LINES); glVertex2i(0, ROAD_BOTTOM);
    glVertex2i(WIDTH, ROAD_BOTTOM); glEnd();
}

// ---------- Road lamps ----------
/*void drawRoadLamps() {
    // lamp positions along x
    const int xs[] = {150, 450, 750, 1050};
    for (int k = 0; k < 4; ++k) {
        int baseX = xs[k];

        // pole
        glColor3f(0.3f, 0.3f, 0.3f);
        glBegin(GL_QUADS);
            glVertex2i(baseX, ROAD_TOP);
            glVertex2i(baseX + 10, ROAD_TOP);
            glVertex2i(baseX + 10, ROAD_TOP + 200);
            glVertex2i(baseX, ROAD_TOP + 200);
        glEnd();

        // head
        glColor3f(1.0f, 1.0f, 0.0f);
        drawCircle(baseX + 5, ROAD_TOP + 210, 15, 20);

        // glow (needs blending enabled in main)
        glColor4f(1.0f, 1.0f, 0.0f, 0.3f);
        drawCircle(baseX + 5, ROAD_TOP + 210, 40, 30);
    }
}*/








void drawRoadLamps() {
    // lamp positions along x (unchanged)
    const int xs[] = {150, 450, 750, 1050};
    for (int k = 0; k < 4; ++k) {
        int baseX = xs[k];

        // pole
        glColor3f(0.3f, 0.3f, 0.3f);
        glBegin(GL_QUADS);
            glVertex2i(baseX, ROAD_TOP);
            glVertex2i(baseX + 10, ROAD_TOP);
            glVertex2i(baseX + 10, ROAD_TOP + 200);
            glVertex2i(baseX, ROAD_TOP + 200);
        glEnd();

        // lamp head (off by day, on by night)
        if (gNightMode) {
            // bright head
            glColor3f(1.0f, 0.95f, 0.4f);
            disk(baseX + 5, ROAD_TOP + 210, 16.f, 24);

            // inner glow
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(1.0f, 0.95f, 0.4f, 0.35f);
            disk(baseX + 5, ROAD_TOP + 210, 48.f, 40);

            // outer soft halo
            glColor4f(1.0f, 0.95f, 0.4f, 0.15f);
            disk(baseX + 5, ROAD_TOP + 210, 90.f, 40);
            glDisable(GL_BLEND);
        } else {
            // day: head looks off (cool gray), no halo
            glColor3f(0.75f, 0.78f, 0.80f);
            disk(baseX + 5, ROAD_TOP + 210, 14.f, 20);
        }
    }
}


















void drawUIButton() {
    // Shadow (subtle)
    glColor4f(0, 0, 0, gNightMode ? 0.45f : 0.25f);
    glBegin(GL_QUADS);
        glVertex2i(BTN_X+3, BTN_Y-3);
        glVertex2i(BTN_X+BTN_W+3, BTN_Y-3);
        glVertex2i(BTN_X+BTN_W+3, BTN_Y+BTN_H-3);
        glVertex2i(BTN_X+3, BTN_Y+BTN_H-3);
    glEnd();

    // Body
    if (gNightMode) glColor3f(0.10f, 0.25f, 0.45f);
    else            glColor3f(0.90f, 0.90f, 0.90f);
    glBegin(GL_QUADS);
        glVertex2i(BTN_X, BTN_Y);
        glVertex2i(BTN_X+BTN_W, BTN_Y);
        glVertex2i(BTN_X+BTN_W, BTN_Y+BTN_H);
        glVertex2i(BTN_X, BTN_Y+BTN_H);
    glEnd();
/*
    // Border
    glLineWidth(2);
    glColor3f(0.15f, 0.15f, 0.15f);
    glBegin(GL_LINE_LOOP);
        glVertex2i(BTN_X, BTN_Y);
        glVertex2i(BTN_X+BTN_W, BTN_Y);
        glVertex2i(BTN_X+BTN_W, BTN_Y+BTN_H);
        glVertex2i(BTN_X, BTN_Y+BTN_H);
    glEnd();

    // Label
    const char* label = gNightMode ? "Night Mode: ON (N)" : "Night Mode: OFF (N)";
    glColor3f(gNightMode ? 1.0f : 0.0f, gNightMode ? 1.0f : 0.0f, gNightMode ? 1.0f : 0.0f);
    glRasterPos2i(BTN_X + 12, BTN_Y + 14);
    for (const char* p = label; *p; ++p)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);


*/

}




// Left click toggles when clicking inside the button.
// NOTE: GLUT gives mouse y with origin at TOP-LEFT; we convert to bottom-left.
void onMouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        int yGL = HEIGHT - y; // convert to your ortho coords
        if (inRect(x, yGL, BTN_X, BTN_Y, BTN_W, BTN_H)) {
            gNightMode = !gNightMode;
            glutPostRedisplay(); // request redraw
        }
    }
}

// 'N' toggles mode, 'Esc' quits.
void onKeyboard(unsigned char key, int, int) {
    if (key == 'n' || key == 'N') {
        gNightMode = !gNightMode;
        glutPostRedisplay();
    } else if (key == 27) {
        exit(0);
    }
}













/*
// ---------- Display ----------
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // draw in back-to-front order
    drawSkySolid();       // single bluish sky
    drawGround();
    drawTrees();
    drawBenches();
    drawBuilding();
    drawConnectorRoad();
    drawMainRoad();
    drawRoadLamps();

    //drawTank();
  // Example inside display(), after you draw the road
    drawMainRoad();
 float tankY = ROAD_BOTTOM + 2.0f;   // sits on road
    drawTankDetailed(680.0f, tankY, 1.0f, 0.0f); // x, y, scale, rotation

    glutSwapBuffers(); // swap at the end
}

// ---------- Timer ----------
void timer(int) {
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0); // ~60 FPS
}
*/



void display() {
    glClear(GL_COLOR_BUFFER_BIT);  // 2D: no depth clear needed
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // back -> front
  //  drawSkySolid();
  // drawSkySolid();
drawSkyGradient();

    drawGround();
    drawPlayground();
    drawTrees();
    drawBenches();
    drawBuilding();
    drawConnectorRoad();

    drawMainRoad(); // draw once
 // Lamps above road & tank so glow overlays nicely
    drawRoadLamps();
    // Tank sits on road: bottom of tracks (y+20) == ROAD_TOP
   // float tankY = ROAD_TOP - 230.0f;
    drawMainRoad();
  drawTankDetailed(/*x=*/80, /*y=*/ROAD_BOTTOM - 16, /*scale=*/1.0f, /*angleDeg=*/0.0f);

   // drawTankDetailed(680.0f, tankY, 1.0f, 0.0f);



    glutSwapBuffers();
}





// ---------- Main ----------
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB); // double buffered
    glutInitWindowSize(WIDTH, HEIGHT);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("Cantonment Frame (refactored)");
    glShadeModel(GL_SMOOTH); // ensure vertex colors interpolate
    glEnable(GL_DITHER);     // helps reduce visible banding on some GPUs

 //trying to change

    // background color (also acts as fallback sky if you remove drawSkySolid)
    glClearColor(0.6f, 0.8f, 1.0f, 1.0f);

    // 2D setup
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIDTH, 0, HEIGHT);

    // enable blending so lamp glow works
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

//    glutTimerFunc(0, timer, 0);
    glutDisplayFunc(display);
    glutKeyboardFunc(onKeyboard);
    glutMouseFunc(onMouse);

    glutMainLoop();
    return 0;
}
