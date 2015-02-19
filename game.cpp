#include "game.h"

D3DCOLORVALUE GetColorByIndex(WORD idx)
{
    //�� ������� ����� �������� �������� ����� ��� Direct3D
    D3DCOLORVALUE mat;
    ZeroMemory(&mat,sizeof(D3DCOLORVALUE));
    mat.a=1.0f;
    switch(idx)
    {
        case 0: { mat.r=1.0f; mat.g=0.0f; mat.b=0.0f; break; }
        case 1: { mat.r=0.0f; mat.g=1.0f; mat.b=0.0f; break; }
        case 2: { mat.r=0.0f; mat.g=0.0f; mat.b=1.0f; break; }
        case 3: { mat.r=1.0f; mat.g=0.0f; mat.b=1.0f; break; }
        case 4: { mat.r=1.0f; mat.g=1.0f; mat.b=0.0f; break; }
        case 5: { mat.r=0.0f; mat.g=1.0f; mat.b=1.0f; break; }
    }
    return mat;
}

void TGame::ClearField()
{
    //������� ���� �����
    ZeroMemory(cells,TOTAL_COUNT*sizeof(TCell));
    for(WORD i=0;i<TOTAL_COUNT;i++)
    {
        cells[i].free=TRUE;
    }
}

WORD TGame::GetSelected()
{
    //��������� ������ ���������� ����
    for(WORD i=0;i<TOTAL_COUNT;i++)
    {
        if(cells[i].selected)
        {
            return i;
        }
    }
    return 0xffff;
}

WORD TGame::GetNeighbours(WORD cellId, WORD *pNeighbours)
{
    //������� ���������� �������� ������ ��� �������� - �� ����� ���� �� 2 �� 4
    //� ����������� �� ����, ��� ��������� ������ ������
    WORD count=0;
    if(cellId>=FIELD_SIZE)
    {
        pNeighbours[count++]=cellId-FIELD_SIZE;
    }
    if(cellId<TOTAL_COUNT-FIELD_SIZE)
    {
        pNeighbours[count++]=cellId+FIELD_SIZE;
    }
    if(cellId%FIELD_SIZE>0)
    {
        pNeighbours[count++]=cellId-1;
    }
    if((cellId+1)%FIELD_SIZE)
    {
        pNeighbours[count++]=cellId+1;
    }
    return count;
}

BOOL TGame::CheckPipeDetonate(WORD *pPipeCells)
{
    //������� ���������, �������� �� ��� ������ � ������ �������� ������ ������ �����
    if(cells[pPipeCells[0]].free)
    {
        return FALSE;
    }
    WORD first=cells[pPipeCells[0]].colorIndex;
    BOOL success=TRUE;
    for(WORD i=1;i<DETONATE_COUNT;i++)
    {
        success=success&&(!cells[pPipeCells[i]].free)&&(cells[pPipeCells[i]].colorIndex==first);
    }
    if(success)
    {
        for(WORD i=0;i<DETONATE_COUNT;i++)
        {
            cells[pPipeCells[i]].detonating=TRUE;
        }
    }
    return success;
}

TGame::TGame()
{
    pathLen=0;
    score=0;
    gameOver=FALSE;
    path=NULL;
    cells=new TCell[TOTAL_COUNT];
    ClearField();
}

TGame::~TGame()
{
    if(path!=NULL)
    {
        delete[] path;
        path=NULL;
    }
    if(cells!=NULL)
    {
        delete[] cells;
        cells=NULL;
    }
}

void TGame::New()
{
    pathLen=0;
    score=0;
    gameOver=FALSE;
    if(path!=NULL)
    {
        delete[] path;
        path=NULL;
    }
    //������� ����
    ClearField();
    //������� ��������� ���-�� �����
    CreateBalls(START_COUNT);
}

BOOL TGame::CreateBalls(WORD count)
{
    WORD *freeCellList,freeCellCount=0;
    //������� �������� ������ ���� ��������� �����
    freeCellList=new WORD[TOTAL_COUNT];
    for(WORD i=0;i<TOTAL_COUNT;i++)
    {
        if(cells[i].free)
        {
            freeCellList[freeCellCount++]=i;
        }
    }
    if(freeCellCount<count)
    {
        //���� ��������� ����� ������������, �� ���� ��������
        gameOver=TRUE;
        return FALSE;
    }
    //�������� ������ ����� ��������� �����
    WORD id;
    for(WORD i=0;i<count;i++)
    {
        id=(freeCellCount-1)*rand()/RAND_MAX;
        cells[freeCellList[id]].colorIndex=(COLOR_COUNT-1)*rand()/RAND_MAX;
        cells[freeCellList[id]].detonating=FALSE;
        cells[freeCellList[id]].free=FALSE;
        cells[freeCellList[id]].isNew=TRUE;
        cells[freeCellList[id]].selected=FALSE;
        //������� ���������� ����� �� ������ ���������
        for(WORD j=id+1;j<freeCellCount;j++)
        {
            freeCellList[j-1]=freeCellList[j];
        }
        freeCellCount--;
    }
    delete[] freeCellList;
    return TRUE;
}

