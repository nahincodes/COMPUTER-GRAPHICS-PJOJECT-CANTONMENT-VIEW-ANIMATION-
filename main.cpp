#include <GL/glut.h>
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
static int gW = 1000, gH = 800;


// window size
const int WIDTH = 1920;
const int HEIGHT = 1080;


void init() {
    glClearColor(0.79f, 0.87f, 0.91f, 1.0f); // sky blue background
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIDTH, 0, HEIGHT); // orthographic projection
}



// Hills made as: one long base polygon + many triangles on top.
// Minimal variables; mostly raw coordinates (1920x1080 canvas).
void drawHillsPolygonAndTriangles() {
    const int h = (int)(HEIGHT * 0.70f); // horizon (same as your ground top)

    // 1) Base ridge (long polygon) — dark green band along the horizon
    glColor3f(0.16f, 0.30f, 0.18f);
    glBegin(GL_POLYGON);
        glVertex2i(   0, h);
        glVertex2i( 200, h);
        glVertex2i( 360, h+50);
        glVertex2i( 640, h+40);
        glVertex2i( 920, h+60);
        glVertex2i(1200, h+45);
        glVertex2i(1480, h+55);
        glVertex2i(1720, h+35);
        glVertex2i(1920, h);
        glVertex2i(1920, h-10); // thin back edge down to horizon line
        glVertex2i(   0, h-10);
    glEnd();

    // 2) Peaks (triangles) — stacked on top of the base ridge
    glColor3f(0.20f, 0.40f, 0.22f);
    glBegin(GL_TRIANGLES);
        // left cluster
        glVertex2i(  40, h); glVertex2i( 180, h); glVertex2i( 110, h+160);
        glVertex2i( 210, h); glVertex2i( 360, h); glVertex2i( 285, h+120);
        glVertex2i( 380, h); glVertex2i( 560, h); glVertex2i( 470, h+150);

        // mid-left
        glVertex2i( 600, h); glVertex2i( 780, h); glVertex2i( 690, h+130);
        glVertex2i( 820, h); glVertex2i(1000, h); glVertex2i( 910, h+190);

        // center (tallest)
        glVertex2i(1040, h); glVertex2i(1220, h); glVertex2i(1130, h+210);

        // mid-right
        glVertex2i(1240, h); glVertex2i(1410, h); glVertex2i(1325, h+150);
        glVertex2i(1430, h); glVertex2i(1600, h); glVertex2i(1515, h+130);

        // right cluster
        glVertex2i(1620, h); glVertex2i(1780, h); glVertex2i(1700, h+170);
        glVertex2i(1800, h); glVertex2i(1920, h); glVertex2i(1860, h+110);
    glEnd();
}







// Draw road + green sides, but STOP them below the horizon
// so they don't sit on top of the hill base. Minimal variables, clear math.
void drawRoadAndGrassBelowHorizon() {
    // same horizon you used for ground (70% of height)
    const int   h   = (int)(HEIGHT * 0.70f);
    const float c   = WIDTH * 0.5f;   // screen center x

    // road half-widths: bottom (wide) and horizon (narrow)
    const float rb  = WIDTH * 0.32f;
    const float rtH = WIDTH * 0.09f;

    // NEW: stop the road a little below the horizon to leave a tan band
    const float topY = h - 10.0f;     // 60px gap; adjust if you want more/less

    // perspective interpolation:
    // half-width at y=topY should be between rb (at bottom) and rtH (at horizon)
    const float t    = topY / (float)h;               // 0..1 from bottom to horizon
    const float rtY  = rb + (rtH - rb) * t;           // half-width at y=topY

    // bottom and top x-limits of the road
    const float Lb = c - rb,  Rb = c + rb;            // bottom
    const float Lt = c - rtY, Rt = c + rtY;           // top (below horizon)

    // --- lighter tan road (trapezoid that ends at topY) ---
    glColor3f(0.82f, 0.76f, 0.65f);
    glBegin(GL_QUADS);
        glVertex2f(Lb, 0);       // bottom-left
        glVertex2f(Rb, 0);       // bottom-right
        glVertex2f(Rt, topY);    // top-right (below horizon)
        glVertex2f(Lt, topY);    // top-left
    glEnd();

    // Green pads on both sides, also ending at topY and narrowing correctly
    const float gapB = 40.0f;    // shoulder gap at bottom
    const float wB   = 300.0f;   // grass width at bottom
    const float s    = rtY / rb; // scale at topY (how much things shrink)

    // LEFT green trapezoid
    glColor3f(0.33f, 0.62f, 0.27f);
    glBegin(GL_QUADS);
        glVertex2f((Lb) - gapB - wB, 0);             // outer-bottom
        glVertex2f((Lb) - gapB,      0);             // inner-bottom
        glVertex2f((Lt) - gapB*s,    topY);          // inner-top
        glVertex2f((Lt) - gapB*s - wB*s, topY);      // outer-top
    glEnd();

    // RIGHT green trapezoid
    glBegin(GL_QUADS);
        glVertex2f((Rb) + gapB,       0);            // inner-bottom
        glVertex2f((Rb) + gapB + wB,  0);            // outer-bottom
        glVertex2f((Rt) + gapB*s + wB*s, topY);      // outer-top
        glVertex2f((Rt) + gapB*s,       topY);       // inner-top
    glEnd();
}










