#if defined(UNICODE) && !defined(_UNICODE)
    #define _UNICODE
#elif defined(_UNICODE) && !defined(UNICODE)
    #define UNICODE
#endif

#include<string.h>
#include <tchar.h>
#include <windows.h>
#include <stdio.h>

#define ALLOC_SIZE 1000
#define VSCROLL_MAX USHRT_MAX
#define ALLOC_MULT 3

// IsOpenFile
typedef enum STATE_T
{
    IN_FILE,
    OUT_OF_FILE,
} STATE;

typedef struct STRING_T
{
    TCHAR* str;
} STRING;

typedef struct TEXT_T
{
    STRING* t;
    POINT metrics;
    char* currPos;
} TEXT;


/*  Declare Windows procedure  */
LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);

/*  Make the class name into a global variable  */
TCHAR szClassName[ ] = _T("CodeBlocksWindowsApp");

int WINAPI WinMain (HINSTANCE hThisInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpszArgument,
                     int nCmdShow)
{
    HWND hwnd;               /* This is the handle for our window */
    MSG messages;            /* Here messages to the application are saved */
    WNDCLASSEX wincl;        /* Data structure for the windowclass */

    /* The Window structure */
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;      /* This function is called by windows */
    wincl.style = CS_DBLCLKS;                 /* Catch double-clicks */
    wincl.cbSize = sizeof (WNDCLASSEX);

    /* Use default icon and mouse-pointer */
    wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;                 /* No menu */
    wincl.cbClsExtra = 0;                      /* No extra bytes after the window class */
    wincl.cbWndExtra = 0;                      /* structure or the window instance */
    /* Use Windows's default colour as the background of the window */
    wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

    /* Register the window class, and if it fails quit the program */
    if (!RegisterClassEx (&wincl))
        return 0;

    /* The class is registered, let's create the program*/
    hwnd = CreateWindowEx (
           0,                   /* Extended possibilites for variation */
           szClassName,         /* Classname */
           _T("Code::Blocks Template Windows App"),       /* Title Text */
           WS_OVERLAPPEDWINDOW, /* default window */
           CW_USEDEFAULT,       /* Windows decides the position */
           CW_USEDEFAULT,       /* where the window ends up on the screen */
           544,                 /* The programs width */
           375,                 /* and height in pixels */
           HWND_DESKTOP,        /* The window is a child-window to desktop */
           NULL,                /* No menu */
           hThisInstance,       /* Program Instance handler */
           lpszArgument         /* No Window Creation data */
           );

    /* Make the window visible on the screen */
    ShowWindow (hwnd, nCmdShow);

    /* Run the message loop. It will run until GetMessage() returns 0 */
    while (GetMessage (&messages, NULL, 0, 0))
    {
        /* Translate virtual-key messages into character messages */
        TranslateMessage(&messages);
        /* Send message to WindowProcedure */
        DispatchMessage(&messages);
    }

    /* The program return-value is 0 - The value that PostQuitMessage() gave */
    return messages.wParam;
}
unsigned int GetTextSize(FILE* file)
{
    unsigned int size = 0;

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);
    size = size - ftell(file);
    size /= sizeof(TCHAR);
    return size;
}

void InitPoint(POINT* p)
{
    p->x = 0;
    p->y = 0;
    return;
}

bool TextInit(TEXT* t,
              int size)
{
    if(t)
        if(t->t = (STRING*) malloc(size * sizeof(STRING)))
        {
            InitPoint(&(t->metrics));
            t->currPos = NULL;
            return true;
        }

    return false;
}

void TextDestruct(TEXT* t)
{
    free(t->t[0].str);
    free(t->t);
    free(t);
}

TEXT* TextToStrings(TCHAR* t, char* viewLine)
{
    TEXT* result = (TEXT*) malloc(sizeof(TEXT));
    TCHAR* endOfLine = strchr(t, '\n') + 1;
    TCHAR* currLine = t;
    STRING* buff = NULL;
    int i;
    int availSize = ALLOC_SIZE;

    if(result)
    if(TextInit(result, availSize + 1))
        for(i = 0; endOfLine - 1; i++)
        {
            if(!result->currPos && viewLine && currLine > viewLine)
                result->currPos = result->t[i - 1].str;

            if(endOfLine - currLine > result->metrics.x)
                result->metrics.x = endOfLine - currLine;

            result->t[i].str = currLine;
            currLine = endOfLine;
            endOfLine = strchr(currLine, '\n') + 1;
            result->metrics.y++;
            if(result->metrics.y >= (size_t) availSize)
            {
                if(buff = (STRING*) realloc(result->t, ALLOC_MULT * availSize * sizeof(STRING)))
                    result->t = buff;
                else
                {
                    TextDestruct(result);
                    return NULL;
                }
                availSize *= ALLOC_MULT;
            }
        }
    else
        return NULL;

    result->t[i].str = 0;
    if (!viewLine)
        result->currPos = t;
    return result;
}

TEXT* GetTextFromFile(FILE* file)
{
    TCHAR* tchar_res = NULL;
    TEXT* res = NULL;
    unsigned int size = GetTextSize(file);
    unsigned int fread_size;

    tchar_res = (TCHAR*)malloc((size + 1) * sizeof(TCHAR));
    if(!tchar_res)
        return NULL;

    fread_size = fread(tchar_res, sizeof(TCHAR), size, file);
    if(fread_size != size)
    {
        free(tchar_res);
        return NULL;
    }

    tchar_res[size] = 0;
    fclose(file);
    res = TextToStrings(tchar_res, NULL);

    return res;
}
/*  This function is called by the Windows function DispatchMessage()  */
LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    char* filename = NULL; // for file opening
    TEXT* buff = NULL; // buffer with my text type
    STATE currState; // current state - in or out of file
    FILE* file = NULL;

    HDC hdc; // handler device context

    PAINTSTRUCT paintStruct; // for painting
    RECT rectText;
    HFONT hfont;
    TEXTMETRIC fontMetric; //current font

    static TCHAR name[1000]; //name of displayed file

    switch (message)                  /* handle the messages */
    {
        case WM_CREATE:
            filename = (char*)(((CREATESTRUCT*)lParam)->lpCreateParams);
            if(filename)
            {
                file = fopen(filename,"r");
                currState = IN_FILE;
                buff = GetTextFromFile(file);
                fclose(file);
            }
            else
                currState = OUT_OF_FILE;
            hdc = GetDC(hwnd);
            hfont = CreateFont(15,10,-300,0,FW_DONTCARE,FALSE,FALSE,FALSE,
                               DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,CLIP_DEFAULT_PRECIS,
                               ANTIALIASED_QUALITY,VARIABLE_PITCH,TEXT("Courier"));
            SelectObject(hdc, hfont);
            GetTextMetrics(hdc, &fontMetric);
            GetClientRect(hwnd, &rectText);
            ReleaseDC(hwnd, hdc);
            break;  // WM_Size, WM_scroll, WM_Command,

        case WM_PAINT:
            hdc = BeginPaint(hwnd, &paintStruct);
            if(currState == IN_FILE)
                PrintText(hdc, buff, viewPos, fontMetric.tmHeight, &rectText);
            EndPaint(hwnd, &paintStruct);
            break;

        case WM_DESTROY:
            PostQuitMessage (0);       /* send a WM_QUIT to the message queue */
            break;
        default:                      /* for messages that we don't deal with */
            return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}
