#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "minesweeper_engine.h"

#include <QPainter>
#include <QMouseEvent>
#include <QMessageBox>
#include <QComboBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <filesystem>
#include <fstream>
#include <QMenuBar>
#include <QMenu>
#include <QAction>

static QColor askColor(int n) //根据雷数返回颜色
{
    switch (n)
    {
    case 1: return QColor(255, 102, 102); //1返回红色
    case 2: return QColor(255, 255, 102); //2返回黄色
    case 3: return QColor(102, 255, 102); //3返回绿色
    case 4: return QColor(102, 255, 255); //4返回蓝色
    case 5: return QColor(255, 178, 102); //5返回橙色
    case 6: return QColor(178, 102, 255); //6返回紫色
    case 7: return QColor(255, 102, 178); //7返回粉色
    case 8: return QColor(102, 102, 255); //8返回深蓝色
    default: return QColor(0, 0, 0); //返回黑色
    }
}

class BoardWidget : public QWidget
{
public:
    explicit BoardWidget(Engine& engine, QWidget* parent = nullptr)
        : QWidget(parent), engine_(engine) //初始化列表，engine_是一个引用成员，必须写在初始化列表中在构造时完成其本身的初始化
    {
        //setMouseTracking(true);//此行对于当前程序没用
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed); //保证棋盘大小不随着窗口改变
    }

    void setCellSize(int sz) //设置格子的像素大小
    {
        cellSize_ = sz;
        setFixedSize(cols_ * cellSize_, rows_ * cellSize_);
        update();
    }

    void setStarted(bool flag)
    {
        started_ = flag;
        update();
    }

    void setBoardSize()
    {
        rows_ = engine_.getRows();
        cols_ = engine_.getCols();
        setFixedSize(cols_ * cellSize_, rows_ * cellSize_);
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override //子类override要保证函数签名的一致，但是这里用不到这个指针，所以只空写一个*而不命名
    {
        /*解释一下画格子用到的函数
        fillrect是用rgb颜色把矩形区域填满
        setpen就是指定画线/画框用什么笔（颜色，宽度）
        drawline就是用设置好的笔来画一条线段
        drawrect就是画矩形的边框
        adjusted是用来调整矩形的，参数分别是左上右下的调整值，正数是往外扩展，负数是往内收缩
        setbrush就是设置填充的颜色，与下面的那个搭配
        drawpolygon是用上面brush设置的颜色来填充，pen设置的颜色来画边框，画一个多边形
        drawellipse画圆，（中心的，半径，半径），其它类似polygon
        */
        QPainter p(this);
        p.fillRect(rect(), QColor(192, 192, 192)); //把控件自身的矩形区域（由rect返回）涂满成灰色

        if (!started_)
        {
            return;
        }

        for (int x = 0; x < rows_; x++)
        {
            for (int y = 0; y < cols_; y++) //遍历每个格子
            {
                QRect r(y * cellSize_, x * cellSize_, cellSize_, cellSize_); //计算该格子在窗口里的像素矩形区域，水平像素在前所以y在前，垂直像素在后所以x在后
                const Cell& cell = engine_.getCell(x, y); //取出该格当前状态

                if (!cell.isRevealed) //如果没有翻开过
                {
                    //此处效仿windows扫雷的效果
                    p.fillRect(r, QColor(192, 192, 192)); //格子填成浅灰色

                    //用不同颜色线段来模拟出3d效果
                    p.setPen(QPen(Qt::white, 2)); //白色笔
                    p.drawLine(r.topLeft(), r.topRight()); //上高光
                    p.drawLine(r.topLeft(), r.bottomLeft()); //左高光

                    p.setPen(QPen(QColor(128, 128, 128), 2)); //深灰色笔
                    p.drawLine(r.bottomLeft(), r.bottomRight()); //下阴影
                    p.drawLine(r.topRight(), r.bottomRight()); //右阴影

                    if (cell.isFlagged) //如果插了旗子
                    {
                        QRect fr = r.adjusted(4, 4, -4, -4);
                        int px = fr.left() + fr.width() / 2; //旗子竿位置

                        p.setPen(QPen(Qt::black, 2));
                        p.drawLine(QPoint(px, fr.top()), QPoint(px, fr.bottom()));

                        QPolygon flag; //用于画多边形，表示一个多边形的顶点的集合
                        flag << QPoint(px, fr.top()) << QPoint(fr.right(), fr.top() + fr.height() / 4) << QPoint(
                            px, fr.top() + fr.height() / 2); //加入三个点

                        p.setBrush(QColor(255, 0, 0));
                        p.setPen(Qt::NoPen);
                        p.drawPolygon(flag);

                        p.setBrush(Qt::NoBrush); //清空brush防止出现bug
                    }
                }
                else //翻开了
                {
                    //这三行画已经被翻开的格子的底色和边框
                    p.fillRect(r, QColor(210, 210, 210)); //比未翻开还要浅一点的灰色
                    p.setPen(QPen(QColor(160, 160, 160), 1));
                    p.drawRect(r.adjusted(0, 0, -1, -1));

                    if (cell.hasMine) //有雷
                    {
                        QPoint c = r.center();
                        int rad = cellSize_ / 4;

                        p.setBrush(Qt::black);
                        p.setPen(Qt::NoPen); //不要边框
                        p.drawEllipse(c, rad, rad);
                        p.setBrush(Qt::NoBrush);
                    }
                    else if (cell.roundMines > 0) //没有雷则画数字
                    {
                        QFont f = p.font(); //获得当前painter的字体副本
                        f.setBold(true); //加粗字体
                        f.setPointSize(cellSize_ * 0.55); //0.55倍比例的字号
                        p.setFont(f); //应用该字体

                        p.setPen(askColor(cell.roundMines)); //从askcolor获得颜色
                        p.drawText(r, Qt::AlignCenter, QString::number(cell.roundMines)); //居中
                    }
                }
            }
        }
    }

    void mousePressEvent(QMouseEvent* event) override //处理鼠标事件
    {
        if (!started_)
        {
            return;
        }
        //把鼠标电子的坐标换算成行和列
        int y = event->position().x() / cellSize_;
        int x = event->position().y() / cellSize_;
        if (x < 0 || x >= rows_ || y < 0 || y >= cols_) return;

        //得到返回结果
        MoveResult res;

        if (event->button() == Qt::LeftButton)
        {
            res = engine_.reveal(x, y);
        }
        else if (event->button() == Qt::RightButton)
        {
            res = engine_.setFlag(x, y);

            if (res.isRejected) //不能让计数变成负数，弹框提示一下
            {
                QMessageBox::information(this, "警告", "旗子数量已达到雷数上限，再思考一下吧！");
                return;
            }
        }
        else
        {
            return;
        }

        update(); //刷新界面

        if (res.afterState == GameState::SWin)
        {
            static_cast<MainWindow*>(window())->updateBestScores();
            QMessageBox::information(this, "胜利", "恭喜你，你赢了！");
        }
        else if (res.afterState == GameState::SLost)
        {
            QMessageBox::information(this, "失败", "你失败了，再来一次吧！");
        }
    }

private:
    Engine& engine_;
    int rows_ = 0;
    int cols_ = 0;
    int cellSize_ = 24;
    bool started_ = false;
};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
      , ui(new Ui::MainWindow) //先构造基类，所以QMainWindow必须放在初始化列表，ui则可以不用
{
    ui->setupUi(this);

    setWindowTitle(QStringLiteral("扫雷")); //设置标题
    setWindowIcon(QIcon(QString::fromUtf8(APP_ICON_PATH))); //设置图标，宏定义在cmakelist里，./icons/minesweeperICON.ico

    //菜单部分，游戏（重新开始）-成绩（查看最佳成绩/清除最佳成绩）-关于（关于本程序）
    QMenu* menuGame = menuBar()->addMenu(QStringLiteral("游戏"));
    QAction* actRestart = menuGame->addAction(QStringLiteral("重新开始"));

    QMenu* menuScore = menuBar()->addMenu(QStringLiteral("成绩"));
    QAction* actShowBest = menuScore->addAction(QStringLiteral("查看最佳成绩"));
    QAction* actClearBest = menuScore->addAction(QStringLiteral("清除最佳成绩"));

    QMenu* menuAbout = menuBar()->addMenu(QStringLiteral("关于"));
    QAction* actAbout = menuAbout->addAction(QStringLiteral("关于本程序"));

    connect(actRestart, &QAction::triggered, this, [this]()
    {
        startSelection();
    });//把重新开始关联到startselection，作用等同于再点一次开始

    connect(actShowBest, &QAction::triggered, this, [this]()
    {
        showBestScores();
    });

    connect(actClearBest, &QAction::triggered, this, [this]()
    {
        clearBestScores();
    });

    connect(actAbout, &QAction::triggered, this, [this]()
    {
        QMessageBox::information(this, QStringLiteral("关于"),QStringLiteral("本程序是基于Qt框架、使用C++语言开发的一款简易扫雷游戏\n支持五种难度、计时和展示最佳成绩等功能\n画面模仿了Windows自带扫雷的风格\n\n本程序为东北大学C++课程的课程设计作业\n作者：东北大学计算机科学与技术2026级3班杜圣宇、郝一霖"));
    });

    //创建根布局
    auto* root = new QVBoxLayout(ui->centralwidget);
    root->setContentsMargins(10, 10, 10, 10); //每个边的边距设为10
    root->setSpacing(8); //垂直方向每个布局项之间的间距设为8

    auto* top = new QHBoxLayout();
    top->setSpacing(8);

    minesLabel_ = new QLabel(QStringLiteral("雷: 0"), ui->centralwidget);
    timerLabel_ = new QLabel(QStringLiteral("00:00"), ui->centralwidget);

    //设置字体
    QFont tf = timerLabel_->font(); //得到副本
    tf.setBold(true); //加粗
    timerLabel_->setFont(tf); //传回去应用
    timerLabel_->setAlignment(Qt::AlignCenter); //居中
    timerLabel_->setMinimumWidth(80); //设置最小宽度

    //下拉框选择难度
    difficulty_ = new QComboBox(ui->centralwidget);
    difficulty_->addItem(QStringLiteral("入门 6×6     雷3"));
    difficulty_->addItem(QStringLiteral("简单 9×9     雷10"));
    difficulty_->addItem(QStringLiteral("中等 16×16 雷40"));
    difficulty_->addItem(QStringLiteral("高手 24×24 雷100"));
    difficulty_->addItem(QStringLiteral("专家 24×30 雷180"));
    difficulty_->setCurrentIndex(1);

    start_ = new QPushButton(QStringLiteral("开始"), ui->centralwidget); //开始按钮

    top->addWidget(minesLabel_); //雷数在最左边
    top->addStretch(1);
    top->addWidget(timerLabel_); //计时被前后两个strech挤在中间从而居中
    top->addStretch(1);
    top->addWidget(difficulty_);
    top->addWidget(start_);

    root->addLayout(top);

    //棋盘
    board_ = new BoardWidget(engine_, ui->centralwidget);
    board_->setCellSize(24);
    board_->setStarted(false);

    root->addStretch(1);
    root->addWidget(board_, 0, Qt::AlignHCenter); //与上面的类似，上下两个stretch把垂直方向挤在中间，然后用水平居中对齐
    root->addStretch(1);

    connect(start_, &QPushButton::clicked, this, [this]()
    {
        startSelection();
    }); //开始按钮的信号槽

    //每1秒触发一次刷新计时，还没开始点击就不计时
    timer_ = new QTimer(this);
    timer_->setInterval(1000);

    connect(timer_, &QTimer::timeout, this, [this]()
    {
        engine_.tick(1);

        const int t = engine_.getTime();
        const int m = t / 60;
        const int s = t % 60;
        timerLabel_->setText(QString("%1:%2")
                             .arg(m, 2, 10, QChar('0'))
                             .arg(s, 2, 10, QChar('0')));

        minesLabel_->setText(QStringLiteral("雷: %1").arg(engine_.getMinesLeft()));

        if (engine_.getState() == GameState::SWin || engine_.getState() == GameState::SLost)
            timer_->stop();
    }); //lambda更新计数和计时，游戏结束停止计时

    adjustSize(); //让窗口大小适配内容

    loadFile();
}