// Military watchtower: lattice legs with X-braces, cabin, roof.
// (x, y) = base center on the ground; s = scale (1.0 is a good start).
// Uses only GL_QUADS, GL_TRIANGLES, GL_LINES.
void drawWatchTower_Prims(float x, float y, float s)
{
    // ----- sizes (kept tiny & readable) -----
    float legW   = 12.0f * s;   // leg thickness
    float span   = 88.0f * s;   // distance between leg center lines
    float frameH = 260.0f * s;  // height of lattice section (legs)
    float slabH  = 14.0f * s;   // platform slab thickness
    float cabH   = 62.0f * s;   // cabin height
    float roofH  = 12.0f * s;   // roof thickness

    float xL = x - span * 0.5f; // left leg center
    float xR = x + span * 0.5f; // right leg center
    float yT = y + frameH;      // top of the lattice section

    // ----- legs (QUADS) -----
    glColor3f(0.58f, 0.47f, 0.34f); // brown
    glBegin(GL_QUADS); // left leg
        glVertex2f(xL - legW*0.5f, y);
        glVertex2f(xL + legW*0.5f, y);
        glVertex2f(xL + legW*0.5f, yT);
        glVertex2f(xL - legW*0.5f, yT);
    glEnd();
    glBegin(GL_QUADS); // right leg
        glVertex2f(xR - legW*0.5f, y);
        glVertex2f(xR + legW*0.5f, y);
        glVertex2f(xR + legW*0.5f, yT);
        glVertex2f(xR - legW*0.5f, yT);
    glEnd();

    // ----- X-braces (LINES) across four segments -----
    float s1 = y + frameH * 0.25f;
    float s2 = y + frameH * 0.50f;
    float s3 = y + frameH * 0.75f;
    glColor3f(0.45f, 0.36f, 0.26f);
    glBegin(GL_LINES);
        // segment 1
        glVertex2f(xL, y);  glVertex2f(xR, s1);
        glVertex2f(xR, y);  glVertex2f(xL, s1);
        // segment 2
        glVertex2f(xL, s1); glVertex2f(xR, s2);
        glVertex2f(xR, s1); glVertex2f(xL, s2);
        // segment 3
        glVertex2f(xL, s2); glVertex2f(xR, s3);
        glVertex2f(xR, s2); glVertex2f(xL, s3);
        // segment 4
        glVertex2f(xL, s3); glVertex2f(xR, yT);
        glVertex2f(xR, s3); glVertex2f(xL, yT);
    glEnd();

    // optional horizontal rails (thin lines)
    glBegin(GL_LINES);
        glVertex2f(xL, y);   glVertex2f(xR, y);
        glVertex2f(xL, s1);  glVertex2f(xR, s1);
        glVertex2f(xL, s2);  glVertex2f(xR, s2);
        glVertex2f(xL, s3);  glVertex2f(xR, s3);
        glVertex2f(xL, yT);  glVertex2f(xR, yT);
    glEnd();

    // ----- platform slab (QUAD) -----
    glColor3f(0.52f, 0.41f, 0.29f);
    glBegin(GL_QUADS);
        glVertex2f(xL - 10.0f*s, yT);
        glVertex2f(xR + 10.0f*s, yT);
        glVertex2f(xR + 10.0f*s, yT + slabH);
        glVertex2f(xL - 10.0f*s, yT + slabH);
    glEnd();

    // ----- cabin (windows as light rectangles) -----
    float cabTop = yT + slabH + cabH;
    glColor3f(0.58f, 0.47f, 0.34f); // cabin body
    glBegin(GL_QUADS);
        glVertex2f(xL - 16.0f*s, yT + slabH);
        glVertex2f(xR + 16.0f*s, yT + slabH);
        glVertex2f(xR + 16.0f*s, cabTop);
        glVertex2f(xL - 16.0f*s, cabTop);
    glEnd();

    // windows (two panes)
    glColor3f(0.78f, 0.86f, 0.92f);
    glBegin(GL_QUADS); // left window
        glVertex2f(x - 30.0f*s, yT + slabH + 14.0f*s);
        glVertex2f(x -  4.0f*s, yT + slabH + 14.0f*s);
        glVertex2f(x -  4.0f*s, yT + slabH + 38.0f*s);
        glVertex2f(x - 30.0f*s, yT + slabH + 38.0f*s);
    glEnd();
    glBegin(GL_QUADS); // right window
        glVertex2f(x +  4.0f*s, yT + slabH + 14.0f*s);
        glVertex2f(x + 30.0f*s, yT + slabH + 14.0f*s);
        glVertex2f(x + 30.0f*s, yT + slabH + 38.0f*s);
        glVertex2f(x +  4.0f*s, yT + slabH + 38.0f*s);
    glEnd();

    // window mullion (center post)
    glColor3f(0.45f, 0.36f, 0.26f);
    glBegin(GL_LINES);
        glVertex2f(x, yT + slabH + 12.0f*s);
        glVertex2f(x, cabTop - 12.0f*s);
    glEnd();

    // ----- roof (slight overhang) -----
    glColor3f(0.40f, 0.31f, 0.23f);
    glBegin(GL_QUADS);
        glVertex2f(xL - 28.0f*s, cabTop);
        glVertex2f(xR + 28.0f*s, cabTop);
        glVertex2f(xR + 28.0f*s, cabTop + roofH);
        glVertex2f(xL - 28.0f*s, cabTop + roofH);
    glEnd();

    // ground contact line (seats tower visually)
    glColor3f(0.18f, 0.20f, 0.16f);
    glBegin(GL_LINES);
        glVertex2f(xL - 10.0f*s, y);
        glVertex2f(xR + 10.0f*s, y);
    glEnd();
}


