// =============================================================================
// MainWindow.cpp — MVC 重构实现
// =============================================================================
#include "MainWindow.h"
#include "delegates/AlarmDelegate.h"
#include "delegates/StatusDelegate.h"
#include "delegates/ValueDelegate.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QGroupBox>

MainWindow::MainWindow(DataController& dataCtrl, AlarmController& alarmCtrl,
                       ILogger* logger, QWidget* parent)
    : QMainWindow(parent), m_dataCtrl(dataCtrl), m_alarmCtrl(alarmCtrl), m_logger(logger)
{
    setWindowTitle("MYShangweiji DCS — 插件化 + MVC");
    resize(1400, 900);
    setupUi();
    connectSignals();
}

void MainWindow::setupUi() {
    m_toolbar = addToolBar("控制");
    m_btnConnect = new QPushButton("▶ 启动");
    m_btnDisconnect = new QPushButton("⏹ 停止");
    m_btnDisconnect->setEnabled(false);
    m_toolbar->addWidget(m_btnConnect);
    m_toolbar->addWidget(m_btnDisconnect);

    // ── 中央区域: QSplitter 垂直分割 ──
    auto *splitter = new QSplitter(Qt::Vertical, this);
    setCentralWidget(splitter);

    // ═══════════════════════════════════════
    // 上半: 告警面板 + 设备状态 (水平分割)
    // ═══════════════════════════════════════
    auto *topSplitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(topSplitter);

    // 告警列表
    auto *alarmGroup = new QGroupBox("告警列表");
    auto *alarmLayout = new QVBoxLayout(alarmGroup);
    m_alarmView = new QTableView;
    m_alarmModel = new AlarmTableModel(this);
    m_alarmDelegate = new AlarmDelegate(this);
    m_alarmView->setModel(m_alarmModel);
    m_alarmView->setItemDelegate(m_alarmDelegate);
    m_alarmView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_alarmView->horizontalHeader()->setStretchLastSection(true);
    m_alarmView->verticalHeader()->hide();
    alarmLayout->addWidget(m_alarmView);
    topSplitter->addWidget(alarmGroup);

    // 设备状态
    auto *deviceGroup = new QGroupBox("设备状态");
    auto *deviceLayout = new QVBoxLayout(deviceGroup);
    m_deviceView = new QTableView;
    m_deviceModel = new DeviceStatusModel(this);
    m_statusDelegate = new StatusDelegate(this);
    m_deviceView->setModel(m_deviceModel);
    m_deviceView->setItemDelegate(m_statusDelegate);
    m_deviceView->horizontalHeader()->setStretchLastSection(true);
    m_deviceView->verticalHeader()->hide();
    deviceLayout->addWidget(m_deviceView);
    topSplitter->addWidget(deviceGroup);

    // ═══════════════════════════════════════
    // 下半: 实时数据
    // ═══════════════════════════════════════
    auto *tagGroup = new QGroupBox("实时数据");
    auto *tagLayout = new QVBoxLayout(tagGroup);
    m_tagView = new QTableView;
    m_tagModel = new TagDataModel(this);
    m_valueDelegate = new ValueDelegate(this);
    m_tagView->setModel(m_tagModel);
    m_tagView->setItemDelegate(m_valueDelegate);
    m_tagView->horizontalHeader()->setStretchLastSection(true);
    m_tagView->verticalHeader()->hide();
    tagLayout->addWidget(m_tagView);
    splitter->addWidget(tagGroup);

    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);

    // ── 状态栏 ──
    m_statusLabel = new QLabel("就绪");
    statusBar()->addPermanentWidget(m_statusLabel);

    // ── 刷新定时器 ──
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(1000);
    connect(m_refreshTimer, &QTimer::timeout, this, &MainWindow::refreshAll);
}

void MainWindow::connectSignals() {
    connect(m_btnConnect, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_btnDisconnect, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(m_alarmDelegate, &AlarmDelegate::ackClicked, this, &MainWindow::onAckClicked);
    connect(&m_alarmCtrl, &AlarmController::alarmTriggered, this, &MainWindow::onAlarmTriggered);
}

void MainWindow::setConnected(bool connected) {
    m_connected = connected;
    m_btnConnect->setEnabled(!connected);
    m_btnDisconnect->setEnabled(connected);
    if (connected) m_refreshTimer->start();
    else m_refreshTimer->stop();
}

void MainWindow::onConnectClicked() {
    m_dataCtrl.connectAll();
    setConnected(true);
    m_statusLabel->setText("运行中");
}

void MainWindow::onDisconnectClicked() {
    m_dataCtrl.disconnectAll();
    setConnected(false);
    m_statusLabel->setText("已停止");
}

void MainWindow::onAlarmTriggered(const AlarmEvent& event) {
    m_alarmModel->appendEvent(event);
}

void MainWindow::onAckClicked(int row) {
    AlarmEvent e = m_alarmModel->eventAt(row);
    if (!e.alarmId.isEmpty()) {
        m_alarmCtrl.acknowledgeAlarm(e.alarmId, "操作员");
        m_statusLabel->setText(QString("确认告警: %1").arg(e.alarmId));
    }
}

void MainWindow::refreshAll() {
    // 刷新数据表
    m_tagModel->refresh(m_dataCtrl.readRealtimeData());
    m_statusLabel->setText(QString("运行中 | 告警:%1 | 刷新:%2")
        .arg(m_alarmModel->rowCount())
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
}