void MainWindow::startSelection()
{
    GameConfig config;

    switch (difficulty_->currentIndex())
    {
    case 0: config.row = 6;
        config.col = 6;
        config.mines = 3;
        break;
    case 1: config.row = 9;
        config.col = 9;
        config.mines = 10;
        break;
    case 2: config.row = 16;
        config.col = 16;
        config.mines = 40;
        break;
    case 3: config.row = 24;
        config.col = 24;
        config.mines = 100;
        break;
    case 4: config.row = 24;
        config.col = 30;
        config.mines = 180;
        break;
    default: config.row = 9;
        config.col = 9;
        config.mines = 10;
        break;
    } //下拉框选项

    try
    {
        engine_.newGame(config);
    }
    catch (const std::exception& e)
    {
        QMessageBox::warning(this, "配置错误", e.what()); //异常处理，校验行数列数地雷数是否合法
        return;
    }

    board_->setBoardSize();
    board_->setStarted(true);

    //归零
    timerLabel_->setText("00:00");
    minesLabel_->setText(QStringLiteral("雷: %1").arg(engine_.getMinesLeft()));

    timer_->start(); //让计时开始刷新

    adjustSize();
}

void MainWindow::loadFile()//加载最佳成绩文件
{
    for (int i = 0; i < 5; i++)
    {
        bestScores_[i] = -1;
    }//先初始化为-1

    std::filesystem::path dir = std::filesystem::current_path() / "appdata";
    std::filesystem::create_directories(dir);

    std::ifstream in(dir / "data.dt");//在本exe位于的文件夹下的appdata文件夹的darta.dt里
    if (!in)
    {
        return;//如果打开失败就直接返回
    }

    for (int i = 0; i < 5; i++)
    {
        if (!(in >> bestScores_[i]))
        {
            break;
        }
    }
}