// Draw a Quonset-hut style military tent.
// Assumes an orthographic projection in pixel coordinates:
// (0,0) at bottom-left, (W,H) at top-right.
void drawQuonsetTent(float W, float H) {
    // -------------------------------
    // 1) Basic placement and sizes
    // -------------------------------

    // Left edge of the tent (you asked to start at x = 20 px)
    const float xOffset = 20.0f;

    // The tent's roof PEAK should reach y = 750 px
    const float yTop    = 750.0f;

    // Tent width (in pixels). Change this if you want it wider/narrower.
    const float hutWidth  = 280.0f;

    // For a Quonset hut (half-circle top), radius = width / 2
    const float r  = hutWidth * 0.5f;

    // Left and right x of the tent base line
    const float xL = xOffset;
    const float xR = xOffset + hutWidth;

    // Center x (middle of the half-circle)
    const float xc = 0.5f * (xL + xR);

    // The base y of the tent: base + radius = peak (yTop)
    // so base = yTop - radius
    const float yBase = yTop - r;

    // -------------------------------
    // 2) Simple colors
    // -------------------------------
    const float canvas[3]  = {0.24f, 0.44f, 0.24f}; // main green
    const float canvas2[3] = {0.20f, 0.38f, 0.22f}; // inner shade
    const float trim[3]    = {0.12f, 0.20f, 0.14f}; // outline
    const float doorC[3]   = {0.10f, 0.14f, 0.18f}; // door
    const float windowC[3] = {0.16f, 0.20f, 0.24f}; // windows

    // -------------------------------
    // 3) Main hut body (half-circle made of triangles)
    //    We use a triangle fan: one center point + many edge points.
    // -------------------------------
    const int segments = 64;           // more = smoother curve
    glColor3f(canvas[0], canvas[1], canvas[2]);
    glBegin(GL_TRIANGLE_FAN);
        // Center of the fan sits on the base line at the middle
        glVertex2f(xc, yBase);

        // Generate points along the half-circle from left to right (0..π)
        for (int i = 0; i <= segments; ++i) {
            float t = (float)i / (float)segments; // 0..1
            float a = (float)M_PI * t;            // 0..π
            float x = xc + r * std::cos(a);
            float y = yBase + r * std::sin(a);
            glVertex2f(x, y);
        }
    glEnd();

    // -------------------------------
    // 4) Inner shading (a smaller half-circle)
    //    This gives a bit of depth without complicated lighting.
    // -------------------------------
    const float inset = 15.0f; // how much smaller the inner arc is
    glColor3f(canvas2[0], canvas2[1], canvas2[2]);
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(xc, yBase + 5.0f); // nudge center up a bit
        for (int i = 0; i <= segments; ++i) {
            float t = (float)i / (float)segments;
            float a = (float)M_PI * t;
            float x = xc + (r - inset) * std::cos(a);
            float y = yBase + (r - inset) * std::sin(a);
            glVertex2f(x, y);
        }
    glEnd();

    // -------------------------------
    // 5) Door (simple dark rectangle on the right side)
    // -------------------------------
    const float doorW = 40.0f;
    const float doorH = 90.0f;

    // Place door roughly on the right third of the hut
    const float xDoor = xR - 0.25f * hutWidth - 0.5f * doorW;
    const float yDoor = yBase;

    glColor3f(doorC[0], doorC[1], doorC[2]);
    glBegin(GL_QUADS);
        glVertex2f(xDoor,            yDoor);
        glVertex2f(xDoor + doorW,    yDoor);
        glVertex2f(xDoor + doorW,    yDoor + doorH);
        glVertex2f(xDoor,            yDoor + doorH);
    glEnd();

    // -------------------------------
    // 6) Two small windows (rectangles) on the left
    // -------------------------------
    const float winW = 35.0f;
    const float winH = 22.0f;

    // Window y is somewhere above the middle of the wall
    const float yWin  = yBase + 0.4f * r;

    // Window x positions (left side, spaced apart)
    const float xWin1 = xL + 0.25f * hutWidth - 0.5f * winW;
    const float xWin2 = xL + 0.45f * hutWidth - 0.5f * winW;

    glColor3f(windowC[0], windowC[1], windowC[2]);
    glBegin(GL_QUADS);
        // left window
        glVertex2f(xWin1,          yWin);
        glVertex2f(xWin1 + winW,   yWin);
        glVertex2f(xWin1 + winW,   yWin + winH);
        glVertex2f(xWin1,          yWin + winH);
        // right window
        glVertex2f(xWin2,          yWin);
        glVertex2f(xWin2 + winW,   yWin);
        glVertex2f(xWin2 + winW,   yWin + winH);
        glVertex2f(xWin2,          yWin + winH);
    glEnd();

    // -------------------------------
    // 7) Outline of the half-circle (a simple line strip)
    // -------------------------------
    glColor3f(trim[0], trim[1], trim[2]);
    glBegin(GL_LINE_STRIP);
        for (int i = 0; i <= segments; ++i) {
            float t = (float)i / (float)segments;
            float a = (float)M_PI * t;
            float x = xc + r * std::cos(a);
            float y = yBase + r * std::sin(a);
            glVertex2f(x, y);
        }
    glEnd();

    // Base line (left to right)
    glBegin(GL_LINES);
        glVertex2f(xL, yBase);
        glVertex2f(xR, yBase);
    glEnd();
}





