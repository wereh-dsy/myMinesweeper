/*
minesweeper后端部分

功能：
管理游戏状态（SReady/SRun/SWin/SLost）
负责布雷、计算周围雷数、翻开格子、插旗、胜负判断、计时
提供前端调用的接口（reveal/setFlag/newGame/tick/查询类接口）

newGame(config)初始化棋盘与状态
reveal(x,y)左键点击
    首击触发布雷保护3x3
    点雷失败展开全部地雷
    点空区触发bfs自动展开
    胜利判断
setFlag(x,y)右键插旗/取消旗
tick()计时

接口：
getCell(x,y)获取格子状态
getState()获取游戏状态
getTime()获取已用时间
getMinesLeft()剩余雷数（雷数-旗数）
getRows()/getCols()获取棋盘尺寸

MoveResult：
changed：本次操作后需要刷新的格子列表
isStart：首次点击开始游戏
isExploded：是否踩雷
afterState：操作后的游戏状态
*/

#ifndef MINESWEEPER_ENGINE_H
#define MINESWEEPER_ENGINE_H

#include <vector>

enum class GameState{SReady,SRun,SWin,SLost};

struct GameConfig
{
    int row=9,col=9,mines=10;
};

struct Cell
{
    bool hasMine=false,isRevealed=false,isFlagged=false;
    int roundMines=0;
};

struct Position
{
    int x,y;
};

struct MoveResult
{
    std::vector<Position> changed;
    bool isStart=false;
    bool isExploded=false;
    bool isRejected=false;
    GameState afterState=GameState::SReady;
};

class Engine
{
private:
    //私有的状态，封装在private里
    GameConfig config_;
    GameState state_=GameState::SReady;
    std::vector<Cell> board_;
    bool minesPlaced_=false;

    int cntFlags_=0;
    int cntRevealed_=0;
    int times_=0;

    //私有的工具函数
    //不会修改引擎状态的都加了const，另外isProtected不依赖于engine对象，所以加static，下面public里的同理
    int toidx(int x,int y) const;
    static bool isProtected(int r,int c,int firstR,int firstC);
    void countRoundMines();
    bool checkWin() const;
    void bfsReveal(int x,int y,MoveResult &res);
    void revealAllMines(MoveResult &res);

public:
    //供前端ui调用的api函数
    void newGame(const GameConfig &config);
    void tick(int second=1);
    MoveResult reveal(int x,int y);
    MoveResult setFlag(int x,int y);
    void placeMines(int firstr,int firstc);

    const Cell& getCell(int x, int y) const;
    GameState getState() const;
    int getTime() const;
    int getMinesLeft() const;
    int getRows() const;
    int getCols() const;
};

#endif
