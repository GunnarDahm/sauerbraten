// gamepad.cpp: game controller support (USB/Bluetooth) via the SDL2 GameController API
//
// Controller buttons show up as bindable keys named PAD_* (see data/keymap.cfg and
// data/gamepad.cfg for the defaults). The left stick moves, the right stick looks around,
// and while a menu is open both sticks steer the menu cursor, A clicks and B goes back.

#include "engine.h"

VARP(gamepad, 0, 1, 1);
VARP(paddeadzone, 0, 20, 90);          // percent of stick travel ignored around the center
VARP(padtriggerthreshold, 1, 30, 100); // percent of trigger travel needed to count as pressed
VARP(padyawspeed, 1, 200, 1000);       // degrees per second at full right-stick deflection
VARP(padpitchspeed, 1, 140, 1000);
FVARP(padlookcurve, 1, 2, 4);          // response curve exponent, higher = finer aim near the center
VARP(padinvert, 0, 0, 1);
VARP(padcursorspeed, 1, 80, 1000);     // menu cursor speed
VARP(padmenubuttons, 0, 1, 1);         // A/B act as click/back while a menu is open

enum
{
    PADKEY_BUTTONS = -100,                  // PAD_A = -100 ... one code per SDL_GameControllerButton
    PADKEY_LT = PADKEY_BUTTONS - 30,
    PADKEY_RT = PADKEY_BUTTONS - 31
};

struct gamepadstate
{
    SDL_GameController *controller;
    SDL_JoystickID id;
    bool lt, rt;
    int sentcode[SDL_CONTROLLER_BUTTON_MAX];
};

static vector<gamepadstate> pads;
static bool padmoving = false;
static float padcursorx = 0, padcursory = 0;

static gamepadstate *findpad(SDL_JoystickID id)
{
    loopv(pads) if(pads[i].id == id) return &pads[i];
    return NULL;
}

static void openpad(int index)
{
    if(!SDL_IsGameController(index))
    {
        conoutf(CON_WARN, "joystick \"%s\" has no controller mapping, add one to data/gamecontrollerdb.txt", SDL_JoystickNameForIndex(index));
        return;
    }
    SDL_GameController *c = SDL_GameControllerOpen(index);
    if(!c) { conoutf(CON_ERROR, "could not open controller: %s", SDL_GetError()); return; }
    SDL_JoystickID id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(c));
    if(findpad(id)) { SDL_GameControllerClose(c); return; }
    gamepadstate &p = pads.add();
    p.controller = c;
    p.id = id;
    p.lt = p.rt = false;
    loopi(SDL_CONTROLLER_BUTTON_MAX) p.sentcode[i] = 0;
    conoutf("controller connected: %s", SDL_GameControllerName(c));
}

static void closepad(SDL_JoystickID id)
{
    loopv(pads) if(pads[i].id == id)
    {
        gamepadstate &p = pads[i];
        // release anything still held so binds don't get stuck
        loopj(SDL_CONTROLLER_BUTTON_MAX) if(p.sentcode[j]) keypress(p.sentcode[j], false);
        if(p.lt) keypress(PADKEY_LT, false);
        if(p.rt) keypress(PADKEY_RT, false);
        conoutf("controller disconnected: %s", SDL_GameControllerName(p.controller));
        SDL_GameControllerClose(p.controller);
        pads.remove(i);
        break;
    }
}

void initgamepads()
{
    const char *db = findfile("data/gamecontrollerdb.txt", "rb");
    if(fileexists(db, "r")) SDL_GameControllerAddMappingsFromFile(db);
    SDL_GameControllerEventState(SDL_ENABLE);
    // devices already plugged in are also reported through SDL_CONTROLLERDEVICEADDED
}

void cleanupgamepads()
{
    loopv(pads) SDL_GameControllerClose(pads[i].controller);
    pads.setsize(0);
}

static bool menuactive()
{
    return g3d_movecursor(0, 0);
}

static void padbutton(gamepadstate &p, int button, bool down)
{
    if(button < 0 || button >= SDL_CONTROLLER_BUTTON_MAX) return;
    if(down)
    {
        if(p.sentcode[button]) return;
        int code = PADKEY_BUTTONS - button;
        if(padmenubuttons && menuactive()) switch(button)
        {
            case SDL_CONTROLLER_BUTTON_A: code = -1; break;           // left click
            case SDL_CONTROLLER_BUTTON_B: code = SDLK_ESCAPE; break;  // back out of the menu
        }
        p.sentcode[button] = code;
        keypress(code, true);
    }
    else
    {
        int code = p.sentcode[button];
        if(!code) return;
        p.sentcode[button] = 0;
        keypress(code, false);
    }
}

