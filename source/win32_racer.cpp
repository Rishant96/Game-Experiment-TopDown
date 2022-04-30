#include "windows.h"
#include <stdint.h>

#define internal static 
#define local_persist static 
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef float real32;
typedef double real64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#include "racer_math.h"

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
    real32 Accel;
    real32 Vel;
    uint32 Color;
    uint32 ExplosionTimer;
};

struct game_buffer
{
    int32 OffsetX;
    int32 OffsetY;
    int32 Width;
    int32 Height;
    void *DataPtr;
    BITMAPINFO Info;
};

struct window_width_height
{
    int32 Width;
    int32 Height;
};

global_variable bool32 GlobalRunning;
global_variable game_buffer GlobalBuffer;
global_variable game_buffer KeysBuffer;
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

internal window_width_height
Win32GetWindowDimension(HWND Window)
{
    window_width_height Result;
    
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;
    
    return(Result);
}

internal void
Win32ResizeOrCreateDisplayBuffer(int32 Width2Create, int32 Height2Create, game_buffer *Buffer=&GlobalBuffer)
{
    if(Buffer->DataPtr)
    {
        VirtualFree(Buffer->DataPtr, 0, MEM_RELEASE);
    }
    Buffer->Width = Width2Create;
    Buffer->Height = Height2Create;
    
    int32 BytesPerPixel = 4;
    
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;
    
    int32 BitmapMemorySize = (Buffer->Width*Buffer->Height)*BytesPerPixel;
    Buffer->DataPtr = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

inline uint32
Win32GetColorFromRGB(uint8 Red, uint8 Green, uint8 Blue)
{
    uint32 Result = (uint32)((Red << 16) | (Green << 8) | (Blue << 0));
    return Result;
}

internal void
Win32ClearDisplayBufferToStaticColors(uint32 Color = 0x00ffffff,
                                      game_buffer *Buffer = &GlobalBuffer)
{
    for(int32 i = 0; 
        i < Buffer->Height;
        ++i)
    {
        uint32 *RowPtr = (uint32 *)Buffer->DataPtr + (i * Buffer->Width);
        for(int32 j = 0;
            j < Buffer->Width;
            ++j)
        {
            uint32 *Pixel = RowPtr + j;
            *Pixel = Color;
        }
    }
}

internal void
Win32DrawRectangle(int32 RectPosX, int32 RectPosY,
                   int32 RectWidth, int32 RectHeight,
                   uint32 Color,
                   game_buffer *Buffer = &GlobalBuffer)
{
    if(RectPosX < 0)
    {
        RectWidth += RectPosX;
        RectPosX = 0;
    }
    else if(RectPosX > Buffer->Width)
    {
        return;
    }
    if(RectPosY < 0)
    {
        RectHeight += RectPosY;
        RectPosY = 0;
    }
    else if(RectPosY > Buffer->Height)
    {
        return;
    }
    if(RectPosX + RectWidth > Buffer->Width)
    {
        RectWidth = Buffer->Width - RectPosX;
    }
    if(RectPosY + RectHeight > Buffer->Height)
    {
        RectHeight = Buffer->Height - RectPosY;
    }
    
    for(int32 i = 0; 
        i < RectHeight;
        ++i)
    {
        uint32 *RowPtr = (uint32 *)Buffer->DataPtr 
            + ((RectPosY + i) * Buffer->Width)
            + RectPosX;
        for(int32 j = 0;
            j < RectWidth;
            ++j)
        {
            uint32 *Pixel = RowPtr + j;
            *Pixel = Color;
        }
    }
}


internal void
Win32DrawRectangle(v2 TopLeft, v2 Dim,
                   uint32 Color,
                   game_buffer *Buffer = &GlobalBuffer)
{
    Win32DrawRectangle(TopLeft.X, TopLeft.Y,
                       Dim.X, Dim.Y, Color, Buffer);
}

internal int32
Win32DisplayCurrentBuffersToScreen(HDC DeviceContext,
                                   int32 WindowWidth, int32 WindowHeight,
                                   game_buffer *Buffer = &GlobalBuffer)
{
    PatBlt(DeviceContext, 0, 0, WindowWidth, Buffer->OffsetX, BLACKNESS);
    PatBlt(DeviceContext, 0, Buffer->OffsetY, Buffer->OffsetX, Buffer->Height, BLACKNESS);
    PatBlt(DeviceContext, 
           Buffer->Width + Buffer->OffsetX, Buffer->OffsetY, 
           WindowWidth - Buffer->Width, Buffer->Height -KeysBuffer.Height, 
           BLACKNESS);
    PatBlt(DeviceContext, 
           Buffer->Width + Buffer->OffsetX + KeysBuffer.Width, 
           Buffer->OffsetY + Buffer->Height - KeysBuffer.Height, 
           WindowWidth - Buffer->Width - Buffer->OffsetX - KeysBuffer.Width, 
           KeysBuffer.Height, 
           BLACKNESS);
    PatBlt(DeviceContext, 
           0, Buffer->OffsetY + Buffer->Height, 
           WindowWidth, WindowHeight - Buffer->Height - Buffer->OffsetY, 
           BLACKNESS);
    int32 Result = StretchDIBits(DeviceContext,
                                 /*
                                 X, Y, Width, Height,
                                 X, Y, Width, Height,
                                 0, 0, WindowWidth, WindowHeight,
                                 */
                                 Buffer->OffsetX, Buffer->OffsetY, 
                                 Buffer->Width, Buffer->Height,
                                 0, 0, Buffer->Width, Buffer->Height,
                                 Buffer->DataPtr,
                                 &Buffer->Info,
                                 DIB_RGB_COLORS, SRCCOPY);
    int32 Result2 = StretchDIBits(DeviceContext,
                                  Buffer->OffsetX + Buffer->Width, 
                                  Buffer->OffsetY + Buffer->Height - KeysBuffer.Height,
                                  KeysBuffer.Width, KeysBuffer.Height,
                                  0, 0, KeysBuffer.Width, KeysBuffer.Height,
                                  KeysBuffer.DataPtr,
                                  &KeysBuffer.Info,
                                  DIB_RGB_COLORS, SRCCOPY);
    return Result;
}

internal void
Win32ProcessSystemInput(MSG Message)
{
    switch(Message.message)
    {
        case WM_QUIT:
        {
            GlobalRunning = false;
        } break;
        
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            uint32 VKCode = (uint32)Message.wParam;
            if(VKCode == VK_UP)
            {
                GlobalInput.IsUpPressed = true;
            }
            else if(VKCode == VK_LEFT)
            {
                GlobalInput.IsLeftPressed = true;
            }
            else if(VKCode == VK_DOWN)
            {
                GlobalInput.IsDownPressed = true;
            }
            else if(VKCode == VK_RIGHT)
            {
                GlobalInput.IsRightPressed = true;
            }
            else if(VKCode == VK_ESCAPE)
            {
                
            }
            else if(VKCode == VK_SPACE)
            {
                
            }
            
            bool32 AltKeyWasDown = (Message.lParam & (1 << 29));
            if((VKCode == VK_F4) && AltKeyWasDown)
            {
                GlobalRunning = false;
            }
        } break;
        
        case WM_SYSKEYUP:
        case WM_KEYUP:
        {
            uint32 VKCode = (uint32)Message.wParam;
            if(VKCode == VK_UP)
            {
                GlobalInput.IsUpPressed = false;
            }
            else if(VKCode == VK_LEFT)
            {
                GlobalInput.IsLeftPressed = false;
            }
            else if(VKCode == VK_DOWN)
            {
                GlobalInput.IsDownPressed = false;
            }
            else if(VKCode == VK_RIGHT)
            {
                GlobalInput.IsRightPressed = false;
            }
            else if(VKCode == VK_ESCAPE)
            {
                
            }
            else if(VKCode == VK_SPACE)
            {
                
            }
        } break;
        
        default:
        {
            TranslateMessage(&Message);
            DispatchMessageA(&Message);
        } break;
    }
}

