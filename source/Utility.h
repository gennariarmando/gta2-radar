#pragma once
#include "common.h"
#include "Other.h"

#define DEFAULT_SCREEN_WIDTH 640.0f
#define DEFAULT_SCREEN_HEIGHT 480.0f
#define DEFAULT_SCREEN_ASPECT_RATIO (DEFAULT_SCREEN_WIDTH / DEFAULT_SCREEN_HEIGHT)
#define SCREEN_ASPECT_RATIO (SCREEN_WIDTH / SCREEN_HEIGHT)

static float ScaleX(float x) {
    float f = ((x) * (float)SCREEN_WIDTH / DEFAULT_SCREEN_WIDTH) * DEFAULT_SCREEN_ASPECT_RATIO / SCREEN_ASPECT_RATIO;
    return f;
}

static float ScaleXKeepCentered(float x) {
    float f = ((SCREEN_WIDTH == DEFAULT_SCREEN_WIDTH) ? (x) : (SCREEN_WIDTH - ScaleX(DEFAULT_SCREEN_WIDTH)) / 2 + ScaleX((x)));
    return f;
}

static float ScaleY(float y) {
    float f = ((y) * (float)SCREEN_HEIGHT / DEFAULT_SCREEN_HEIGHT);
    return f;
}

static float ScaleW(float w) {
    float f = ((w) * (float)SCREEN_WIDTH / DEFAULT_SCREEN_WIDTH) * DEFAULT_SCREEN_ASPECT_RATIO / SCREEN_ASPECT_RATIO;
    return f;
}

static float ScaleH(float h) {
    float f = ((h) * (float)SCREEN_HEIGHT / DEFAULT_SCREEN_HEIGHT);
    return f;
}

#define SCREEN_SCALE_X(x) (ScaleX(x))
#define SCREEN_SCALE_X_CENTER(x) (ScaleXKeepCentered(x))
#define SCREEN_SCALE_Y(y) (ScaleY(y))
#define SCREEN_SCALE_FROM_RIGHT(x) (SCREEN_WIDTH - SCREEN_SCALE_X(x))
#define SCREEN_SCALE_FROM_BOTTOM(y) (SCREEN_HEIGHT - SCREEN_SCALE_Y(y))

static void RotateVertices(CVector2D* rect, float x, float y, float angle) {
    float xold, yold;
    //angle /= 57.2957795;
    float _cos = cosf(angle);
    float _sin = sinf(angle);
    for (unsigned int i = 0; i < 4; i++) {
        xold = rect[i].x;
        yold = rect[i].y;
        rect[i].x = x + (xold - x) * _cos + (yold - y) * _sin;
        rect[i].y = y - (xold - x) * _sin + (yold - y) * _cos;
    }
}

static void DrawTriangle(float x, float y, float scale, float angle, CRGBA const& col) {
    CVector2D posn[4];
    float w = scale;
    float h = scale;

    posn[1].x = x - (w * 0.5f); posn[1].y = y - (h * 0.5f); posn[0].x = x + (w * 0.5f); posn[0].y = y - (h * 0.5f);
    posn[3].x = x; posn[3].y = y + (h * 0.5f);	posn[2].x = x; posn[2].y = y + (h * 0.5f);

    RotateVertices(posn, x, y, angle);
    CSprite2d::Draw2DPolygon(posn[0].x, posn[0].y, posn[1].x, posn[1].y, posn[2].x, posn[2].y, posn[3].x, posn[3].y, CRGBA(col));
}

static void DrawLevel(CVector2D pos, int type, float scale, CRGBA const& col) {
    switch (type) {
    case 1:
        DrawTriangle(pos.x, pos.y, scale * 1.5f, DEGTORAD(0.0f), CRGBA(0, 0, 0, 255));
        DrawTriangle(pos.x, pos.y, scale, DEGTORAD(0.0f), col);
        break;
    case 2:
        DrawTriangle(pos.x, pos.y, scale * 1.5f, DEGTORAD(180.0f), CRGBA(0, 0, 0, 255));
        DrawTriangle(pos.x, pos.y, scale, DEGTORAD(180.0f), col);
        break;
    default:
        CSprite2d::DrawRect(CRect(pos.x - (scale * 0.65f), pos.y - (scale * 0.65f), pos.x + (scale * 0.65f), pos.y + (scale * 0.65f)), CRGBA(0, 0, 0, 255));
        CSprite2d::DrawRect(CRect(pos.x - (scale * 0.5f), pos.y - (scale * 0.5f), pos.x + (scale * 0.5f), pos.y + (scale * 0.5f)), col);
        break;
    }
}