void TGame::Select(WORD cellId)
{
    //������� ������� ��������� �� ���� �����, �.�. ������ game �� ������ �������� ����� ����������� ����
    for(WORD i=0;i<TOTAL_COUNT;i++)
    {
        cells[i].selected=FALSE;
    }
    //���� ����������� ������ �� ��������, �� �������� ��������
    if(!cells[cellId].free)
    {
        cells[cellId].selected=TRUE;
    }
}

BOOL TGame::TryMove(WORD targetCellId)
{
    //������� ����������� � �������� ������
    WORD *weight,*currentSet,*goodNeighbours,*allNeighbours,
         currentCount=1,goodNeighbourCount,allNeighbourCount,
         distance=0,
         selectedId,cellId;
    if(!cells[targetCellId].free)
    {
        return FALSE;
    }
    weight=new WORD[TOTAL_COUNT];
    //�������� �������� - ���������� ������� �����.
    //�������������� ��������� ������ ��������� TOTAL_COUNT, � ������� 0xffff
    for(WORD i=0;i<TOTAL_COUNT;i++)
    {
        weight[i]=cells[i].free ? TOTAL_COUNT : 0xffff;
    }
    //������� ����� �����
    currentSet=new WORD[TOTAL_COUNT];
    //����� "�������" ������� - ����� ����� ��������� �� ���
    goodNeighbours=new WORD[TOTAL_COUNT];
    //��� ������ ������� ������ (�� 2 �� 4)
    allNeighbours=new WORD[4];
    //��������� ������
    selectedId=GetSelected();
    //�������������� ������� ����� ����� ������ �����, �.�. �������
    currentSet[0]=selectedId;
    weight[currentSet[0]]=0;
    BOOL finished=FALSE;
    do
    {
        distance++; //������� ����������
        goodNeighbourCount=0; //"�������" ������ - ��� ��������� �������� ������, ���������� ������� �����������, ��� ��� ������� ����� ����
        //�������� �� ���� ������� �������
        for(WORD i=0;(i<currentCount);i++)
        {
            //�������� ���� ������� ����� �� ������� �����
            allNeighbourCount=GetNeighbours(currentSet[i],allNeighbours); //��� 2, 3 ��� 4 ������
            for(WORD j=0;j<allNeighbourCount;j++)
            {
                //���� �������� ������ �������� � �� ��� ������ �������� �������� ���������, �� ��������� �� � ����������, ��� �������� ������
                if((weight[allNeighbours[j]]!=0xffff)&&(weight[allNeighbours[j]]>distance))
                {
                    weight[allNeighbours[j]]=distance;
                    goodNeighbours[goodNeighbourCount++]=allNeighbours[j];
                    if(allNeighbours[j]==targetCellId)
                    {
                        //���� ��� ������ �������, �� ���������� ���������� ������� ����� ���������������
                        finished=TRUE;
                    }
                }
            }
        }
        //��������� ������� ������� � ������� ��������� ����� ��� ��������� ��������
        memcpy(currentSet,goodNeighbours,goodNeighbourCount*sizeof(WORD));
        currentCount=goodNeighbourCount;
    }while((goodNeighbourCount>0)&&!finished); //���� �����������, ���� �� ����� ���������� ������� ������, ���� ���� �� ����� ��������� ��������� ������� �����
    //���� ���� ����������, �� ��� ������� ������ ���������
    if(weight[targetCellId]!=TOTAL_COUNT)
    {
        //������ ���� - ��� ����� �������� �� ������� � �������� �����������,
        //�� ���� �� ������� � �������
        pathLen=distance;
        path=new WORD[pathLen];
        cellId=targetCellId;
        path[pathLen-1]=cellId;
        for(WORD i=1;i<pathLen;i++)
        {
            //�������� ���� ������� ������ (������� � �������)
            allNeighbourCount=GetNeighbours(cellId,allNeighbours);
            for(WORD j=0;j<allNeighbourCount;j++)
            {
                //� ������ ����� ��� ������ ����������� �� ������� - ��� ������������� �������� �����
                if(weight[allNeighbours[j]]==pathLen-i)
                {
                    cellId=allNeighbours[j];
                    path[pathLen-i-1]=cellId;
                    break;
                }
            }
        }
        //����������� ���������� ������ � ��������� ��� � �������
        ZeroMemory(&cells[targetCellId],sizeof(TCell));
        cells[targetCellId].colorIndex=cells[selectedId].colorIndex;
        ZeroMemory(&cells[selectedId],sizeof(TCell));
        cells[selectedId].free=TRUE;
    }
    delete[] weight;
    delete[] currentSet;
    delete[] goodNeighbours;
    delete[] allNeighbours;
    return (pathLen>0);
}

