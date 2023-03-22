#define RADAR_SIZE_X (256.0f)
#define RADAR_SIZE_Y (256.0f)

#define RADAR_NUM_TILES (8)
#define RADAR_TILE_SIZE (RADAR_SIZE_X / RADAR_NUM_TILES)

#define RADAR_MIN_RANGE (14.0f)// og (7.8f)  - > 120.0f
#define RADAR_MAX_RANGE (48.0f)// og (22.5f) - > 350.0f
#define RADAR_MIN_SPEED (0.3f)
#define RADAR_MAX_SPEED (0.9f)

#define RADAR_LEFT (8.0f)
#define RADAR_BOTTOM (8.0f)
#define RADAR_WIDTH (82.0f) 
#define RADAR_HEIGHT (82.0f)

#define RADAR_BLIPS_SIZE (7.0f)

#include "plugin.h"
#include "common.h"
#include "CMenuManager.h"
#include "CSprite2d.h"
#include "Utility.h"
#include "CText.h"
#include "CPlayerPed.h"
#include "CPed.h"
#include "CGame.h"
#include "CObject.h"
#include "CGlobal.h"
#include "CKeybrd.h"
#include "CHud.h"
#include "CFont.h"
#include "CTheScripts.h"

#include "tVideo.h"
#include "cDMAudio.h"
#include "GBH.h"

using namespace plugin;

static int states[D3DRENDERSTATE_RANGEFOGENABLE];

struct tHardCodedBlips {
    CVector pos;
    short sprite;
};

std::vector<tHardCodedBlips> hardCodedBlips;

enum eHudSprites {
    SPRITE_RADAR_CENTRE,
    SPRITE_RADAR_NORTH,
    SPRITE_RADAR_RECT,  
    SPRITE_RADAR_LOONIE,
    SPRITE_RADAR_YAKUZA,
    SPRITE_RADAR_ZAIBATSU,
    SPRITE_RADAR_REDNECK,
    SPRITE_RADAR_SCIENTISTS,
    SPRITE_RADAR_KRISHNA,
    SPRITE_RADAR_MAFIA,
    SPRITE_RADAR_PHONE,
    SPRITE_RADAR_SAVE,
    SPRITE_RADAR_SPRAY,
    NUM_HUD_SPRITES,
};

CSprite2d radarSprites[64];
CSprite2d hudSprites[NUM_HUD_SPRITES];

const char* hudSpritesFileNames[NUM_HUD_SPRITES] = {
    "data\\hud\\radar_centre.dds",
    "data\\hud\\radar_north.dds",
    "data\\hud\\radar_rect.dds",
    "data\\hud\\radar_loonie.dds",
    "data\\hud\\radar_yakuza.dds",
    "data\\hud\\radar_zaibatsu.dds",
    "data\\hud\\radar_redneck.dds",
    "data\\hud\\radar_scientists.dds",
    "data\\hud\\radar_krishna.dds",
    "data\\hud\\radar_mafia.dds",
    "data\\hud\\radar_phone.dds",
    "data\\hud\\radar_save.dds",
    "data\\hud\\radar_spray.dds",
};

static float cachedSin = 0.0f;
static float cachedCos = 1.0f;
static float radarRange = 1.0f;
static CVector2D radarOrigin;

