#include "racer_game.h"

global_variable game_input GlobalInput;
global_variable game_input GlobalOldInput;


inline v2
ConvertTopLeftToCenter(v2 TopLeft, v2 Dim)
{
    v2 Result = {};
    Result = TopLeft + (0.5 * Dim);
    return Result;
}

inline v2
ConvertCenterToTopLeft(v2 Center, v2 Dim)
{
    v2 Result = {};
    Result = Center - (0.5 * Dim);
    return Result;
}


inline game_input 
GetInputDiff(game_input *NewInput, game_input *StaleInput)
{
    game_input Result = *NewInput;
    
    if(NewInput->IsUpPressed && StaleInput->IsUpPressed)
    {
        Result.IsUpPressed = false;
    }
    if(NewInput->IsLeftPressed && StaleInput->IsLeftPressed)
    {
        Result.IsLeftPressed = false;
    }
    if(NewInput->IsDownPressed && StaleInput->IsDownPressed)
    {
        Result.IsDownPressed = false;
    }
    if(NewInput->IsRightPressed && StaleInput->IsRightPressed)
    {
        Result.IsRightPressed = false;
    }
    
    return Result;
}

internal void
MoveCharacter(game_character* Character, 
              real32 deltaTimeSec,
              window_width_height BoundingDim,
              game_input *NewInput,
              game_input *StaleInput)
{
    real32 SpeedModifierX = 0.0f;
    real32 SpeedModifierY = 0.0f;
    
    int32 EffectRandomizer = false;
    int32 EffectMaxPercentX = 30;
    int32 EffectMaxPercentY = 50;
    
    real32 ForceConst = 250.0f;
    real32 VelConst = 1.0f;
    real32 AccelConst = 0.34f;
    real32 DiagonalConst = 0.0f;
    
    // Coeff values range from  0.0f to 1.0f
    real32 FrictionCoeff = 0.33f;
    real32 DragCoeff = 0.15f;
    
    local_persist int32 RepeatCount = 1;
    int32 AccelMultiplier = 1;
    real32 AccelGrowthFactor = 1.5f;
    
    Character->Accel = V2(0, 0);
    Character->Color = 0x00ff00ff;
    
    game_input DiffInput = GetInputDiff(NewInput, StaleInput);
    game_input Input = *NewInput;
    
    if(!(DiffInput.IsUpPressed || DiffInput.IsDownPressed ||
         DiffInput.IsLeftPressed || DiffInput.IsRightPressed))
    {
        RepeatCount++;
    }
    else
    {
        RepeatCount = 1;
    }
    AccelMultiplier *= RepeatCount * AccelGrowthFactor;
    //AccelMultiplier *= Square(RepeatCount);
    
    int32 InputCount = 0;
    if(Input.IsUpPressed)
    {
        Character->Accel.Y -= ForceConst;
        Character->Color = 0x00ffff;
        InputCount++;
    }
    if(Input.IsDownPressed)
    {
        Character->Accel.Y += ForceConst;
        Character->Color = 0x00ffff;
        InputCount++;
    }
    if(Input.IsLeftPressed)
    {
        Character->Accel.X -= ForceConst;
        Character->Color = 0x00ffff;
        InputCount++;
    }
    if(Input.IsRightPressed)
    {
        Character->Accel.X += ForceConst;
        Character->Color = 0x00ffff;
        InputCount++;
    }
    
    real32 MaxSpeedLocal = Character->MaxSpeed;
    if (InputCount == 2)
    {
        Character->Accel *= DiagonalConst;
        MaxSpeedLocal *= DiagonalConst;
        if(MaxSpeedLocal == 0)
        {
            MaxSpeedLocal = 1;
        }
        DragCoeff *= DiagonalConst;
        //AccelMultiplier *= DiagonalConst;
    }
    
    if(EffectRandomizer)
    {
        real32 Effect = (rand() % EffectMaxPercentX + 1) / 100;
        Character->Accel.X *= 1 -Effect;
        Effect = (rand() % EffectMaxPercentY + 1) / 100;
        Character->Accel.Y *=  1 - Effect;
    }
    
    Character->Vel += Character->Accel * deltaTimeSec * AccelMultiplier;
    real32 SpeedRatioX = (abs((int)(Character->Vel.X * 1000)) / 1000) / MaxSpeedLocal;
    Character->Vel.X *=  1 - ((FrictionCoeff / 2) + ((DragCoeff / 2) * Square(SpeedRatioX))); 
    real32 SpeedRatioY = (abs((int)(Character->Vel.Y * 1000)) / 1000) / MaxSpeedLocal;
    Character->Vel.Y *=  1 - ((FrictionCoeff / 2) + ((DragCoeff / 2) * Square(SpeedRatioY))); 
    
    if(Length(Character->Vel) > Character->MaxSpeed)
    {
        Character->Vel *= Character->MaxSpeed / Length(Character->Vel);
    }
    
    Character->Center += (Character->Accel * deltaTimeSec * deltaTimeSec * AccelConst)
        + (Character->Vel * deltaTimeSec * VelConst);
    
    Character->Center = V2(RoundToCeil(Character->Center.X), 
                           RoundToCeil(Character->Center.Y));
    v2 TopLeftPos = ConvertCenterToTopLeft(Character->Center, Character->Dim);
    if(TopLeftPos.X < 0)
    {
        TopLeftPos.X = 0;
    }
    else if(TopLeftPos.X > BoundingDim.Width - Character->Dim.X)
    {
        TopLeftPos.X = BoundingDim.Width - Character->Dim.X;
    }
    if(TopLeftPos.Y < 0)
    {
        TopLeftPos.Y = 0;
    }
    else if(TopLeftPos.Y > BoundingDim.Height - Character->Dim.Y)
    {
        TopLeftPos.Y = BoundingDim.Height - Character->Dim.Y;
    }
    Character->Center = ConvertTopLeftToCenter(TopLeftPos, Character->Dim);
}
