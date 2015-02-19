#include "app.h"

void TApplication::RegWindow()
{
    WNDCLASSEX wcx;
    ZeroMemory(&wcx,sizeof(WNDCLASSEX));
    wcx.cbClsExtra=0;
    wcx.cbSize=sizeof(WNDCLASSEX);
    wcx.cbWndExtra=sizeof(LONG_PTR);
    wcx.hbrBackground=(HBRUSH)COLOR_BACKGROUND;
    wcx.hCursor=LoadCursor(NULL,IDC_ARROW);
    wcx.hIcon=LoadIcon(hInstance,IDI_APPLICATION);
    wcx.hIconSm=LoadIcon(hInstance,IDI_APPLICATION);
    wcx.hInstance=hInstance,
    wcx.lpfnWndProc=&TApplication::MsgProc;
    wcx.lpszClassName=WINDOW_CLASSNAME;
    wcx.lpszMenuName=NULL;
    wcx.style=CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    RegisterClassEx(&wcx);
}

LRESULT CALLBACK TApplication::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TApplication* app=(TApplication*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
    switch(msg)
    {
    case WM_MOUSEMOVE:
        {
            WORD x,y;
            x=LOWORD(lParam);
            y=HIWORD(lParam);
            if(wParam & MK_RBUTTON)
            {
                app->GetEngine()->OnRotateX(x-app->mouseCoords.x);
                app->GetEngine()->OnRotateY(y-app->mouseCoords.y);
            }
            app->mouseCoords.x=x;
            app->mouseCoords.y=y;
            return 0;
        }
    case WM_MOUSEWHEEL:
        {
            app->GetEngine()->OnZoom((short)HIWORD(wParam)/120);
            return 0;
        }
    case WM_LBUTTONDOWN:
        {
            if(app->GetGame()->IsGameOver())
            {
                return 0;
            }
            BOOL IsCell;
            WORD id=app->GetEngine()->OnClick(LOWORD(lParam),HIWORD(lParam),&IsCell);
            //���������, ������������� �� �������� �� �������
            if(id<TOTAL_COUNT)
            {
                //���� �� ������? (engine ��� ���� �����������, ��� �����-�� ��� ��� ��� ������)
                if(IsCell)
                {
                    //������� �����������
                    if(app->GetGame()->TryMove(id))
                    {
                        //���������� �����������
                        WORD *path,
                             pathLen=app->GetGame()->GetLastMovePath(&path);
                        app->GetEngine()->MoveBall(path,pathLen);
                        app->moveStarted=TRUE;
                        //� ����� ���� �� ��������� ��������
                    }
                }
                else
                {
                    //������ �������� ���
                    app->GetGame()->Select(id);
                }
            }
            return 0;
        }
    case WM_KEYDOWN:
        {
            if(wParam==VK_SPACE)
            {
                //����� ����
                app->GetEngine()->OnResetGame();
                app->GetGame()->New();
                TBallInfo *info;
                WORD appearCount=app->GetGame()->GetNewBallList(&info);
                app->GetEngine()->AppearBalls(info,appearCount);
                delete[] info;
            }
            else if(wParam==VK_ESCAPE)
            {
                //�����
                PostQuitMessage(0);
            }
            return 0;
        }
    case WM_SIZE:
        {
            app->GetEngine()->OnResize();
            return 0;
        }
    case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
    default:
        {
            return DefWindowProc(hwnd,msg,wParam,lParam);
        }
    }
}

void TApplication::ProcessGame()
{
    if(moveStarted)
    {
        //���� �� ��������� �������� �����������
        if(!engine->IsMoving())
        {
            //����������� �������� - ��������� �� �����
            moveStarted=FALSE;
            if(game->DetonateTest())
            {
                //���������� ����� � ����������� ����
                WORD *detonateList,
                count=game->GetDetonateList(&detonateList);
                detonateStarted=TRUE;
                engine->DetonateBalls(detonateList,count);
                engine->OnUpdateScore(game->GetScore());
            }
            else
            {
                //����� �������� �������� ����
                if(game->CreateBalls(APPEAR_COUNT))
                {
                    TBallInfo *appearList;
                    WORD count=game->GetNewBallList(&appearList);
                    appearStarted=TRUE;
                    engine->AppearBalls(appearList,count);
                }
                else
                {
                    //game over!
                }
            }
        }
    }
    if(appearStarted)
    {
        //���� �� ��������� �������� ���������
        if(!engine->IsAppearing())
        {
            appearStarted=FALSE;
            //��������� �������� - ��������� �� ����� �� ������ ������
            if(game->DetonateTest())
            {
                //���������� ����� � ����������� ����
                WORD *detonateList,
                count=game->GetDetonateList(&detonateList);
                detonateStarted=TRUE;
                engine->DetonateBalls(detonateList,count);
                engine->OnUpdateScore(game->GetScore());
            }
        }
    }
    if(detonateStarted)
    {
        //���� �� ��������� �������� ������
        if(!engine->IsDetonating())
        {
            //������ ���������� ����
            detonateStarted=FALSE;
        }
    }
}

TApplication::TApplication(HINSTANCE hInstance, INT cmdShow)
{
    this->hInstance=hInstance;
    RegWindow();
    mouseCoords.x=0;
    mouseCoords.y=0;
    moveStarted=FALSE;
    appearStarted=FALSE;
    detonateStarted=FALSE;
    hWindow=CreateWindow(WINDOW_CLASSNAME,WINDOW_TITLE,WS_POPUP,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,NULL,NULL,hInstance,0);
    //��� ��� �������� ��������� �� ������ app ����� � ��������� �������� ����,
    //����� ����� ���� ���������� �� ������������ ������ MsgProc
    SetWindowLongPtr(hWindow,GWLP_USERDATA,(LONG_PTR)this);
    engine=new TEngine(hWindow);
    game=new TGame();
    ShowWindow(hWindow,cmdShow);
    UpdateWindow(hWindow);
    //new game
    SendMessage(hWindow,WM_KEYDOWN,VK_SPACE,0);
}

TApplication::~TApplication()
{
    if(engine!=NULL)
    {
        delete engine;
    }
    if(game!=NULL)
    {
        delete game;
    }
}

TEngine* TApplication::GetEngine()
{
    return engine;
}

TGame* TApplication::GetGame()
{
    return game;
}

INT TApplication::MainLoop()
{
    MSG msg;
    ZeroMemory(&msg,sizeof(MSG));
    while(msg.message!=WM_QUIT)
    {
        if(PeekMessage(&msg,NULL,0,0,PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            //���� ��� ���������, �� ������������ ��������� ���� � ���������� �����������
            ProcessGame();
            engine->Render();
        }
    }
    return (INT)msg.wParam;
}