internal void
Win32DEBUGDrawKeyInput(HDC ClientDC,
                       int32 WindowWidth,
                       int32 WindowHeight,
                       game_buffer* Buffer = &GlobalBuffer,
                       game_input* Input = &GlobalInput)
{
    int32 RectStartX = 10;
    int32 RectStartY = 20;
    int32 KeyDim = 30;
    uint32 GrayColor = 0x00676767;
    uint32 KeyColor = 0x00ff0000;
    Win32DrawRectangle(RectStartX, RectStartY,
                       KeyDim*3, KeyDim*2,
                       GrayColor,
                       Buffer);
    Win32DrawRectangle(RectStartX, RectStartY,
                       KeyDim, KeyDim,
                       0xff000000,
                       Buffer);
    Win32DrawRectangle(RectStartX + (KeyDim * 2), 
                       RectStartY,
                       KeyDim, KeyDim,
                       0xff000000,
                       Buffer);
    
    if(Input->IsUpPressed) 
    {
        Win32DrawRectangle(RectStartX + KeyDim,
                           RectStartY,
                           KeyDim, KeyDim,
                           KeyColor,
                           Buffer);
    }
    if(Input->IsDownPressed)
    {
        Win32DrawRectangle(RectStartX + KeyDim,
                           RectStartY + KeyDim,
                           KeyDim, KeyDim,
                           KeyColor,
                           Buffer);
    }
    if(Input->IsLeftPressed)
    {
        Win32DrawRectangle(RectStartX,
                           RectStartY + KeyDim,
                           KeyDim, KeyDim,
                           KeyColor,
                           Buffer);
    }
    if(Input->IsRightPressed)
    {
        Win32DrawRectangle(RectStartX + (KeyDim * 2),
                           RectStartY + KeyDim,
                           KeyDim, KeyDim,
                           KeyColor,
                           Buffer);
    }
}

