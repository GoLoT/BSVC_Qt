#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <sstream>
#include "qtinterface.h"
#include "Widgets/memorydockwidget.h"

class QtInterface;

namespace Ui {
  class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

private slots:
  void on_consoleInput_returnPressed();
  void on_actionLoadProgram_triggered();
  void on_logOutput_changed();
  void new68000Processor();
  void new68360Processor();
  void resetInterface();
  void on_createProcessorButton_pressed();
  void onStep();
  void onRun();
  void onRunStep();
  void onReset();
  void onToggleBP();
  void onRegistersChanged();
  void on_editRegisterButton_pressed();
  void on_newMemViewButton_pressed();
  void on_memViewButton_pressed();
  void on_addBreakpointButton_pressed();
  void on_removeBPButton_pressed();
  void onListingLoaded();
  void onProgramCounterChanged();
  void on_gotoBPButton_pressed();
  void onStartRunning();
  void onStopRunning(QString);

  void on_actionLoad_setup_triggered();

  void on_actionSet_RunStep_delay_triggered();

signals:
  void logOutputChanged();
  void newDeviceCreated();
  void stepped();
  void memoryChanged();
  void registersChanged();
  void breakpointsChanged();
  void programCounterChanged(Address address);
  void stoppedRunning(QString);
  void startedRunning();
  void stopRunning();

private:
  Ui::MainWindow *ui;
  QtInterface* newProcessor(QString type);
  std::stringstream logBuffer;
  RegisterModelList* registers;
  BreakpointModelList* breakpoints;
  CodeLineList* code;
  bool running = false;
  QLabel statusbarStatus;
  QLabel statusbarExtraInfo;
  QAction* stepAction;
  QAction* runAction;
  QAction* runStepAction;
  QAction* resetAction;
  QAction* toggleBPAction;
  MemoryDockWidget* memoryDock;
  QList<MemoryDockWidget*> memoryDockList;
  MemoryModelList* memory;
  int runStepWait = 220;

public:
  QtInterface *interface;
  void outputChanged();
  std::stringstream* GetLogBuffer();
  void Trace(std::string str);
  void TraceLn(std::string str);
  void Log(std::string str);
  void LogLn(std::string str);
  void setStatusbarTemp(std::string str);
  void setStatusbarTemp(QString str);
  bool isRunning();
  //int setRunStepWait(int s) {runStepWait = s;}
  int getRunStepWait() {return runStepWait;}

  friend class QtInterface;
  friend class RunModeWorker;
  friend class MemoryDockWidget;
};

#endif // MAINWINDOW_H