// ======================================================================
// RIGHT-FACING HUMVEE (primitives only; literal vertices; flat colors)
// Footprint ~ X: 36..330, Y: 386..596   (within your 30..330, 370..600 budget)
// No GL_LINES, no glutSolid*, no glu*. Immediate mode only.
// ======================================================================
void drawHumvee_Right_Prims()
{


    // =========================
    // ====== TIRES ============
    // =========================
    // ---- Rear tire (smooth ring) ----
    glColor3f(0.07f, 0.08f, 0.09f);           // tire
    glBegin(GL_POLYGON);
        glVertex2i(128,420); glVertex2i(124,433); glVertex2i(116,444); glVertex2i(104,452);
        glVertex2i( 90,454); glVertex2i( 76,452); glVertex2i( 64,444); glVertex2i( 56,433);
        glVertex2i( 52,420); glVertex2i( 56,407); glVertex2i( 64,396); glVertex2i( 76,388);
        glVertex2i( 90,386); glVertex2i(104,388); glVertex2i(116,396); glVertex2i(124,407);
    glEnd();
    glColor3f(0.38f, 0.36f, 0.30f);           // outer rim (tan)
    glBegin(GL_POLYGON);
        glVertex2i(116,420); glVertex2i(112,430); glVertex2i(104,438); glVertex2i( 92,442);
        glVertex2i( 88,442); glVertex2i( 76,438); glVertex2i( 68,430); glVertex2i( 64,420);
        glVertex2i( 68,410); glVertex2i( 76,402); glVertex2i( 88,398); glVertex2i( 92,398);
        glVertex2i(104,402); glVertex2i(112,410);
    glEnd();
    glColor3f(0.30f, 0.28f, 0.22f);           // inner rim
    glBegin(GL_POLYGON);
        glVertex2i(108,420); glVertex2i(105,427); glVertex2i( 98,432); glVertex2i( 90,434);
        glVertex2i( 82,432); glVertex2i( 75,427); glVertex2i( 72,420); glVertex2i( 75,413);
        glVertex2i( 82,408); glVertex2i( 90,406); glVertex2i( 98,408); glVertex2i(105,413);
    glEnd();
    glColor3f(0.66f, 0.63f, 0.46f);           // hub
    glBegin(GL_POLYGON);
        glVertex2i(100,420); glVertex2i( 97,425); glVertex2i( 90,428); glVertex2i( 83,425);
        glVertex2i( 80,420); glVertex2i( 83,415); glVertex2i( 90,412); glVertex2i( 97,415);
    glEnd();
    glColor3f(0.18f, 0.18f, 0.16f);           // lug dots (tiny quads)
    glBegin(GL_QUADS);
        glVertex2i( 90,432); glVertex2i( 92,432); glVertex2i( 92,430); glVertex2i( 90,430);
        glVertex2i(101,425); glVertex2i(103,425); glVertex2i(103,423); glVertex2i(101,423);
        glVertex2i( 79,425); glVertex2i( 81,425); glVertex2i( 81,423); glVertex2i( 79,423);
        glVertex2i( 90,410); glVertex2i( 92,410); glVertex2i( 92,408); glVertex2i( 90,408);
    glEnd();

    // ---- Front tire (smooth ring) ----
    glColor3f(0.07f, 0.08f, 0.09f);           // tire
    glBegin(GL_POLYGON);
        glVertex2i(288,420); glVertex2i(284,433); glVertex2i(276,444); glVertex2i(264,452);
        glVertex2i(250,454); glVertex2i(236,452); glVertex2i(224,444); glVertex2i(216,433);
        glVertex2i(212,420); glVertex2i(216,407); glVertex2i(224,396); glVertex2i(236,388);
        glVertex2i(250,386); glVertex2i(264,388); glVertex2i(276,396); glVertex2i(284,407);
    glEnd();
    glColor3f(0.38f, 0.36f, 0.30f);           // outer rim
    glBegin(GL_POLYGON);
        glVertex2i(276,420); glVertex2i(272,430); glVertex2i(264,438); glVertex2i(252,442);
        glVertex2i(248,442); glVertex2i(236,438); glVertex2i(228,430); glVertex2i(224,420);
        glVertex2i(228,410); glVertex2i(236,402); glVertex2i(248,398); glVertex2i(252,398);
        glVertex2i(264,402); glVertex2i(272,410);
    glEnd();
    glColor3f(0.30f, 0.28f, 0.22f);           // inner rim
    glBegin(GL_POLYGON);
        glVertex2i(268,420); glVertex2i(265,427); glVertex2i(258,432); glVertex2i(250,434);
        glVertex2i(242,432); glVertex2i(235,427); glVertex2i(232,420); glVertex2i(235,413);
        glVertex2i(242,408); glVertex2i(250,406); glVertex2i(258,408); glVertex2i(265,413);
    glEnd();
    glColor3f(0.66f, 0.63f, 0.46f);           // hub
    glBegin(GL_POLYGON);
        glVertex2i(260,420); glVertex2i(257,425); glVertex2i(250,428); glVertex2i(243,425);
        glVertex2i(240,420); glVertex2i(243,415); glVertex2i(250,412); glVertex2i(257,415);
    glEnd();
    glColor3f(0.18f, 0.18f, 0.16f);           // lug dots
    glBegin(GL_QUADS);
        glVertex2i(250,432); glVertex2i(252,432); glVertex2i(252,430); glVertex2i(250,430);
        glVertex2i(261,425); glVertex2i(263,425); glVertex2i(263,423); glVertex2i(261,423);
        glVertex2i(239,425); glVertex2i(241,425); glVertex2i(241,423); glVertex2i(239,423);
        glVertex2i(250,410); glVertex2i(252,410); glVertex2i(252,408); glVertex2i(250,408);
    glEnd();

    // =========================
    // ====== FENDERS ==========
    // =========================
    glColor3f(0.08f, 0.09f, 0.10f);           // rear black fender plate
    glBegin(GL_POLYGON);
        glVertex2i( 42,452); glVertex2i(148,452);
        glVertex2i(132,514); glVertex2i( 58,514);
    glEnd();
    glBegin(GL_POLYGON);                       // front black fender plate
        glVertex2i(202,452); glVertex2i(310,452);
        glVertex2i(294,514); glVertex2i(220,514);
    glEnd();

    // =========================
    // ====== BODY =============
    // =========================
    glColor3f(0.16f, 0.24f, 0.24f);           // lower armored hull
    glBegin(GL_QUADS);
        glVertex2i( 36,452); glVertex2i(305,452);
        glVertex2i(305,520); glVertex2i( 36,520);
    glEnd();

    // angled sill block (door→fender transition)
    glColor3f(0.19f, 0.28f, 0.28f);
    glBegin(GL_POLYGON);
        glVertex2i(188,452); glVertex2i(210,452);
        glVertex2i(206,502); glVertex2i(192,520);
        glVertex2i(180,520); glVertex2i(180,452);
    glEnd();

    // long, low hood (dark green)
    glColor3f(0.18f, 0.27f, 0.27f);
    glBegin(GL_POLYGON);
        glVertex2i(206,520); glVertex2i(305,520);
        glVertex2i(295,534); glVertex2i(208,534);
    glEnd();
    // hood spine strip
    glColor3f(0.21f, 0.31f, 0.31f);
    glBegin(GL_QUADS);
        glVertex2i(208,534); glVertex2i(295,534);
        glVertex2i(295,538); glVertex2i(208,538);
    glEnd();

    // nose wedge + bumper
    glColor3f(0.16f, 0.24f, 0.24f);           // wedge
    glBegin(GL_POLYGON);
        glVertex2i(305,520); glVertex2i(330,514);
        glVertex2i(330,472); glVertex2i(306,476);
    glEnd();
    glColor3f(0.08f, 0.09f, 0.10f);           // bumper bar
    glBegin(GL_QUADS);
        glVertex2i(304,460); glVertex2i(332,460);
        glVertex2i(332,468); glVertex2i(304,468);
    glEnd();

    // small amber/marker lights
    glColor3f(0.96f, 0.64f, 0.20f);
    glBegin(GL_QUADS);
        glVertex2i(316,504); glVertex2i(322,504);
        glVertex2i(322,498); glVertex2i(316,498);
    glEnd();
    glColor3f(0.93f, 0.44f, 0.18f);
    glBegin(GL_QUADS);
        glVertex2i(312,486); glVertex2i(318,486);
        glVertex2i(318,482); glVertex2i(312,482);
    glEnd();

    // =========================
    // ====== DOORS ============
    // =========================
    // two big armored doors
    glColor3f(0.17f, 0.25f, 0.25f);
    glBegin(GL_QUADS); // front door
        glVertex2i( 92,466); glVertex2i(188,466);
        glVertex2i(188,520); glVertex2i( 92,520);
    glEnd();
    glBegin(GL_QUADS); // rear/center door
        glVertex2i(188,466); glVertex2i(244,466);
        glVertex2i(244,520); glVertex2i(188,520);
    glEnd();

    // X bracing (thin quads instead of lines)
    glColor3f(0.22f, 0.31f, 0.31f);
    // front door X
    glBegin(GL_QUADS);                         // \
        glVertex2i( 96,470); glVertex2i(104,470);
        glVertex2i(180,516); glVertex2i(172,516);
    glEnd();
    glBegin(GL_QUADS);                         // /
        glVertex2i(172,470); glVertex2i(180,470);
        glVertex2i(104,516); glVertex2i( 96,516);
    glEnd();
    // rear door X
    glBegin(GL_QUADS);                         // \
        glVertex2i(192,470); glVertex2i(198,470);
        glVertex2i(238,516); glVertex2i(232,516);
    glEnd();
    glBegin(GL_QUADS);                         // /
        glVertex2i(232,470); glVertex2i(238,470);
        glVertex2i(198,516); glVertex2i(192,516);
    glEnd();

    // hinge posts & handles (thin quads)
    glColor3f(0.10f, 0.12f, 0.12f);
    glBegin(GL_QUADS);                         // hinges
        glVertex2i( 88,466); glVertex2i( 92,466); glVertex2i( 92,520); glVertex2i( 88,520);
        glVertex2i(188,466); glVertex2i(192,466); glVertex2i(192,520); glVertex2i(188,520);
    glEnd();
    glBegin(GL_QUADS);                         // small handles
        glVertex2i(176,490); glVertex2i(184,490); glVertex2i(184,494); glVertex2i(176,494);
        glVertex2i(232,490); glVertex2i(238,490); glVertex2i(238,494); glVertex2i(232,494);
    glEnd();

    // =========================
    // ====== WINDOWS ==========
    // =========================
    // window band/frame
    glColor3f(0.20f, 0.30f, 0.30f);
    glBegin(GL_QUADS);
        glVertex2i(100,515); glVertex2i(236,515);
        glVertex2i(232,544); glVertex2i(104,544);
    glEnd();

    // panes (light glass) + mullions (thin quads)
    glColor3f(0.82f, 0.88f, 0.91f);           // glass
    glBegin(GL_QUADS); // front pane
        glVertex2i(108,524); glVertex2i(168,524);
        glVertex2i(168,540); glVertex2i(108,540);
    glEnd();
    glBegin(GL_QUADS); // rear pane
        glVertex2i(176,524); glVertex2i(232,524);
        glVertex2i(232,540); glVertex2i(176,540);
    glEnd();
    glColor3f(0.12f, 0.14f, 0.14f);           // mullions
    glBegin(GL_QUADS);
        glVertex2i(136,524); glVertex2i(140,524); glVertex2i(140,540); glVertex2i(136,540);
        glVertex2i(202,524); glVertex2i(206,524); glVertex2i(206,540); glVertex2i(202,540);
    glEnd();

    // =========================
    // ====== ROOF / TURRET ====
    // =========================
    // twin roof lumps
    glColor3f(0.18f, 0.27f, 0.27f);
    glBegin(GL_QUADS);
        glVertex2i(100,544); glVertex2i(196,544);
        glVertex2i(196,552); glVertex2i(100,552);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2i(196,544); glVertex2i(256,544);
        glVertex2i(256,552); glVertex2i(196,552);
    glEnd();

    // turret pedestal
    glColor3f(0.19f, 0.28f, 0.28f);
    glBegin(GL_QUADS);
        glVertex2i(192,552); glVertex2i(208,552);
        glVertex2i(208,560); glVertex2i(192,560);
    glEnd();

    // turret box + top lip
    glColor3f(0.22f, 0.31f, 0.31f);
    glBegin(GL_QUADS);
        glVertex2i(160,560); glVertex2i(240,560);
        glVertex2i(240,588); glVertex2i(160,588);
    glEnd();
    glColor3f(0.14f, 0.20f, 0.20f);
    glBegin(GL_QUADS);
        glVertex2i(158,588); glVertex2i(242,588);
        glVertex2i(242,592); glVertex2i(158,592);
    glEnd();

    // gun box + barrel (RIGHT)
    glColor3f(0.09f, 0.10f, 0.11f);
    glBegin(GL_QUADS);                          // gun box on the turret
        glVertex2i(226,584); glVertex2i(240,584);
        glVertex2i(240,596); glVertex2i(226,596);
    glEnd();
    glBegin(GL_QUADS);                          // barrel (thin quad)
        glVertex2i(240,590); glVertex2i(328,590);
        glVertex2i(328,594); glVertex2i(240,594);
    glEnd();
    glBegin(GL_TRIANGLES);                      // small stock wedge
        glVertex2i(240,594); glVertex2i(246,598); glVertex2i(240,590);
    glEnd();
    glBegin(GL_QUADS);                          // muzzle block
        glVertex2i(328,588); glVertex2i(332,588);
        glVertex2i(332,596); glVertex2i(328,596);
    glEnd();

    // =========================
    // ====== MIRRORS ==========
    // =========================
    glColor3f(0.09f, 0.10f, 0.11f);             // right mirror
    glBegin(GL_QUADS);
        glVertex2i(290,535); glVertex2i(300,535);
        glVertex2i(300,545); glVertex2i(290,545);
    glEnd();
    glColor3f(0.09f, 0.10f, 0.11f);             // small left antenna stump (optional)
    glBegin(GL_QUADS);
        glVertex2i( 60,520); glVertex2i( 62,520);
        glVertex2i( 62,596); glVertex2i( 60,596);
    glEnd();
}