inline int
RoundToCeil(float val)
{
    if(val < 0)
    {
        val -= 0.5;
    }
    else if(val > 0)
    {
        val += 0.5;
    }
    return (int)val;
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
              game_input *NewInput = &GlobalInput, 
              game_input *StaleInput = &GlobalOldInput)
{
    float SpeedModifierX = 0.0f;
    float SpeedModifierY = 0.0f;
    float StraightForceConst = 0.52f;
    float DiagonalForceConst = 0.525f;
    game_input Input = GetInputDiff(NewInput, StaleInput);
    if(Input.IsUpPressed)
    {
        SpeedModifierY = -StraightModifier;
    }
    if(Input.IsDownPressed)
    {
        SpeedModifierY = StraightModifier;
    }
    if(Input.IsLeftPressed)
    {
        SpeedModifierX = -StraightModifier;
    }
    if(Input.IsRightPressed)
    {
        SpeedModifierX = StraightModifier;
    }
    if(Input.IsUpPressed && Input.IsLeftPressed)
    {
        SpeedModifierX = -DiagonalModifier;
        SpeedModifierY = -DiagonalModifier;
    }
    if(Input.IsUpPressed && Input.IsRightPressed)
    {
        SpeedModifierX = DiagonalModifier;
        SpeedModifierY = -DiagonalModifier;
    }
    if(Input.IsDownPressed && Input.IsLeftPressed)
    {
        SpeedModifierX = -DiagonalModifier;
        SpeedModifierY = DiagonalModifier;
    }
    if(Input.IsDownPressed && Input.IsRightPressed)
    {
        SpeedModifierX = DiagonalModifier;
        SpeedModifierY = DiagonalModifier;
    }
    if(Input.IsLeftPressed && Input.IsRightPressed)
    {
        SpeedModifierX = 0;
    }
    if(Input.IsUpPressed && Input.IsDownPressed)
    {
        SpeedModifierY = 0;
    }
    
    Character->Center += V2(RoundToCeil(Character->Speed * SpeedModifierX), 
                            RoundToCeil(Character->Speed * SpeedModifierY));
    v2 TopLeftPos = ConvertCenterToTopLeft(Character->Center, Character->Dim);
    if(TopLeftPos.X < 0)
    {
        TopLeftPos.X = 0;
    }
    else if(TopLeftPos.X > GlobalBuffer.Width - Character->Dim.X)
    {
        TopLeftPos.X = GlobalBuffer.Width - Character->Dim.X;
    }
    if(TopLeftPos.Y < 0)
    {
        TopLeftPos.Y = 0;
    }
    else if(TopLeftPos.Y > GlobalBuffer.Height - Character->Dim.Y)
    {
        TopLeftPos.Y = GlobalBuffer.Height - Character->Dim.Y;
    }
    Character->Center = ConvertTopLeftToCenter(TopLeftPos, Character->Dim);
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
                        UINT Message,
                        WPARAM WParam,
                        LPARAM LParam)
{       
    LRESULT Result = 0;
    
    switch(Message)
    {
        case WM_CLOSE:
        {
            GlobalRunning = false;
        } break;
        
        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        
        case WM_DESTROY:
        {
            GlobalRunning = false;
        } break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            window_width_height Dimension = Win32GetWindowDimension(Window);
            Win32DisplayCurrentBuffersToScreen(DeviceContext,
                                               Dimension.Width,
                                               Dimension.Height);
            EndPaint(Window, &Paint);
        } break;
        
        /*
                case WM_SIZE:
                {
                    ResizeOrCreateDisplayBuffer();
                } break;
                */
        
        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }
    
    return(Result);
}