void MainWindow::saveFile() const//保存最佳成绩文件
{
    std::filesystem::path dir = std::filesystem::current_path() / "appdata";
    std::filesystem::create_directories(dir);

    std::ofstream out(dir / "data.dt", std::ios::trunc);
    if (!out)
    {
        return;//如果打开失败就直接返回
    }

    for (int i = 0; i < 5; i++)
    {
        if (i)
        {
            out << ' ';
        }
        out << bestScores_[i];
    }
    out << '\n';
}

void MainWindow::updateBestScores()
{
    int idx = difficulty_->currentIndex();
    int tm = engine_.getTime();
    if (bestScores_[idx] == -1 || tm < bestScores_[idx])
    {
        bestScores_[idx] = tm;//用时少则修改
        saveFile();
    }
}

static QString formatTime(int second)//把秒数转化为分钟：秒数
{
    if (second < 0)
    {
        return QStringLiteral("无");
    }
    int m = second / 60;
    int s = second % 60;
    return QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
}

void MainWindow::showBestScores()//查看最佳成绩窗口
{
    const char* names[5] = {"入门", "简单", "中等", "高手", "专家"};

    QString text;
    for (int i = 0; i < 5; i++)
    {
        text += QString("%1：%2\n").arg(names[i]).arg(formatTime(bestScores_[i]));
    }

    QMessageBox::information(this, QStringLiteral("最佳成绩"), text);
}

void MainWindow::clearBestScores()//清除最佳成绩窗口
{
    if (QMessageBox::question(this, QStringLiteral("确认"),QStringLiteral("确定要清除所有难度的最佳成绩吗？")) != QMessageBox::Yes)
    {
        return;
    }

    for (int i = 0; i < 5; i++)
    {
        bestScores_[i] = -1;
    }
    saveFile();

    QMessageBox::information(this, QStringLiteral("完成"),QStringLiteral("已清除最佳成绩。"));
}

MainWindow::~MainWindow()
{
    saveFile();
    delete ui;
}
