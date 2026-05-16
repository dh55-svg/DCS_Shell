// =============================================================================
// MainWindow.h — MVC 重构版主窗口
// =============================================================================
#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QTabWidget>
#include <QTableView>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include <QSplitter>
#include "../application/DataController.h"
#include "../application/AlarmController.h"
#include "models/AlarmTableModel.h"
#include "models/DeviceStatusModel.h"
#include "models/TagDataModel.h"

class AlarmDelegate;
class StatusDelegate;
class ValueDelegate;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(DataController& dataCtrl, AlarmController& alarmCtrl,
                        ILogger* logger = nullptr, QWidget* parent = nullptr);
    void setConnected(bool connected);

private slots:
    void refreshAll();
    void onAlarmTriggered(const AlarmEvent& event);
    void onAckClicked(int row);
    void onConnectClicked();
    void onDisconnectClicked();

private:
    void setupUi();
    void connectSignals();

    DataController& m_dataCtrl;
    AlarmController& m_alarmCtrl;
    ILogger* m_logger;

    // Models
    AlarmTableModel* m_alarmModel;
    DeviceStatusModel* m_deviceModel;
    TagDataModel* m_tagModel;

    // Views
    QTableView* m_alarmView;
    QTableView* m_deviceView;
    QTableView* m_tagView;

    // Delegates
    AlarmDelegate* m_alarmDelegate;
    StatusDelegate* m_statusDelegate;
    ValueDelegate* m_valueDelegate;

    // UI
    QToolBar* m_toolbar;
    QPushButton* m_btnConnect;
    QPushButton* m_btnDisconnect;
    QLabel* m_statusLabel;
    QTimer* m_refreshTimer;
    bool m_connected = false;
};

#endif