class GTA2Radar {
public:
    static void DrawRadarMask() {
        CVector2D corners[4] = {
            CVector2D(1.0f, -1.0f),
            CVector2D(1.0f, 1.0f),
            CVector2D(-1.0f, 1.0f),
            CVector2D(-1.0, -1.0f)
        };

        RenderStateSet(D3DRENDERSTATE_TEXTUREHANDLE, (void*)NULL);
        RenderStateSet(D3DRENDERSTATE_FOGENABLE, (void*)FALSE);
        RenderStateSet(D3DRENDERSTATE_TEXTUREMAG, (void*)D3DFILTER_LINEAR);
        RenderStateSet(D3DRENDERSTATE_TEXTUREMIN, (void*)D3DFILTER_LINEAR);
        RenderStateSet(D3DRENDERSTATE_SHADEMODE, (void*)D3DSHADE_FLAT);
        RenderStateSet(D3DRENDERSTATE_ZENABLE, (void*)TRUE);
        RenderStateSet(D3DRENDERSTATE_ZWRITEENABLE, (void*)TRUE);
        RenderStateSet(D3DRENDERSTATE_VERTEXBLEND, (void*)TRUE);
        RenderStateSet(D3DRENDERSTATE_ALPHABLENDENABLE, (void*)TRUE);

        RenderStateSet(D3DRENDERSTATE_SRCBLEND, (void*)D3DBLEND_ZERO);
        RenderStateSet(D3DRENDERSTATE_DESTBLEND, (void*)D3DBLEND_ONE);

        CRect rect(0.0f, 0.0f, SCREEN_SCALE_X(RADAR_WIDTH / 2), SCREEN_SCALE_Y(RADAR_HEIGHT));
        rect.Translate(SCREEN_SCALE_X(RADAR_LEFT), SCREEN_SCALE_FROM_BOTTOM(RADAR_BOTTOM + RADAR_HEIGHT));

        rect.Grow(SCREEN_SCALE_X(4.0f), SCREEN_SCALE_X(4.0f), SCREEN_SCALE_Y(4.0f), SCREEN_SCALE_Y(4.0f));

        CRGBA col = { 255, 255, 255, 255 };
        CSprite2d::SetVertices(rect, col, col, col, col);
        RenderPrimitive(D3DPT_TRIANGLEFAN, CSprite2d::ms_aVertices, 4);
        CVector2D out[8];
        CVector2D in;

        for (int i = 0; i < 4; i++) {
            in.x = corners[i].x;
            in.y = corners[i].y;
            TransformRadarPointToScreenSpace(out[0], in);

            for (int j = 0; j < 7; j++) {
                in.x = corners[i].x * cos(j * (M_PI / 2.0f / 6.0f));
                in.y = corners[i].y * sin(j * (M_PI / 2.0f / 6.0f));
                TransformRadarPointToScreenSpace(out[j + 1], in);
            };

            CSprite2d::SetMaskVertices(8, (float*)out);
            RenderPrimitive(D3DPT_TRIANGLEFAN, CSprite2d::ms_aVertices, 8);
        }
    }

    static void DrawMap() {
        CPlayerPed* playa = GetGame()->FindPlayerPed(0);
        CCar* car = playa->m_pPed->m_pCurrentCar;

        radarRange = RADAR_MIN_RANGE + (playa->m_ViewCamera.m_vPosInterp.FromInt16().z * 2.0f);
        radarOrigin = playa->GetPed()->GetPosition2D().FromInt16();
        DrawRadarMap();
    }

    static void DrawRadarMap() {
        DrawRadarMask();

        int x = floor(radarOrigin.x / RADAR_TILE_SIZE);
        int y = ceil(radarOrigin.y / RADAR_TILE_SIZE);

        RenderStateSet(D3DRENDERSTATE_FOGENABLE, (void*)FALSE);
        RenderStateSet(D3DRENDERSTATE_SRCBLEND, (void*)D3DBLEND_SRCALPHA);
        RenderStateSet(D3DRENDERSTATE_DESTBLEND, (void*)D3DBLEND_INVSRCALPHA);
        RenderStateSet(D3DRENDERSTATE_TEXTUREMAG, (void*)D3DFILTER_LINEAR);
        RenderStateSet(D3DRENDERSTATE_TEXTUREMIN, (void*)D3DFILTER_LINEAR);
        RenderStateSet(D3DRENDERSTATE_SHADEMODE, (void*)D3DSHADE_FLAT);
        RenderStateSet(D3DRENDERSTATE_ZENABLE, (void*)TRUE);
        RenderStateSet(D3DRENDERSTATE_ZWRITEENABLE, (void*)FALSE);
        RenderStateSet(D3DRENDERSTATE_VERTEXBLEND, (void*)TRUE);
        RenderStateSet(D3DRENDERSTATE_ALPHABLENDENABLE, (void*)TRUE);
        RenderStateSet(D3DRENDERSTATE_TEXTUREADDRESS, (void*)D3DTADDRESS_CLAMP);
        RenderStateSet(D3DRENDERSTATE_TEXTUREPERSPECTIVE, (void*)FALSE);

        for (int i = x - 4; i < x + 4; i++) {
            for (int j = y - 4; j < y + 4; j++) {
                DrawRadarSection(i, j);
            }
        }
    }

