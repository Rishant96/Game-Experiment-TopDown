/* date = May 7th 2022 5:22 am */

#ifndef RACER_GAME_H
#define RACER_GAME_H


struct window_width_height
{
    int32 Width;
    int32 Height;
};

struct game_input
{
    bool32 IsUpPressed;
    bool32 IsDownPressed;
    bool32 IsLeftPressed;
    bool32 IsRightPressed;
};

enum entity_type
{
    Player,
    Explosive
};

struct game_character
{
    v2 Center;
    v2 Dim;
    entity_type Type;
    v2 Accel;
    v2 Vel;
    real32 MaxSpeed;
    uint32 Color;
    uint32 ExplosionTimer;
};

#endif //RACER_GAME_H