/*

// ====== primitives helpers (still primitives-only) ======
static inline void rectf(float x, float y, float w, float h){
    glBegin(GL_QUADS);
      glVertex2f(x,   y);
      glVertex2f(x+w, y);
      glVertex2f(x+w, y+h);
      glVertex2f(x,   y+h);
    glEnd();
}
static inline void rectOutline(float x, float y, float w, float h, float lw=2.0f){
    glLineWidth(lw);
    glBegin(GL_LINE_LOOP);
      glVertex2f(x,   y);
      glVertex2f(x+w, y);
      glVertex2f(x+w, y+h);
      glVertex2f(x,   y+h);
    glEnd();
}

// ====== 2D building on right-side ground (centered in that strip) ======
void drawRightSideBuilding2D(float c, float h, float rb, float rt, float gap, float grassWidth)
{
    // Right road shoulder (bottom & horizon)
    const float xR0 = c + rb + gap;          // bottom
    // const float xR1 = c + rt + gap;       // horizon (not needed for 2D)

    // Right-side ground (green strip) spans [xR0, xR0 + grassWidth] at the bottom.
    const float stripMid = xR0 + grassWidth * 0.5f;

    // ---- Building footprint (axis-aligned) ----
    const float B_W = 160.0f;     // building width
    const float B_H = 320.0f;     // building height
    const float x0  = stripMid - B_W * 0.5f;
    const float y0  = 0.0f;       // rests on ground

    // Base (foundation)
    glColor3f(0.30f, 0.30f, 0.30f);
    rectf(x0 - 8, y0 - 10, B_W + 16, 10);

    // Body
    glColor3f(0.70f, 0.70f, 0.68f);
    rectf(x0, y0, B_W, B_H);
    glColor3f(0.15f, 0.15f, 0.15f);
    rectOutline(x0, y0, B_W, B_H, 2.0f);

    // Simple triangular roof
    glColor3f(0.62f, 0.62f, 0.60f);
    glBegin(GL_TRIANGLES);
        glVertex2f(x0 - 6,     y0 + B_H);
        glVertex2f(x0 + B_W+6, y0 + B_H);
        glVertex2f(x0 + B_W*0.5f, y0 + B_H + 36.0f);
    glEnd();
    glColor3f(0.15f, 0.15f, 0.15f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x0 - 6,     y0 + B_H);
        glVertex2f(x0 + B_W+6, y0 + B_H);
        glVertex2f(x0 + B_W*0.5f, y0 + B_H + 36.0f);
    glEnd();

    // Door
    const float DW = 30.0f, DH = 70.0f;
    glColor3f(0.35f, 0.35f, 0.35f);
    rectf(x0 + 12, y0, DW, DH);
    glColor3f(0.12f, 0.12f, 0.12f);
    rectOutline(x0 + 12, y0, DW, DH, 1.5f);

    // Windows grid (front only, simple rectangles)
    const int floors = 6;
    const int cols   = 3;
    const float mL = 16.0f, mR = 16.0f, mT = 20.0f, mB = 20.0f, gX = 10.0f, gY = 12.0f;
    const float cellW = (B_W - mL - mR - (cols-1)*gX) / cols;
    const float cellH = (B_H - mT - mB - (floors-1)*gY) / floors;

    for (int r=0; r<floors; ++r){
        for (int ccol=0; ccol<cols; ++ccol){
            const float wx = x0 + mL + ccol*(cellW + gX);
            const float wy = y0 + mB + r*(cellH + gY);
            glColor3f(0.20f, 0.28f, 0.33f);     // glass
            rectf(wx, wy, cellW, cellH);
            glColor3f(0.10f, 0.10f, 0.10f);     // frame
            rectOutline(wx, wy, cellW, cellH, 1.2f);
        }
    }
}
*/