    static void DrawRadarSection(int x, int y) {
        CVector2D worldPoly[8];
        CVector2D radarCorners[4];
        CVector2D radarPoly[8];
        CVector2D texCoords[8];
        CVector2D screenPoly[8];

        GetTextureCorners(x, y, worldPoly);
        ClipRadarTileCoords(x, y);

        int index = x + RADAR_NUM_TILES * y;
        LPDIRECT3DTEXTURE2 texture = radarSprites[index].m_pTexture;

        for (int i = 0; i < 4; i++)
            TransformRealWorldPointToRadarSpace(radarCorners[i], worldPoly[i]);

        int numVertices = ClipRadarPoly(radarPoly, radarCorners);

        if (numVertices < 3)
            return;

        for (int i = 0; i < numVertices; i++) {
            TransformRadarPointToRealWorldSpace(worldPoly[i], radarPoly[i]);
            TransformRealWorldToTexCoordSpace(texCoords[i], worldPoly[i], x, y);
            TransformRadarPointToScreenSpace(screenPoly[i], radarPoly[i]);
        }

        RenderStateSet(D3DRENDERSTATE_TEXTUREHANDLE, texture);
        CSprite2d::SetVertices(numVertices, (float*)screenPoly, (float*)texCoords, CRGBA(255, 255, 255, 255));
        RenderPrimitive(D3DPRIMITIVETYPE::D3DPT_TRIANGLEFAN, CSprite2d::ms_aVertices, numVertices);
    }

    static void GetTextureCorners(int x, int y, CVector2D* out) {
        // bottom left
        out[0].x = RADAR_TILE_SIZE * (x);
        out[0].y = RADAR_TILE_SIZE * (y + 1);

        // bottom right
        out[1].x = RADAR_TILE_SIZE * (x + 1);
        out[1].y = RADAR_TILE_SIZE * (y + 1);

        // top right
        out[2].x = RADAR_TILE_SIZE * (x + 1);
        out[2].y = RADAR_TILE_SIZE * (y);

        // top left
        out[3].x = RADAR_TILE_SIZE * (x);
        out[3].y = RADAR_TILE_SIZE * (y);
    }

    static void ClipRadarTileCoords(int& x, int& y) {
        if (x < 0)
            x = 0;
        if (x > RADAR_NUM_TILES - 1)
            x = RADAR_NUM_TILES - 1;
        if (y < 0)
            y = 0;
        if (y > RADAR_NUM_TILES - 1)
            y = RADAR_NUM_TILES - 1;
    }

    static int ClipPolyPlane(const CVector2D* in, int nin, CVector2D* out, CVector* plane) {
        int j;
        int nout;
        int x1, x2;
        float d1, d2, t;

        nout = 0;
        for (j = 0; j < nin; j++) {
            x1 = j;
            x2 = (j + 1) % nin;

            d1 = plane->x * in[x1].x + plane->y * in[x1].y + plane->z;
            d2 = plane->x * in[x2].x + plane->y * in[x2].y + plane->z;
            if (d1 * d2 < 0.0f) {
                t = d1 / (d1 - d2);
                out[nout++] = in[x1] * (1.0f - t) + in[x2] * t;
            }
            if (d2 >= 0.0f)
                out[nout++] = in[x2];
        }
        return nout;
    }

    static int ClipRadarPoly(CVector2D* poly, const CVector2D* rect) {
        CVector planes[4] = {
            CVector(-1.0f, 0.0f, 1.0f),
            CVector(1.0f, 0.0f, 1.0f),
            CVector(0.0f, -1.0f, 1.0f),
            CVector(0.0f,  1.0f, 1.0f)
        };
        CVector2D tmp[8];
        int n;
        if (n = ClipPolyPlane(rect, 4, tmp, &planes[0]), n == 0) return 0;
        if (n = ClipPolyPlane(tmp, n, poly, &planes[1]), n == 0) return 0;
        if (n = ClipPolyPlane(poly, n, tmp, &planes[2]), n == 0) return 0;
        if (n = ClipPolyPlane(tmp, n, poly, &planes[3]), n == 0) return 0;
        return n;
    }

    static void TransformRealWorldPointToRadarSpace(CVector2D& out, const CVector2D& in) {
        float x = (in.x - radarOrigin.x) * (1.0f / radarRange);
        float y = (in.y - radarOrigin.y) * (1.0f / radarRange);

        out.x = cachedSin * y + cachedCos * x;
        out.y = cachedCos * y - cachedSin * x;
    }

    static void TransformRadarPointToRealWorldSpace(CVector2D& out, const CVector2D& in) {
        float s = -cachedSin;
        float c = cachedCos;

        out.x = s * in.y + c * in.x;
        out.y = c * in.y - s * in.x;

        out = out * radarRange + radarOrigin;
    }