static void padtrigger(gamepadstate &p, int axis, int value)
{
    bool down = value >= padtriggerthreshold*32767/100;
    bool &state = axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT ? p.lt : p.rt;
    if(down == state) return;
    state = down;
    keypress(axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT ? PADKEY_LT : PADKEY_RT, down);
}

bool gamepadevent(const SDL_Event &e)
{
    switch(e.type)
    {
        case SDL_CONTROLLERDEVICEADDED:
            openpad(e.cdevice.which);
            return true;
        case SDL_CONTROLLERDEVICEREMOVED:
            closepad(e.cdevice.which);
            return true;
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
        {
            gamepadstate *p = findpad(e.cbutton.which);
            if(p && gamepad) padbutton(*p, e.cbutton.button, e.cbutton.state == SDL_PRESSED);
            return true;
        }
        case SDL_CONTROLLERAXISMOTION:
        {
            gamepadstate *p = findpad(e.caxis.which);
            if(p && gamepad && (e.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT || e.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT))
                padtrigger(*p, e.caxis.axis, e.caxis.value);
            return true;
        }
    }
    return false;
}

// returns the stick position in [-1, 1] with the dead zone removed
static void readstick(SDL_GameController *c, SDL_GameControllerAxis ax, SDL_GameControllerAxis ay, float &x, float &y)
{
    x = SDL_GameControllerGetAxis(c, ax)/32767.0f;
    y = SDL_GameControllerGetAxis(c, ay)/32767.0f;
    float mag = sqrtf(x*x + y*y), dead = paddeadzone/100.0f;
    if(mag <= dead) { x = y = 0; return; }
    float scale = min((mag - dead)/(1 - dead), 1.0f)/mag;
    x *= scale;
    y *= scale;
}

static void movepadcursor(float x, float y)
{
    float speed = padcursorspeed*min(elapsedtime, 100)/1000.0f;
    padcursorx += x*speed*10;
    padcursory += y*speed*10;
    int dx = int(padcursorx), dy = int(padcursory);
    padcursorx -= dx;
    padcursory -= dy;
    if(dx || dy) g3d_movecursor(dx, dy);
}

void updategamepads()
{
    if(!gamepad || pads.empty())
    {
        if(padmoving) { player->move = player->strafe = 0; padmoving = false; }
        return;
    }
    float lx = 0, ly = 0, rx = 0, ry = 0;
    loopv(pads)
    {
        float x, y;
        readstick(pads[i].controller, SDL_CONTROLLER_AXIS_LEFTX, SDL_CONTROLLER_AXIS_LEFTY, x, y);
        if(fabs(x) > fabs(lx)) lx = x;
        if(fabs(y) > fabs(ly)) ly = y;
        readstick(pads[i].controller, SDL_CONTROLLER_AXIS_RIGHTX, SDL_CONTROLLER_AXIS_RIGHTY, x, y);
        if(fabs(x) > fabs(rx)) rx = x;
        if(fabs(y) > fabs(ry)) ry = y;
    }

    if(menuactive())
    {
        if(padmoving) { player->move = player->strafe = 0; padmoving = false; }
        movepadcursor(fabs(lx) > fabs(rx) ? lx : rx, fabs(ly) > fabs(ry) ? ly : ry);
        return;
    }

    // left stick: movement (the engine only knows full speed, so it's digital past half deflection)
    const float movethreshold = 0.5f;
    int move = ly < -movethreshold ? 1 : (ly > movethreshold ? -1 : 0),
        strafe = lx < -movethreshold ? 1 : (lx > movethreshold ? -1 : 0);
    if(move || strafe)
    {
        player->move = move;
        player->strafe = strafe;
        padmoving = true;
    }
    else if(padmoving)
    {
        player->move = player->k_up ? 1 : (player->k_down ? -1 : 0);
        player->strafe = player->k_left ? 1 : (player->k_right ? -1 : 0);
        padmoving = false;
    }

    // right stick: looking around
    if((rx || ry) && curtime > 0 && game::allowmouselook())
    {
        float secs = curtime/1000.0f,
              cx = (rx < 0 ? -1 : 1)*powf(fabs(rx), padlookcurve),
              cy = (ry < 0 ? -1 : 1)*powf(fabs(ry), padlookcurve);
        extern int zoom, fov, zoomfov;
        float zoomscale = zoom ? float(zoomfov)/fov : 1.0f;
        extern void lookdelta(float dyaw, float dpitch);
        lookdelta(cx*padyawspeed*secs*zoomscale, -cy*padpitchspeed*secs*zoomscale*(padinvert ? -1 : 1));
    }
}

ICOMMAND(numgamepads, "", (), intret(pads.length()));
ICOMMAND(gamepadname, "i", (int *i), result(pads.inrange(*i) ? SDL_GameControllerName(pads[*i].controller) : ""));