int WINAPI
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance, 
        LPSTR CmdLinePTR, 
        int ShowCmd)
{
    WNDCLASSA WindowClass = {};
    
    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.hCursor = LoadCursorA(Instance, IDC_ARROW);
    WindowClass.lpszClassName = "2DGameWindowClass";
    
    if(RegisterClassA(&WindowClass))
    {
        HWND Window =  CreateWindowExA(0,
                                       WindowClass.lpszClassName,
                                       "2DGameWindow",
                                       WS_VISIBLE | WS_OVERLAPPEDWINDOW,
                                       CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                       0, 0,
                                       Instance,
                                       0);
        if(Window)
        {
            ShowWindow(Window, ShowCmd);
            HDC DeviceContext = GetDC(Window);
            
            GlobalBuffer = {};
            GlobalBuffer.OffsetX = 10;
            GlobalBuffer.OffsetY = 10;
            Win32ResizeOrCreateDisplayBuffer(1280, 720);
            Win32ResizeOrCreateDisplayBuffer(110, 80, &KeysBuffer);
            
            GlobalRunning = true;
            
            GlobalInput = {};
            GlobalOldInput = {};
            
            game_character RectCharacter = {};
            RectCharacter.Center = V2(30, 30);
            RectCharacter.Dim = V2(30, 30);
            RectCharacter.Accel = 0f;
            RectCharacter.Vel = 0f;
            RectCharacter.Color = 0x00ff00ff;
            while(GlobalRunning)
            {
                Win32ClearDisplayBufferToStaticColors();
                // Get Inputs
                MSG Message = {};
                while(PeekMessageA(&Message, Window, 0, 0, PM_REMOVE))
                {
                    Win32ProcessSystemInput(Message);
                }
                // Process Inputs
                MoveCharacter(&RectCharacter);
                Win32DrawRectangle(ConvertCenterToTopLeft(RectCharacter.Center, RectCharacter.Dim),
                                   RectCharacter.Dim,
                                   RectCharacter.Color);
                window_width_height Dimension = Win32GetWindowDimension(Window);
#if 1
                Win32DEBUGDrawKeyInput(DeviceContext, 
                                       Dimension.Width, Dimension.Height, 
                                       &KeysBuffer);
#endif
                Win32DisplayCurrentBuffersToScreen(DeviceContext,
                                                   Dimension.Width,
                                                   Dimension.Height);
                GlobalOldInput = GlobalInput;
            }
        }
    }
    
    return 0;
}