BOOL TGame::DetonateTest()
{
    //���� �� �����
    WORD detonates=0;
    WORD *pipe;
    pipe=new WORD[DETONATE_COUNT];
    //������� ��� ��������� �������������� ��������
    for(WORD i=0;i<FIELD_SIZE;i++)
    {
        //j ������������� ���������� ������������� ����
        for(WORD j=0;j<=FIELD_SIZE-DETONATE_COUNT;j++)
        {
            for(WORD k=0;k<DETONATE_COUNT;k++)
            {
                pipe[k]=i*FIELD_SIZE+j+k;
            }
            //���������� �� �������� �����
            if(CheckPipeDetonate(pipe))
            {
                detonates++;
            }
        }
    }
    //������� ��� ��������� ������������ ��������
    for(WORD i=0;i<FIELD_SIZE;i++)
    {
        //j ������������� ���������� ��������������� ����
        for(WORD j=0;j<=FIELD_SIZE-DETONATE_COUNT;j++)
        {
            for(WORD k=0;k<DETONATE_COUNT;k++)
            {
                pipe[k]=(j+k)*FIELD_SIZE+i;
            }
            if(CheckPipeDetonate(pipe))
            {
                detonates++;
            }
        }
    }
    //������� ��� ������ ���������
    for(WORD i=0;i<=FIELD_SIZE-DETONATE_COUNT;i++)
    {
        for(WORD j=0;j<=FIELD_SIZE-DETONATE_COUNT;j++)
        {
            for(WORD k=0;k<DETONATE_COUNT;k++)
            {
                pipe[k]=(i+k)*FIELD_SIZE+j+k;
            }
            if(CheckPipeDetonate(pipe))
            {
                detonates++;
            }
        }
    }
    //������� ��� �������� ���������
    for(WORD i=DETONATE_COUNT-1;i<FIELD_SIZE;i++)
    {
        for(WORD j=0;j<=FIELD_SIZE-DETONATE_COUNT;j++)
        {
            for(WORD k=0;k<DETONATE_COUNT;k++)
            {
                pipe[k]=(i-k)*FIELD_SIZE+j+k;
            }
            if(CheckPipeDetonate(pipe))
            {
                detonates++;
            }
        }
    }
    delete[] pipe;
    return (detonates>0);
}

WORD TGame::GetNewBallList(TBallInfo **ppNewList)
{
    //������� ���������� ������ ������� ��������� �����,
    //����� ���� ��� ��� ��������� ������� (���� isNew ������������)
    WORD newBallCount=0,currentId=0;
    //������� ������� ����������
    for(WORD i=0;i<TOTAL_COUNT;i++)
    {
        if(cells[i].isNew)
        {
            newBallCount++;
        }
    }
    if(newBallCount==0)
    {
        return 0;
    }
    *ppNewList=new TBallInfo[newBallCount];
    for(WORD i=0;i<TOTAL_COUNT;i++)
    {
        if(cells[i].isNew)
        {
            //����� � ������ ID � ���� d3dcolorvalue
            cells[i].isNew=FALSE;
            (*ppNewList)[currentId].cellId=i;
            (*ppNewList)[currentId].color=GetColorByIndex(cells[i].colorIndex);
            currentId++;
        }
    }
    return newBallCount;
}

WORD TGame::GetLastMovePath(WORD **ppMovePath)
{
    //������� ���������� ���� ���������� ����������� ����
    if(path==NULL)
    {
        return 0;
    }
    *ppMovePath=new WORD[pathLen];
    memcpy(*ppMovePath,path,pathLen*sizeof(WORD));
    //����� ����� ������� ����
    delete[] path;
    path=NULL;
    WORD tmp=pathLen;
    pathLen=0;
    return tmp;
}

WORD TGame::GetDetonateList(WORD **ppDetonateList)
{
    //������� ���������� ������ ������������ �����, ����� ����
    //������� ��
    WORD detonateCount=0,currentId=0;
    for(WORD i=0;i<TOTAL_COUNT;i++)
    {
        if(cells[i].detonating)
        {
            detonateCount++;
        }
    }
    if(detonateCount==0)
    {
        return 0;
    }
    *ppDetonateList=new WORD[detonateCount];
    for(WORD i=0;i<TOTAL_COUNT;i++)
    {
        if(cells[i].detonating)
        {
            (*ppDetonateList)[currentId]=i;
            ZeroMemory(&cells[i],sizeof(TCell));
            cells[i].free=TRUE;
            currentId++;
        }
    }
    //����������� ���� ��������������� ����� ���������� �����
    score+=detonateCount*100;
    return detonateCount;
}

LONG TGame::GetScore()
{
    return score;
}

BOOL TGame::IsGameOver()
{
    return gameOver;
}