// ======================================================================
// TRAINING BUILDING (strict 2D, primitives only, literal vertices)
// Footprint ~ X: 1600..1888 (facade), up to ~825 in Y (roof apex)
// Layers: base/steps -> facade -> pilasters -> windows -> door -> parapet/roof
// ======================================================================
void drawTrainingBuilding_Prims()
{
    // ---------------------------
    // Footing / steps (at y=500)
    // ---------------------------
    glColor3f(0.40f, 0.38f, 0.34f); // lower step
    glBegin(GL_QUADS);
        glVertex2i(1592, 500); glVertex2i(1896, 500);
        glVertex2i(1896, 506); glVertex2i(1592, 506);
    glEnd();

    glColor3f(0.46f, 0.44f, 0.40f); // upper step
    glBegin(GL_QUADS);
        glVertex2i(1596, 506); glVertex2i(1892, 506);
        glVertex2i(1892, 512); glVertex2i(1596, 512);
    glEnd();

    // ---------------------------
    // Main facade (3 floors)
    // ---------------------------
    glColor3f(0.72f, 0.70f, 0.64f); // warm sand/khaki
    glBegin(GL_QUADS);
        glVertex2i(1600, 512); glVertex2i(1888, 512);
        glVertex2i(1888, 780); glVertex2i(1600, 780);
    glEnd();

    // Vertical pilasters (flat accents)
    glColor3f(0.68f, 0.66f, 0.60f);
    glBegin(GL_QUADS); // left pilaster
        glVertex2i(1608, 512); glVertex2i(1628, 512);
        glVertex2i(1628, 780); glVertex2i(1608, 780);
    glEnd();
    glBegin(GL_QUADS); // center pilaster
        glVertex2i(1736, 512); glVertex2i(1752, 512);
        glVertex2i(1752, 780); glVertex2i(1736, 780);
    glEnd();
    glBegin(GL_QUADS); // right pilaster
        glVertex2i(1860, 512); glVertex2i(1880, 512);
        glVertex2i(1880, 780); glVertex2i(1860, 780);
    glEnd();

    // ---------------------------------
    // Parapet band and triangular roof
    // ---------------------------------
    glColor3f(0.55f, 0.53f, 0.49f); // parapet cap
    glBegin(GL_QUADS);
        glVertex2i(1600, 780); glVertex2i(1888, 780);
        glVertex2i(1888, 795); glVertex2i(1600, 795);
    glEnd();

    glColor3f(0.35f, 0.34f, 0.30f); // simple gable roof
    glBegin(GL_TRIANGLES);
        glVertex2i(1600, 795);  // left eave
        glVertex2i(1888, 795);  // right eave
        glVertex2i(1744, 825);  // apex
    glEnd();

    // ---------------------------
    // Door (double) – centered
    // ---------------------------
    glColor3f(0.28f, 0.32f, 0.32f); // door frame block
    glBegin(GL_QUADS);
        glVertex2i(1710, 512); glVertex2i(1778, 512);
        glVertex2i(1778, 572); glVertex2i(1710, 572);
    glEnd();

    glColor3f(0.22f, 0.26f, 0.26f); // left leaf
    glBegin(GL_QUADS);
        glVertex2i(1714, 512); glVertex2i(1742, 512);
        glVertex2i(1742, 572); glVertex2i(1714, 572);
    glEnd();
    glBegin(GL_QUADS);             // right leaf
        glVertex2i(1746, 512); glVertex2i(1774, 512);
        glVertex2i(1774, 572); glVertex2i(1746, 572);
    glEnd();

    // -------------
    // Windows (glass)
    // -------------
    glColor3f(0.78f, 0.86f, 0.93f); // light blue/gray

    // Ground floor side windows (left/right of door)
    glBegin(GL_QUADS); // left
        glVertex2i(1620, 540); glVertex2i(1670, 540);
        glVertex2i(1670, 570); glVertex2i(1620, 570);
    glEnd();
    glBegin(GL_QUADS); // right
        glVertex2i(1818, 540); glVertex2i(1868, 540);
        glVertex2i(1868, 570); glVertex2i(1818, 570);
    glEnd();

    // Second floor: three windows
    glBegin(GL_QUADS);
        glVertex2i(1620, 612); glVertex2i(1670, 612);
        glVertex2i(1670, 642); glVertex2i(1620, 642);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2i(1714, 612); glVertex2i(1764, 612);
        glVertex2i(1764, 642); glVertex2i(1714, 642);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2i(1818, 612); glVertex2i(1868, 612);
        glVertex2i(1868, 642); glVertex2i(1818, 642);
    glEnd();

    // Third floor: three windows
    glBegin(GL_QUADS);
        glVertex2i(1620, 686); glVertex2i(1670, 686);
        glVertex2i(1670, 716); glVertex2i(1620, 716);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2i(1714, 686); glVertex2i(1764, 686);
        glVertex2i(1764, 716); glVertex2i(1714, 716);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2i(1818, 686); glVertex2i(1868, 686);
        glVertex2i(1868, 716); glVertex2i(1818, 716);
    glEnd();

    // Fourth floor (attic band) – two smaller windows
    glBegin(GL_QUADS);
        glVertex2i(1660, 740); glVertex2i(1706, 740);
        glVertex2i(1706, 764); glVertex2i(1660, 764);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2i(1782, 740); glVertex2i(1828, 740);
        glVertex2i(1828, 764); glVertex2i(1782, 764);
    glEnd();
}

