    static void TransformRealWorldToTexCoordSpace(CVector2D& out, const CVector2D& in, int x, int y) {
        out.x = in.x - (x * RADAR_TILE_SIZE);
        out.y = in.y - (y * RADAR_TILE_SIZE);
        out.x /= RADAR_TILE_SIZE;
        out.y /= RADAR_TILE_SIZE;
    }

    static void TransformRadarPointToScreenSpace(CVector2D& out, const CVector2D& in) {
        out.x = (in.x + 1.0f) * 0.5f * SCREEN_SCALE_X(RADAR_WIDTH) + SCREEN_SCALE_X(RADAR_LEFT);
        out.y = (in.y + 1.0f) * 0.5f * SCREEN_SCALE_Y(RADAR_HEIGHT) + SCREEN_SCALE_FROM_BOTTOM(RADAR_BOTTOM + RADAR_HEIGHT);
    }

    static float LimitRadarPoint(CVector2D& pos) {
        float dist = pos.Magnitude();
        pos.x = CLAMP(pos.x, -1.0f, 1.0f);
        pos.y = CLAMP(pos.y, -1.0f, 1.0f);
        return dist;
    }

    static void DrawRotatingRadarSprite(CSprite2d* sprite, float x, float y, float angle, int alpha) {
        CVector curPosn[4];
        const float correctedAngle = angle - M_PI / 4.f;
        float scale = SCREEN_SCALE_Y(RADAR_BLIPS_SIZE);

        for (unsigned int i = 0; i < 4; i++) {
            const float cornerAngle = i * (M_PI / 2) + correctedAngle;
            curPosn[i].x = x + (0.0f * cos(cornerAngle) + 1.0f * sin(cornerAngle)) * scale;
            curPosn[i].y = y - (0.0f * sin(cornerAngle) - 1.0f * cos(cornerAngle)) * scale;
        }

        sprite->Draw(curPosn[3].x, curPosn[3].y, curPosn[2].x, curPosn[2].y, curPosn[0].x, curPosn[0].y, curPosn[1].x, curPosn[1].y, CRGBA(255, 255, 255, alpha));
    }

    static void DrawBlip(CSprite2d* sprite, CVector2D out, CRGBA const& col) {
        float scale = SCREEN_SCALE_Y(RADAR_BLIPS_SIZE);
        sprite->Draw(CRect(out.x - scale, out.y - scale, out.x + scale, out.y + scale), col);
    }

    static unsigned char CalculateBlipAlpha(float dist) {
        if (dist <= 1.0f)
            return 255;

        if (dist <= 5.0f)
            return (128.0f * ((dist - 1.0f) / 4.0f)) + ((1.0f - (dist - 1.0f) / 4.0f) * 255.0f);

        return 128;
    }

    static void DrawRadarNorth() {
        CVector2D out;
        CVector2D in;
        out.x = radarOrigin.x;
        out.y = -M_SQRT2 * radarRange + radarOrigin.y;
        TransformRealWorldPointToRadarSpace(in, out);
        LimitRadarPoint(in);
        TransformRadarPointToScreenSpace(out, in);

        float scale = SCREEN_SCALE_X(RADAR_BLIPS_SIZE);
        hudSprites[SPRITE_RADAR_NORTH].Draw(CRect(out.x - scale, out.y - scale, out.x + scale, out.y + scale), CRGBA(255, 255, 255, 255));
    }

    static void DrawRadarCentre() {
        CVector2D out;
        CVector2D in = CVector2D(0.0f, 0.0f);
        TransformRadarPointToScreenSpace(out, in);

        CPlayerPed* playa = GetGame()->FindPlayerPed(0);
        CCar* car = playa->m_pPed->m_pCurrentCar;

        float angle = 0.0f;
        if (car)
            angle = DEGTORAD(car->m_pSprite->m_nRotation / 4.0f);
        else
            angle = DEGTORAD(playa->m_pPed->m_pObject->m_pSprite->m_nRotation / 4.0f);

        DrawRotatingRadarSprite(&hudSprites[SPRITE_RADAR_CENTRE], out.x, out.y, angle, 255);
    }

