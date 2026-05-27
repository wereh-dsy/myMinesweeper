#include "minesweeper_engine.h"
#include <queue>
#include <random>
#include <algorithm>
#include <stdexcept>
#include <cmath>

int Engine::toidx(int x,int y) const{
    return x*config_.col+y;
}

bool Engine::isProtected(int r,int c,int firstR,int firstC)
{
    //保护第一步点击的格子和周围八个格子
    return std::abs(r-firstR)<=1&&std::abs(c-firstC)<=1;
}

void Engine::countRoundMines()
{
    for (int x=0;x<config_.row;x++)
    {
        for (int y=0;y<config_.col;y++)
        {
            Cell &cell = board_[toidx(x,y)];//取当前格子的引用
            if (cell.hasMine)//雷格子本身不显示数字
            {
                cell.roundMines = 0;
                continue;
            }
            int cnt = 0;
            for (int dx=-1;dx<=1;dx++)
            {
                for (int dy=-1;dy<=1;dy++)
                {
                    if (dx==0&&dy==0)
                    {
                        continue;
                    }
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx<0||nx>=config_.row||ny<0||ny>=config_.col)
                    {
                        continue;
                    }
                    if (board_[toidx(nx,ny)].hasMine)
                    {
                        cnt++;
                    }
                }
            }
            cell.roundMines = cnt;
        }
    }
}

bool Engine::checkWin() const
{
    //赢了的条件是翻开的格子数等于总格子数减去地雷数
    if (cntRevealed_==config_.row*config_.col-config_.mines)
    {
        return true;
    }
    return false;
}

void Engine::bfsReveal(int x, int y, MoveResult& res)
{
    /*
    bfs翻开格子
    用std库的queue来实现
    */
    std::queue<Position> q;
    q.push({x,y});
    while (!q.empty())
    {
        Position pos = q.front();
        q.pop();

        Cell &cell = board_[toidx(pos.x,pos.y)];
        if (cell.isRevealed||cell.isFlagged)
        {
            continue;
        }

        //翻开当前格
        cell.isRevealed = true;
        cntRevealed_++;
        res.changed.push_back(pos);

        //如果不是0就不再扩展
        if (cell.roundMines!=0)
        {
            continue;
        }

        //roundMines==0时可以扩展周围8格
        for (int dx=-1;dx<=1;dx++)
        {
            for (int dy=-1;dy<=1;dy++)
            {
                if (dx==0&&dy==0)
                {
                    continue;
                }
                int nx = pos.x + dx;
                int ny = pos.y + dy;
                if (nx<0||nx>=config_.row||ny<0||ny>=config_.col)
                {
                    continue;
                }
                Cell &next = board_[toidx(nx,ny)];
                if (!next.isRevealed&&!next.isFlagged)
                {
                    q.push({nx,ny});
                }
            }
        }
    }
}

void Engine::revealAllMines(MoveResult& res)
{
    for (int x=0;x<config_.row;x++)
    {
        for (int y=0;y<config_.col;y++)
        {
            Cell &cell = board_[toidx(x,y)];
            if (cell.hasMine&&!cell.isRevealed)
            {
                cell.isRevealed = true;
                res.changed.push_back({x,y});
            }
        }
    }
}


const Cell& Engine::getCell(int x, int y) const
{
    return board_[toidx(x, y)];
}

GameState Engine::getState() const
{
    return state_;
}

int Engine::getTime() const
{
    return times_;
}

int Engine::getMinesLeft() const
{
    return config_.mines - cntFlags_;
}

int Engine::getRows() const
{
    return config_.row;
}

int Engine::getCols() const
{
    return config_.col;
}


void Engine::placeMines(int firstr, int firstc)
{
    if (minesPlaced_)
    {
        return;
    }
    //随机放置地雷
    std::vector<int> pos;
    pos.reserve(config_.row*config_.col);
    for (int x=0;x<config_.row;x++)
    {
        for (int y=0;y<config_.col;y++)
        {
            if (!isProtected(x,y,firstr,firstc))
            {
                pos.push_back(toidx(x,y));
            }
        }
    }
    //将合规的也就是不在3x3里的格子放进候选的pos里

    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(pos.begin(),pos.end(),gen);

    for (int i=0;i<config_.mines;i++)
    {
        board_[pos[i]].hasMine = true;
    }
    //随机数然后放雷

    countRoundMines();
    minesPlaced_ = true;
}

void Engine::newGame(const GameConfig &config){
    if(config.row<=0||config.col<=0){//行数列数合法性，抛出异常
        throw std::invalid_argument("Err!!!lines or cols must >0!");
    }
    if(config.mines<0||config.mines>=(config.row*config.col)){//地雷数量合法性，抛出异常
        throw std::invalid_argument("Err!!!mines' count is invalid!");
    }

    //初始化
    config_ = config;
    state_ = GameState::SReady;

    minesPlaced_ = false;
    cntFlags_ = 0;
    cntRevealed_ = 0;
    times_ = 0;

    board_.assign(config_.row*config.col,Cell{});
}

void Engine::tick(int second){
    if(second<=0){
        return;
    }
    if(state_==GameState::SRun){
        times_ += second;
    }
}

MoveResult Engine::setFlag(int x,int y){
    MoveResult res;
    res.afterState = state_;
    if (x<0||x>=config_.row||y<0||y>=config_.col)
    {
        //越界则直接返回
        return res;
    }
    if (state_==GameState::SWin||state_==GameState::SLost)
    {
        //赢了or输了则直接返回
        return res;
    }

    Cell &cell = board_[toidx(x,y)];//取当前格子的引用
    if (cell.isRevealed)
    {
        //翻开了则直接返回
        return res;
    }

    if (!cell.isFlagged)//翻转状态，但会检查是否翻成负数了
    {
        if (cntFlags_ >= config_.mines)
        {
            res.afterState = state_;
            res.isRejected = true;
            return res;
        }

        cell.isFlagged = true;
        cntFlags_++;
    }
    else
    {
        cell.isFlagged = false;
        cntFlags_--;
    }

    res.changed.push_back({x,y});//记录变化让前端ui处理
    res.afterState = state_;
    return res;
}

MoveResult Engine::reveal(int x,int y)
{
    MoveResult res;
    res.afterState = state_;

    if (x<0||x>=config_.row||y<0||y>=config_.col)
    {
        //越界则直接返回
        return res;
    }
    if (state_==GameState::SWin||state_==GameState::SLost)
    {
        //赢了or输了则直接返回
        return res;
    }

    Cell &cell = board_[toidx(x,y)];//取当前格子的引用
    if (cell.isRevealed||cell.isFlagged)
    {
        return res;
        //已翻开or已插旗则直接返回
    }

    if (!minesPlaced_)
    {
        placeMines(x,y);
        state_ = GameState::SRun;
        res.isStart = true;
        //首击后布雷然后进入运行状态
    }

    if (cell.hasMine)
    {
        cell.isRevealed = true;
        state_ = GameState::SLost;
        res.isExploded = true;
        res.changed.push_back({x,y});

        revealAllMines(res);//失败则展开所有雷

        res.afterState = state_;
        return res;
        //踩雷了则翻开格子，进入失败状态
    }

    if (cell.roundMines==0)
    {
        bfsReveal(x,y,res);
    }
    else
    {
        cell.isRevealed = true;
        cntRevealed_++;
        res.changed.push_back({x,y});
    }
    //普通格子翻开然后返回

    if (checkWin())
    {
        state_ = GameState::SWin;
        //胜利判断
    }
    res.afterState = state_;
    return res;
}