void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // --- Minimal layout knobs ---
    const int   h   = (int)(HEIGHT * 0.70f); // horizon (ground height)
    const float c   = WIDTH * 0.5f;          // screen center x
    const float rb  = WIDTH * 0.32f;         // road half-width at bottom (wide)
    const float rt  = WIDTH * 0.09f;         // road half-width at horizon (narrow)
    const float gap = 40.0f;                 // shoulder gap from road
    const float w   = 300.0f;                // grass width at bottom (picked to stay on-screen)
/*
    // --- 1) Tan ground (bottom rectangle) ---
    glColor3f(0.80f, 0.72f, 0.59f);
    glBegin(GL_QUADS);
        glVertex2i(0, 0);
        glVertex2i(WIDTH, 0);
        glVertex2i(WIDTH, h);
        glVertex2i(0, h);
    glEnd();
*/


// --- Tan ground (your code) ---
glColor3f(0.80f, 0.72f, 0.59f);
glBegin(GL_QUADS);
    glVertex2i(0, 0);
    glVertex2i(WIDTH, 0);
    glVertex2i(WIDTH, (int)(HEIGHT*0.70f));  // horizon
    glVertex2i(0, (int)(HEIGHT*0.70f));
glEnd();




////////

// --- Hills made of base polygon + triangles ---
drawHillsPolygonAndTriangles();