    static void DrawBlips() {
        RenderStateSet(D3DRENDERSTATE_ZWRITEENABLE, (void*)FALSE);
        RenderStateSet(D3DRENDERSTATE_ZENABLE, (void*)FALSE);
        RenderStateSet(D3DRENDERSTATE_VERTEXBLEND, (void*)TRUE);
        RenderStateSet(D3DRENDERSTATE_SRCBLEND, (void*)D3DBLEND_SRCALPHA);
        RenderStateSet(D3DRENDERSTATE_DESTBLEND, (void*)D3DBLEND_INVSRCALPHA);
        RenderStateSet(D3DRENDERSTATE_FOGENABLE, (void*)FALSE);

        DrawRadarNorth();

        // Hardcoded blips
        for (auto& it : hardCodedBlips) {
            CVector2D out = {};
            CVector2D in = {};
            TransformRealWorldPointToRadarSpace(in, { it.pos.x, it.pos.y });
            float dist = LimitRadarPoint(in);
            TransformRadarPointToScreenSpace(out, in);

            if (dist > 1.1f)
                continue;

            unsigned char a = CalculateBlipAlpha(dist);
            DrawBlip(&hudSprites[it.sprite], out, CRGBA(255, 255, 255, a));
        }

        CPlayerPed* playa = GetGame()->FindPlayerPed(0);
        for (int i = 0; i < MAX_HUD_ARROWS; i++) {
            CHudArrow* arrow = &GetHud()->m_HudArrows[i];

            if (!arrow->AreBothArrowTracesUsed() /* && arrow->IsArrowVisible()*/) {
                int sprite = arrow->m_nSpriteId;
                int type = arrow->m_nType;

                CRGBA col = { 255, 0, 255, 255 };
                CVector pos = arrow->m_ArrowTrace.m_vPos.FromInt16();

                CVector2D out = {};
                CVector2D in = {};
                TransformRealWorldPointToRadarSpace(in, { pos.x, pos.y });
                float dist = LimitRadarPoint(in);
                TransformRadarPointToScreenSpace(out, in);

                int level = 0;
                float diff = playa->GetPed()->GetPosition().FromInt16().z - pos.z;

                if (diff > 0.1f)
                    level = 1;
                else if (diff < -0.1f)
                    level = 2;
                else
                    level = 0;

                const bool onAMission = !GetTheScripts()->OnAMissionFlag || *GetTheScripts()->OnAMissionFlag;

                switch (sprite) {
                case SPRITE_BIGARROW:
                    DrawLevel(out, level, SCREEN_SCALE_X(RADAR_BLIPS_SIZE), col);
                    break;
                case SPRITE_GREENARROW:
                case SPRITE_BLUEARROW:
                case SPRITE_GREYARROW:
                case SPRITE_BLUELIGHT:
                case SPRITE_YELLOW:
                case SPRITE_ORANGE:
                case SPRITE_RED:
                    if (!onAMission) {
                        unsigned char a = CalculateBlipAlpha(dist);
                        switch (type) {
                        case TYPE_GREEN:
                            if (dist <= 2.0f || arrow->IsArrowVisible())
                                DrawBlip(&hudSprites[SPRITE_RADAR_PHONE], out, CRGBA(0, 190, 0, a));
                            break;
                        case TYPE_RED:
                            if (dist <= 2.0f || arrow->IsArrowVisible())
                                DrawBlip(&hudSprites[SPRITE_RADAR_PHONE], out, CRGBA(190, 0, 0, a));
                            break;
                        case TYPE_YELLOW:
                            if (dist <= 2.0f || arrow->IsArrowVisible())
                                DrawBlip(&hudSprites[SPRITE_RADAR_PHONE], out, CRGBA(190, 180, 0, a));
                            break;
                        default:
                            DrawBlip(&hudSprites[SPRITE_RADAR_LOONIE + sprite - 1], out, CRGBA(255, 255, 255, a));
                            break;
                        }
                    }
                    break;
                case SPRITE_SMALLYELLOW:
                case SPRITE_SMALLGREEN:
                case SPRITE_SMALLRED:
                    break;
                }
            }
        }

        DrawRadarCentre();
    }

    static void GetStates() {
        for (int i = 0; i < D3DRENDERSTATE_RANGEFOGENABLE; i++)
            RenderStateGet((D3DRENDERSTATETYPE)i, (void*)&states[i]);
    }

    static void RestoreStates() {
        for (int i = 0; i < D3DRENDERSTATE_RANGEFOGENABLE; i++)
            RenderStateSet((D3DRENDERSTATETYPE)i, (void*)states[i]);
    }

    GTA2Radar() {
        ThiscallEvent <AddressList<0x462028, H_CALL>, PRIORITY_AFTER, ArgPickNone, void(CGame*)> onGameInit;
        onGameInit += []() {
            for (int i = 0; i < NUM_HUD_SPRITES; i++) {
                hudSprites[i].SetTexture(hudSpritesFileNames[i]);
            }

            char mapName[32] = {};
            const char* s = gGlobal.mapName;
            s += 5;
            
            int i = 0;
            while (*s) {
                if (*s == '.') {
                    mapName[i] = '\0';
                    break;
                }
            
                mapName[i] = *s;
                i++;
                s++;
            }

            // Hardcoded blips
            hardCodedBlips.clear();
            hardCodedBlips.shrink_to_fit();
            if (!strcmp(mapName, "wil")) {
                hardCodedBlips.push_back({ { 159.00, 137.00, 2.00 }, SPRITE_RADAR_SAVE }); // Save
                hardCodedBlips.push_back({ { 204.50, 221.50, 2.00 }, SPRITE_RADAR_SPRAY });
                hardCodedBlips.push_back({ { 219.50, 34.50, 2.00 }, SPRITE_RADAR_SPRAY });
                hardCodedBlips.push_back({ { 24.50, 60.50, 3.00 },  SPRITE_RADAR_SPRAY });
                hardCodedBlips.push_back({ { 46.50, 136.50, 2.00 }, SPRITE_RADAR_SPRAY });
                hardCodedBlips.push_back({ { 86.50, 160.50, 2.00 }, SPRITE_RADAR_SPRAY });
                hardCodedBlips.push_back({ { 209.50, 121.50, 2.00 }, SPRITE_RADAR_SPRAY });
            }
            else if (!strcmp(mapName, "ste")) {
                hardCodedBlips.push_back({ { 113.00, 123.00, 2.00 }, SPRITE_RADAR_SAVE }); // Save
                hardCodedBlips.push_back({ { 6.50, 183.50, 2.00 }, SPRITE_RADAR_SPRAY });
                hardCodedBlips.push_back({ { 178.50, 216.50, 2.00 }, SPRITE_RADAR_SPRAY });
                hardCodedBlips.push_back({ { 215.50, 76.50, 2.00 }, SPRITE_RADAR_SPRAY });
                hardCodedBlips.push_back({ { 94.50, 27.50, 2.00 }, SPRITE_RADAR_SPRAY });
            }
            else if (!strcmp(mapName, "bil")) {
                hardCodedBlips.push_back({ { 44.50, 102.00, 2.00 }, SPRITE_RADAR_SAVE }); // Save
                hardCodedBlips.push_back({ { 177.50, 14.50, 2.00 }, SPRITE_RADAR_SPRAY });
                hardCodedBlips.push_back({ { 234.50, 154.50, 2.00 }, SPRITE_RADAR_SPRAY });
                hardCodedBlips.push_back({ { 30.50, 216.50, 2.00 }, SPRITE_RADAR_SPRAY });
                hardCodedBlips.push_back({ {64.50, 90.50, 2.00}, SPRITE_RADAR_SPRAY });
            }

            for (int i = 0; i < 64; i++) {
                char buff[MAX_PATH];
                sprintf(buff, "data\\radar\\%s\\%s_%02d.dds", mapName, mapName, i + 1);
                radarSprites[i].SetTexture(buff);
            }
        };

        plugin::Events::shutdownEngineEvent += []() {
            for (int i = 0; i < 64; i++) {
                radarSprites[i].Delete();
            }
            
            for (int i = 0; i < NUM_HUD_SPRITES; i++) {
                hudSprites[i].Delete();
            }
        };

        plugin::Events::d3dResetEvent += []() {
            for (int i = 0; i < 64; i++) {
                radarSprites[i].Reset();
            }
        
            for (int i = 0; i < NUM_HUD_SPRITES; i++) {
                hudSprites[i].Reset();
            }
        };

        plugin::Events::drawHudEvent += []() {
            GetStates();
            DrawMap();    
            float x = SCREEN_SCALE_X(RADAR_LEFT);
            float y = SCREEN_SCALE_FROM_BOTTOM(RADAR_BOTTOM + RADAR_HEIGHT);
            CRect rect(x, y, SCREEN_SCALE_X(RADAR_WIDTH) + x, SCREEN_SCALE_Y(RADAR_HEIGHT) + y);
            rect.left -= SCREEN_SCALE_X(2.0f);
            rect.top -= SCREEN_SCALE_Y(2.0f);
            rect.right += SCREEN_SCALE_X(2.0f);
            rect.bottom += SCREEN_SCALE_Y(2.0f);
            
            hudSprites[SPRITE_RADAR_RECT].Draw(rect, CRGBA(255, 255, 255, 255));
            DrawBlips();
            RestoreStates();
        };     

        // No arrows
        plugin::patch::Nop(0x4CA4D2, 11);
    };
} gta2Radar;