// --- Then your road + green sides ---




    // ground (your code)

// hills (your code)
//drawHillsPolygonAndTriangles();

drawRoadAndGrassBelowHorizon();     // your road/grass function



// WATCHTOWERS (place them where you want)
drawWatchTower_Prims(1750.0f, 0.0f, 1.0f);  // right side, on ground
drawWatchTower_Prims( 170.0f, 0.0f, 1.0f);  // left side, on ground (optional)


// ===== WATCHTOWERS on the far side (along the horizon) =====


//            (x-position,  base y,  scale)
// smaller scale (~0.7) makes them look further away
drawWatchTower_Prims( 320.0f, 750, 0.60f);  // left, opposite side x,y,
drawWatchTower_Prims( 960.0f, 750, 0.60f);  // middle, opposite side
drawWatchTower_Prims(1600.0f, 750, 0.60f);  // right, opposite side

     // Your tent (upper-left, around y 610–750)
    drawQuonsetTent(gW, gH);


// ... after your ground/scene is drawn:
drawHumvee_Right_Prims();


    // ... draw ground, hills, road, etc.

    // >>> Basic function call to draw the 2D building:
    //drawRightSideBuilding2D(c, h, rb, rt, gap, w);




drawTrainingBuilding_Prims();



    glutSwapBuffers();
}


int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Military Base - Ground & Sky");

    init();
    glutDisplayFunc(display);
    glutMainLoop();
    return 0;
}
